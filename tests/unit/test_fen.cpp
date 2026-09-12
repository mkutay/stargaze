#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "test_helpers.hpp"

TEST_SUITE("unit") {
    TEST_CASE("FEN Round-trip serialization") {
        for (const auto &fen : test::SERIALIZATION_FENS) {
            Board board(fen);
            CHECK(board.fen() == fen);
        }
    }
}
