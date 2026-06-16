#include "board.hpp"
#include "move.hpp"
#include "search.hpp"
#include <iostream>
#include <print>

#ifdef DEBUG
#include "debug.hpp"
#else
#define debug(...) void(38)
#endif

const int MAX_DEPTH = 12;
const long long TIME_LIMIT_MS = 5000;

int32_t main() {
    Board board;
    Search search(&board);

    std::optional<SearchInfo> info = std::nullopt;

    while (true) {
        std::print("\n===============\nMove {}: {}\n===============\n",
                   board.get_move_history().size() / 2 + 1,
                   board.get_turn() == Colour::WHITE ? "WHITE" : "BLACK");
        std::cout << board.to_string() << std::endl;

        if (board.is_draw()) {
            std::cout << "Game drawn!" << std::endl;
            break;
        }

        // Perform iterative deepening search
        SearchInfo result =
            search.iterative_deepening(MAX_DEPTH, TIME_LIMIT_MS, info);

        debug(result);

        if (!result.pv.moves.empty()) {
            Move best_move = result.pv.moves[0];
            info = result;

            // Make the best move
            board.make_move(best_move);
        } else {
            std::cout << "No valid move found!" << std::endl;
            break;
        }
    }

    return 0;
}
