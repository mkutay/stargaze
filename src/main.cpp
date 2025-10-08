#include <iostream>
#include <random>
#include <stack>
#include "search.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"

#ifdef DEBUG
  #include "debug.hpp"
#else
  #define debug(...) void(38)
#endif

const int MAX_DEPTH = 12;
const long long TIME_LIMIT_MS = 12000;

int32_t main() {
    Board board;
    
    Search search(&board);

    SearchInfo *info = nullptr;
    
    for (int i = 0; i < 100; i++) {
        std::cout << "\n=== Move " << (i + 1) << " ===\n";
        std::cout << "board:\n" << board.to_string() << std::endl;

        // Perform iterative deepening search
        SearchInfo result = search.iterative_deepening(MAX_DEPTH, TIME_LIMIT_MS, info);

        debug(result);
        
        if (!result.pv.moves.empty()) {
            Move best_move = result.pv.moves[0];
            info = &result;
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
