#include "board.hpp"
#include "doctest/doctest.h"
#include "test_helpers.hpp"

TEST_SUITE("unit") {
    TEST_CASE("FEN Round-trip serialization") {
        for (const auto &fen : test::SERIALIZATION_FENS) {
            Board board(fen);
            CHECK(board.fen() == fen);
        }
    }
}
