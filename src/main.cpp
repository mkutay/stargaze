#include "board.hpp"
#include "move.hpp"
#include "search.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <thread>

constexpr uint16_t MAX_DEPTH = 60;
constexpr uint32_t INFINITE_TIME_MS = 100'000'000;
constexpr uint32_t SELF_PLAY_TIME_LIMIT_MS = 5000;

#ifdef DEBUG
#include "debug.hpp"
#else
#define debug(...) void()
#endif

struct GoParams {
    uint16_t max_depth = MAX_DEPTH;
    uint32_t time_limit = INFINITE_TIME_MS;
};

uint32_t calculate_time_limit(Colour turn, uint32_t wtime, uint32_t btime,
                              uint32_t winc, uint32_t binc, uint32_t movestogo,
                              uint32_t moves_played) {
    const uint32_t time_left = (turn == Colour::WHITE) ? wtime : btime;
    const uint32_t inc = (turn == Colour::WHITE) ? winc : binc;

    // Estimate remaining moves using expected total game length of 40 moves
    // (80 plies); fall back to 5 moves once past that mark.
    const uint32_t moves_remaining =
        (movestogo > 0) ? movestogo
                        : ((moves_played >= 40) ? 5u : (40u - moves_played));

    // Allocate a fraction of the remaining time, plus a portion of the
    // increment.
    uint32_t target_time = time_left / moves_remaining + inc * 4 / 5;

    // Hard max: 50% of the remaining time; tighten to 30% in panic mode.
    const uint32_t max_time =
        (time_left < 1000) ? (time_left * 3 / 10) : (time_left / 2);

    // Hard min: 10 ms or 1% of remaining time, whichever is larger.
    const uint32_t min_time =
        std::min(std::max(10u, time_left / 100), time_left);

    target_time = std::clamp(target_time, min_time, max_time);

    // Ensure we never return 0.
    return std::max(target_time, 1u);
}

void stop_and_wait_for_search(Search &search, std::thread &search_thread) {
    if (search_thread.joinable()) {
        search.stop_flag.store(true, std::memory_order_relaxed);
        search_thread.join();
    }
}

GoParams parse_go(const std::string &cmd, Colour turn, uint32_t moves_played) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token; // consume "go"

    uint32_t wtime = 0;
    uint32_t btime = 0;
    uint32_t winc = 0;
    uint32_t binc = 0;
    uint32_t movestogo = 0;
    uint32_t movetime = 0;
    bool depth_specified = false;
    bool movetime_specified = false;
    bool infinite = false;

    GoParams params;

    while (iss >> token) {
        if (token == "wtime") {
            iss >> wtime;
        } else if (token == "btime") {
            iss >> btime;
        } else if (token == "winc") {
            iss >> winc;
        } else if (token == "binc") {
            iss >> binc;
        } else if (token == "movestogo") {
            iss >> movestogo;
        } else if (token == "depth") {
            uint32_t d;
            iss >> d;
            params.max_depth = static_cast<uint16_t>(d);
            depth_specified = true;
        } else if (token == "movetime") {
            iss >> movetime;
            movetime_specified = true;
        } else if (token == "infinite") {
            infinite = true;
        }
    }

    if (movetime_specified) {
        params.time_limit = movetime;
    } else if (!infinite && !depth_specified && (wtime > 0 || btime > 0)) {
        params.time_limit = calculate_time_limit(turn, wtime, btime, winc, binc,
                                                 movestogo, moves_played);
    }

    // infinite / depth_specified / fallback: params.time_limit stays
    // INFINITE_TIME_MS.

    return params;
}

