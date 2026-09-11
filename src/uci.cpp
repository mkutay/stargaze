#include "uci.hpp"

#include "board.hpp"
#include "search.hpp"
#include "time_management.hpp"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <limits>
#include <mutex>
#include <print>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace uci {
namespace {

constexpr uint16_t MAX_DEPTH = 60;
constexpr uint32_t INFINITE_TIME_MS = 100'000'000;

struct GoParams {
    uint16_t max_depth = MAX_DEPTH;
    uint32_t time_limit = INFINITE_TIME_MS;
    uint64_t node_limit = std::numeric_limits<uint64_t>::max();
    std::vector<Move> search_moves;
    bool wait_for_stop = false;
    bool ponder = false;
};

std::vector<std::string> tokens(const std::string &line) {
    std::istringstream iss(line);
    std::vector<std::string> result;
    for (std::string token; iss >> token;)
        result.push_back(token);
    return result;
}

std::optional<Move> find_move(Board &board, const std::string &text) {
    const auto moves = board.get_moves();
    const auto found =
        std::find_if(moves.begin(), moves.end(),
                     [&text](Move move) { return move.to_string() == text; });
    return found == moves.end() ? std::nullopt : std::optional(*found);
}

GoParams parse_go(const std::string &line, Board &board, const Search &search) {
    const auto args = tokens(line);
    uint32_t wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;
    bool has_clock = false;
    GoParams params;

    for (size_t i = 1; i < args.size(); ++i) {
        const auto read_u64 = [&args, &i](uint64_t &value) {
            if (i + 1 < args.size()) {
                try {
                    value = std::stoull(args[++i]);
                } catch (...) {
                }
            }
        };
        uint64_t value = 0;

        if (args[i] == "searchmoves") {
            while (i + 1 < args.size()) {
                auto move = find_move(board, args[i + 1]);
                if (!move)
                    break;
                params.search_moves.push_back(*move);
                ++i;
            }
        } else if (args[i] == "ponder") {
            params.ponder = params.wait_for_stop = true;
        } else if (args[i] == "wtime") {
            read_u64(value);
            wtime = static_cast<uint32_t>(value);
            has_clock = true;
        } else if (args[i] == "btime") {
            read_u64(value);
            btime = static_cast<uint32_t>(value);
            has_clock = true;
        } else if (args[i] == "winc") {
            read_u64(value);
            winc = static_cast<uint32_t>(value);
        } else if (args[i] == "binc") {
            read_u64(value);
            binc = static_cast<uint32_t>(value);
        } else if (args[i] == "movestogo") {
            read_u64(value);
            movestogo = static_cast<uint32_t>(value);
        } else if (args[i] == "depth") {
            read_u64(value);
            params.max_depth = static_cast<uint16_t>(
                std::clamp<uint64_t>(value, 1, Search::MAX_SEARCH_DEPTH));
        } else if (args[i] == "nodes") {
            read_u64(params.node_limit);
        } else if (args[i] == "mate") {
            read_u64(value);
            params.max_depth = static_cast<uint16_t>(
                std::clamp<uint64_t>(value * 2, 1, Search::MAX_SEARCH_DEPTH));
        } else if (args[i] == "movetime") {
            read_u64(value);
            params.time_limit = static_cast<uint32_t>(value);
        } else if (args[i] == "infinite") {
            params.wait_for_stop = true;
        }
    }

    if (params.time_limit == INFINITE_TIME_MS && has_clock &&
        !params.wait_for_stop) {
        params.time_limit = calculate_time_limit(board, search, wtime, btime,
                                                 winc, binc, movestogo);
    }
    return params;
}

void print_search_info(const SearchInfo &info) {
    std::print("info depth {}", info.depth);
    if (info.score.is_mate())
        std::print(" score mate {}", info.score.mate_moves());
    else
        std::print(" score cp {}", info.score.raw());

    const uint64_t nps =
        info.time_ms > 0 ? info.nodes * 1000 / info.time_ms : 0;
    std::print(" nodes {} time {} nps {} pv", info.nodes, info.time_ms, nps);
    for (Move move : info.pv.moves)
        std::print(" {}", move.to_string());
    std::print("\n");
}

class SearchController {
    Search &search;
    std::thread thread;
    std::mutex mutex;
    std::condition_variable cv;
    bool searching = false;
    bool result_ready = false;
    bool release_result = true;
    SearchInfo result{MAX_DEPTH};

