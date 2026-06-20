#include "board.hpp"
#include "doctest/doctest.h"
#include <string_view>

TEST_SUITE("unit") {
    TEST_CASE("Evaluation symmetry on mirrored positions") {
        // clang-format off
        const std::string_view fens[] = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b Kq e3 0 1",
            // Asymmetric pawns and pieces
            "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d3 0 2",
            "k7/8/8/8/8/8/8/7K b - - 15 40"
        };
        // clang-format on

        for (const auto &fen : fens) {
            Board board(fen);
            Board mirrored_board = board.mirrored();

            // The evaluation score should be symmetric.
            CHECK(board.evaluate() == mirrored_board.evaluate());
        }
    }
}
