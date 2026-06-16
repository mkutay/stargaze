/**
 * Perft (Performance Test) executable.
 *
 * Standard Perft Positions & Expected Node Counts
 * -------------------------------------------------------------------------
 * Position 1 (Start Position)
 * FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
 * Depth 1: 20
 * Depth 2: 400
 * Depth 3: 8902
 * Depth 4: 197281
 * Depth 5: 4865609
 * Depth 6: 119060324
 *
 * Position 2 (Kiwipete)
 * FEN: r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
 * Depth 1: 48
 * Depth 2: 2039
 * Depth 3: 97862
 * Depth 4: 4085603
 * Depth 5: 193690690
 *
 * Position 3
 * FEN: 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1
 * Depth 1: 14
 * Depth 2: 191
 * Depth 3: 2812
 * Depth 4: 43238
 * Depth 5: 674624
 * Depth 6: 11030083
 *
 * Position 4
 * FEN: r3k2r/Pppp1ppp/1p6/8/8/8/PPPpPPPP/R3K2R b KQkq - 0 1
 * Depth 1: 6
 * Depth 2: 264
 * Depth 3: 9467
 * Depth 4: 422333
 * Depth 5: 15833292
 * Depth 6: 706045033
 *
 * Position 5
 * FEN: rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8
 * Depth 1: 44
 * Depth 2: 1486
 * Depth 3: 62379
 * Depth 4: 2103487
 * Depth 5: 89941194
 *
 * Position 6
 * FEN: r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10
 * Depth 1: 46
 * Depth 2: 2079
 * Depth 3: 89890
 * Depth 4: 3894594
 * Depth 5: 164075551
 */
#include "board.hpp"
#include <iostream>
#include <string>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <fen> <depth> [--divide]\n";
        return 1;
    }

    std::string fen = argv[1];
    int depth = std::stoi(argv[2]);
    bool divide = (argc > 3 && std::string(argv[3]) == "--divide");

    Board board(fen);
    
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t total_nodes = 0;

    if (divide) {
        if (depth == 0) {
            total_nodes = 1;
        } else {
            auto moves = board.get_moves();
            for (Move move : moves) {
                board.make_move(move);
                uint64_t nodes = board.perft(depth - 1);
                board.undo_move();
                
                std::string move_str = move.from().to_string() + move.to().to_string();
                if (move.is_promotion()) {
                    switch (move.promotion_piece()) {
                        case Piece::KNIGHT: move_str += 'n'; break;
                        case Piece::BISHOP: move_str += 'b'; break;
                        case Piece::ROOK: move_str += 'r'; break;
                        case Piece::QUEEN: move_str += 'q'; break;
                        default: break;
                    }
                }
                
                std::cout << move_str << ": " << nodes << "\n";
                total_nodes += nodes;
            }
        }
    } else {
        total_nodes = board.perft(depth);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "\nTotal time (ms) : " << duration << "\n";
    std::cout << "Nodes searched  : " << total_nodes << "\n";
    if (duration > 0) {
        std::cout << "Nodes/sec       : " << (total_nodes * 1000) / duration << "\n";
    }

    return 0;
}
