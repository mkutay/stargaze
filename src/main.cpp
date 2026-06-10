#include "board.hpp"
#include "move.hpp"
#include "search.hpp"
#include <iostream>

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

    for (int i = 0; i < 300; i++) {
        std::cout << "\n=== Move " << (i + 1) << " ===\n";
        std::cout << "board:\n" << board.to_string() << std::endl;

        // Perform iterative deepening search
        SearchInfo result =
            search.iterative_deepening(MAX_DEPTH, TIME_LIMIT_MS, info);

        debug(result);

        if (!result.pv.moves.empty()) {
            Move best_move = result.pv.moves[0];
            info = result;
            debug(search.get_tt()->get_num_entries());

            // Make the best move
            board.make_move(best_move);
        } else {
            std::cout << "No valid move found!" << std::endl;
            break;
        }
    }

    return 0;
}