    void print_bestmove() {
        if (!result.pv.moves.empty())
            std::println("bestmove {}", result.pv.moves.front().to_string());
        else
            std::println("bestmove 0000");
    }

  public:
    explicit SearchController(Search &search) : search(search) {}
    ~SearchController() { stop(); }

    void start(GoParams params) {
        stop();
        search.set_limits(params.node_limit, std::move(params.search_moves));
        search.stop_flag.store(false, std::memory_order_relaxed);
        {
            std::lock_guard lock(mutex);
            searching = true;
            result_ready = false;
            release_result = !params.wait_for_stop;
        }
        thread = std::thread([this, params]() {
            SearchInfo current = search.iterative_deepening(
                params.max_depth, params.time_limit, print_search_info);
            std::unique_lock lock(mutex);
            result = std::move(current);
            result_ready = true;
            cv.wait(lock, [this] { return release_result; });
            print_bestmove();
            searching = false;
        });
    }

    void stop() {
        if (!thread.joinable())
            return;
        search.stop_flag.store(true, std::memory_order_relaxed);
        {
            std::lock_guard lock(mutex);
            release_result = true;
        }
        cv.notify_one();
        thread.join();
    }

    void ponderhit() {
        std::lock_guard lock(mutex);
        release_result = true;
        cv.notify_one();
    }
};

void set_position(Board &board, std::istringstream &iss) {
    std::string token;
    if (!(iss >> token))
        return;

    if (token == "startpos") {
        board = Board();
        iss >> token;
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") {
            if (!fen.empty())
                fen += ' ';
            fen += token;
        }
        if (!fen.empty())
            board = Board(fen);
    } else {
        return;
    }

    if (token != "moves")
        return;
    while (iss >> token) {
        auto move = find_move(board, token);
        if (!move)
            break;
        board.make_move(*move);
    }
}

} // namespace

void run() {
    std::setbuf(stdout, nullptr);
    Board board;
    Search search(&board);
    SearchController controller(search);
    bool debug = false;

    for (std::string line; std::getline(std::cin, line);) {
        std::istringstream iss(line);
        std::string command;
        if (!(iss >> command))
            continue;

        if (debug)
            std::println("info string received {}", line);

        if (command == "uci") {
            std::print("id name stargaze\n"
                       "id author mkutay\n"
                       "option name Clear Hash type button\n"
                       "option name Ponder type check default true\n"
                       "uciok\n");
        } else if (command == "debug") {
            std::string value;
            if (iss >> value)
                debug = value == "on";
        } else if (command == "isready") {
            std::println("readyok");
        } else if (command == "setoption") {
            std::string token, name;
            iss >> token;
            while (iss >> token && token != "value") {
                if (!name.empty())
                    name += ' ';
                name += token;
            }
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name == "clear hash")
                search.clear_tt();
        } else if (command == "register") {
            // Stargaze does not require registration.
        } else if (command == "ucinewgame") {
            controller.stop();
            board = Board();
            search.clear_tt();
        } else if (command == "position") {
            controller.stop();
            set_position(board, iss);
        } else if (command == "go") {
            controller.start(parse_go(line, board, search));
        } else if (command == "stop") {
            controller.stop();
        } else if (command == "ponderhit") {
            controller.ponderhit();
        } else if (command == "d" || command == "print") {
            std::println("{}", board.nice());
        } else if (command == "quit") {
            controller.stop();
            break;
        }
    }
}

} // namespace uci
