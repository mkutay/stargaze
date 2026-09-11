#include "board.hpp"
#include "doctest/doctest.h"
#include "search.hpp"
#include "test_helpers.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <print>
#include <string_view>

namespace {
struct Position {
    std::string_view name;
    std::string_view fen;
};

constexpr std::array POSITIONS = {
    Position{"startpos", test::START_POSITION},
    Position{"kiwipete", test::KIWIPETE},
    Position{"endgame", test::CPW_POSITION_3},
};

void print_result(std::string_view mode, const Position &position,
                  const SearchInfo &result) {
    const uint64_t nps =
        result.time_ms == 0 ? 0 : result.nodes * 1000 / result.time_ms;
    std::println("benchmark mode={} position={} depth={} time_ms={} nodes={} "
                 "nps={} stopped={}",
                 mode, position.name, result.depth, result.time_ms,
                 result.nodes, nps, result.stopped);
}
} // namespace

TEST_SUITE("benchmark") {
    TEST_CASE("time to reach a fixed depth") {
        constexpr uint16_t target_depth = 7;

        for (const Position &position : POSITIONS) {
            Board board(position.fen);
            Search search(&board);
            search.set_limits();
            const SearchInfo result = search.iterative_deepening<false>(
                target_depth, std::numeric_limits<uint32_t>::max());

            print_result("fixed-depth", position, result);
            REQUIRE_MESSAGE(result.depth == target_depth, position.name);
            CHECK_FALSE(result.stopped);
        }
    }

    TEST_CASE("depth reached in a fixed time") {
        constexpr uint32_t time_budget_ms = 1000;

        for (const Position &position : POSITIONS) {
            Board board(position.fen);
            Search search(&board);
            search.set_limits();
            const SearchInfo result = search.iterative_deepening<false>(
                Search::MAX_SEARCH_DEPTH, time_budget_ms);

            print_result("fixed-time", position, result);
            CHECK_MESSAGE(result.depth > 0, position.name);
            CHECK(result.stopped);
        }
    }
}
