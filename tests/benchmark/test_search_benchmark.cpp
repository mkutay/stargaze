#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "stargaze/search.hpp"
#include "test_helpers.hpp"
#include <cstdint>
#include <format>
#include <limits>
#include <locale>
#include <print>
#include <string_view>

namespace {

class apostrophe_numpunct : public std::numpunct<char> {
  protected:
    char do_thousands_sep() const override { return '\''; }
    std::string do_grouping() const override { return "\3"; }
};

const std::locale benchmark_locale{std::locale::classic(),
                                   new apostrophe_numpunct};

template <typename... Args>
void print_benchmark(std::string_view format, Args &&...args) {
    std::print("{}", std::vformat(benchmark_locale, format,
                                  std::make_format_args(args...)));
}

void print_result(std::string_view mode, const auto &position,
                  const SearchInfo &result, uint64_t nps) {
    print_benchmark(
        "benchmark mode={} position={} depth={} time_ms={:L} nodes={:L} "
        "nps={:L}\n",
        mode, position.name, result.depth, result.time_ms, result.nodes, nps);
}
} // namespace

TEST_SUITE("benchmark") {
    TEST_CASE("time to reach a fixed depth") {
        constexpr uint16_t target_depth = 7;
        uint64_t total_nps = 0;
        uint64_t total_depth = 0;

        for (const auto &position : test::POSITIONS) {
            Board board(position.fen);
            Search search(&board);
            search.set_limits();
            const SearchInfo result = search.iterative_deepening(
                target_depth, std::numeric_limits<uint32_t>::max());

            const uint64_t nps =
                result.time_ms == 0 ? 0 : result.nodes * 1000 / result.time_ms;

            print_result("fixed-depth", position, result, nps);
            REQUIRE_MESSAGE(result.depth == target_depth, position.name);
            CHECK_FALSE(result.stopped);
            total_nps += nps;
            total_depth += result.depth;
        }

        double sz = test::POSITIONS.size();
        print_benchmark("average_nps={:L} average_depth={}\n", total_nps / sz,
                        total_depth / sz);
    }

    TEST_CASE("depth reached in a fixed time") {
        constexpr uint32_t time_budget_ms = 1000;
        uint64_t total_nps = 0;
        uint64_t total_depth = 0;

        for (const auto &position : test::POSITIONS) {
            Board board(position.fen);
            Search search(&board);
            search.set_limits();
            const SearchInfo result = search.iterative_deepening(
                Search::MAX_SEARCH_DEPTH, time_budget_ms);

            const uint64_t nps =
                result.time_ms == 0 ? 0 : result.nodes * 1000 / result.time_ms;

            print_result("fixed-time", position, result, nps);
            CHECK_MESSAGE(result.depth > 0, position.name);
            CHECK(result.stopped);
            total_nps += nps;
            total_depth += result.depth;
        }

        double sz = test::POSITIONS.size();
        print_benchmark("average_nps={:L} average_depth={}\n", total_nps / sz,
                        total_depth / sz);
    }
}
