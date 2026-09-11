#include "board.hpp"
#include "doctest/doctest.h"
#include "test_helpers.hpp"
#include <cstdint>
#include <format>

TEST_SUITE("integration") {
    TEST_CASE("Perft suite validation") {
        for (const auto &[fen, expected_nodes] : test::PERFT_TEST_CASES) {
            Board board(fen);
            for (size_t i = 0; i < expected_nodes.size(); ++i) {
                int depth = static_cast<int>(i + 1);
                uint64_t expected = expected_nodes[i];
                uint64_t actual = board.perft(depth);
                CHECK_MESSAGE(
                    actual == expected,
                    std::format("FEN: {} at depth {} expected {} but got {}.",
                                fen, depth, expected, actual));
            }
        }
    }
}