void run_perft(Board &board, int depth) {
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t total_nodes = 0;

    if (depth == 0) {
        total_nodes = 1;
    } else {
        auto moves = board.get_moves();
        for (Move move : moves) {
            board.make_move(move);
            uint64_t nodes = board.perft(depth - 1);
            board.undo_move();

            std::string move_str = move.to_string();
            std::println("{}: {}", move_str, nodes);
            total_nodes += nodes;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();

    std::println("\nTotal time (ms) : {}", duration);
    std::println("Nodes searched  : {}", total_nodes);
    if (duration > 0) {
        std::println("Nodes/sec       : {}", (total_nodes * 1000) / duration);
    }
}

void run_uci_loop() {
    // Disable buffering on stdout so every write is immediately visible.
    std::setbuf(stdout, nullptr);

    Board board;
    Search search(&board);
    std::thread search_thread;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            std::print("id name stargaze\n"
                       "id author mkutay\n"
                       "uciok\n");
        } else if (command == "isready") {
            std::println("readyok");
        } else if (command == "ucinewgame") {
            stop_and_wait_for_search(search, search_thread);
            board = Board();
            search.clear_tt();
        } else if (command == "position") {
            stop_and_wait_for_search(search, search_thread);

            std::string token;
            iss >> token;
            if (token == "startpos") {
                board = Board();
                iss >> token; // consume "moves" if present
            } else if (token == "fen") {
                std::string fen;
                while (iss >> token && token != "moves") {
                    if (!fen.empty())
                        fen += ' ';
                    fen += token;
                }
                board = Board(fen);
            }

            if (token == "moves") {
                while (iss >> token) {
                    const auto moves = board.get_moves();
                    for (const auto &m : moves) {
                        if (m.to_string() == token) {
                            board.make_move(m);
                            break;
                        }
                    }
                    // Unknown tokens are silently ignored per UCI spec.
                }
            }
        } else if (command == "go") {
            stop_and_wait_for_search(search, search_thread);

            std::string token;
            if (iss >> token && token == "perft") {
                int depth;
                if (iss >> depth) {
                    run_perft(board, depth);
                } else {
                    std::println("Error: depth missing for perft");
                }
            } else {
                const auto moves_played = board.get_move_history().size() / 2;
                const auto params =
                    parse_go(line, board.get_turn(), moves_played);

                search.stop_flag.store(false, std::memory_order_relaxed);
                search_thread = std::thread([&search, params]() {
                    const SearchInfo result = search.iterative_deepening<true>(
                        params.max_depth, params.time_limit);
                    if (!result.pv.moves.empty()) {
                        std::println("bestmove {}",
                                     result.pv.moves.front().to_string());
                    } else {
                        std::println("bestmove 0000");
                    }
                });
            }
        } else if (command == "perft") {
            stop_and_wait_for_search(search, search_thread);
            int depth = 1;
            if (iss >> depth) {
                run_perft(board, depth);
            } else {
                std::println("Error: depth missing for perft");
            }
        } else if (command == "stop") {
            stop_and_wait_for_search(search, search_thread);
        } else if (command == "d" || command == "print") {
            std::println("{}", board.to_string());
        } else if (command == "quit") {
            stop_and_wait_for_search(search, search_thread);
            break;
        }
    }

    // Ensure the search thread is joined before the stack objects are
    // destroyed.
    stop_and_wait_for_search(search, search_thread);
}

void run_self_play() {
    Board board;
    Search search(&board);

    while (true) {
        const std::size_t move_number = board.get_move_history().size() / 2 + 1;
        const std::string_view side =
            (board.get_turn() == Colour::WHITE) ? "WHITE" : "BLACK";

        std::cout << "\n===============\n"
                  << "Move " << move_number << ": " << side << '\n'
                  << "===============\n"
                  << board.to_string() << '\n';

        if (board.is_draw()) {
            std::cout << "Game drawn!\n";
            break;
        }

        const SearchInfo result = search.iterative_deepening<false>(
            MAX_DEPTH, SELF_PLAY_TIME_LIMIT_MS);

        debug(result);

        if (result.pv.moves.empty()) {
            std::cout << "No valid move found!\n";
            break;
        }

        board.make_move(result.pv.moves[0]);
    }
}

int main(int argc, char **argv) {
    const bool selfplay = [&]() {
        for (int i = 1; i < argc; ++i) {
            if (std::string_view(argv[i]) == "--selfplay")
                return true;
        }
        return false;
    }();

    if (selfplay) {
        run_self_play();
    } else {
        run_uci_loop();
    }
    return 0;
}
