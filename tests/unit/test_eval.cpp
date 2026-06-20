#include "board.hpp"
#include "doctest/doctest.h"
#include "test_helpers.hpp"

TEST_SUITE("unit") {
    TEST_CASE("Evaluation symmetry on mirrored positions") {
        for (const auto &fen : test::EVAL_SYMMETRY_FENS) {
            Board board(fen);
            Board mirrored_board = board.mirrored();

            // The evaluation score should be symmetric.
            CHECK(board.evaluate() == mirrored_board.evaluate());
        }
    }
}
