#include "board.hpp"
#include "doctest/doctest.h"
#include <string>

TEST_SUITE("unit") {
    TEST_CASE("FEN Round-trip serialization") {
        // clang-format off
        const std::string fens[] = {
            // Starting position
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            // Kiwipete (famous test position for chess engines)
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            // CPW Position 3
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            // CPW Position 4
            "r3k2r/pbp1qbp1/1p1pn1p1/1P1PN3/1p2P3/2N2Q1p/2PB1PPP/R3K2R b KQkq - 0 1",
            // CPW Position 5
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            // CPW Position 6 (symmetric)
            "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/2NP1N2/PPP1QPPP/R3K2R w KQ - 0 10",
            // Position with castling rights and en-passant square
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b Kq e3 0 1",
            // Endgames with no castling rights
            "8/8/8/8/8/8/8/8 w - - 0 1",
            "k7/8/8/8/8/8/8/7K b - - 15 40"
        };
        // clang-format on

        for (const auto &fen : fens) {
            Board board(fen);
            CHECK(board.fen() == fen);
        }
    }
}
