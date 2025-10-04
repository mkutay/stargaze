#include <iostream>
#include <random>
#include <stack>
#include "search.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "move.hpp"
#include "get_hash.hpp"

const int MAX_DEPTH = 12;
const long long TIME_LIMIT_MS = 5000;

int32_t main() {
    init_eval_table();
    init_hash_table();
    Board board;
    
    Search search(&board);
    
    for (int i = 0; i < 100; i++) {
        std::cout << "\n=== Move " << (i + 1) << " ===\n";
        std::cout << "Current board position:\n" << board.to_string() << std::endl;

        // Perform iterative deepening search
        SearchInfo result = search.iterative_deepening(MAX_DEPTH, TIME_LIMIT_MS);
        
        std::cout << "\nSearch completed:" << std::endl;
        std::cout << "Final depth: " << result.depth << std::endl;
        std::cout << "Best score: " << result.score << std::endl;
        std::cout << "Nodes searched: " << result.nodes << std::endl;
        std::cout << "Time taken: " << result.time_ms << "ms" << std::endl;
        std::cout << "NPS: " << (result.time_ms > 0 ? (result.nodes * 1000ll) / result.time_ms : 0) << std::endl;
        
        if (!result.pv.moves.empty() && 
            result.pv.moves[0].from() != result.pv.moves[0].to()) { // Valid move check
            Move best_move = result.pv.moves[0];
            std::cout << "Best move: " << best_move.to_string() << std::endl;
            
            // Make the best move
            board.make_move(best_move);
        } else {
            std::cout << "No valid move found!" << std::endl;
            break;
        }
    }
    
    return 0;
}
