#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "test_helpers.hpp"

TEST_SUITE("unit") {
    TEST_CASE("Evaluation symmetry on mirrored positions") {
        for (const auto &fen : test::EVAL_SYMMETRY_FENS) {
            Board board(fen);
            Board mirrored_board = board.mirrored();

            CHECK(board.evaluate() == mirrored_board.evaluate());
        }
    }
}
