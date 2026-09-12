#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "test_helpers.hpp"
#include <cstdint>
#include <format>

TEST_SUITE("integration") {
    TEST_CASE("Perft suite validation") {
        for (const auto &[name, fen, expected_nodes] : test::PERFT_TEST_CASES) {
            Board board(fen);
            for (size_t i = 0; i < expected_nodes.size(); ++i) {
                int depth = static_cast<int>(i + 1);
                uint64_t expected = expected_nodes[i];
                uint64_t actual = board.perft(depth);
                CHECK_MESSAGE(
                    actual == expected,
                    std::format("{} ({}) at depth {} expected {} but got {}.",
                                name, fen, depth, expected, actual));
            }
        }
    }
}
