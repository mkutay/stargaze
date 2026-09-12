#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "stargaze/score.hpp"
#include "stargaze/search.hpp"
#include "test_helpers.hpp"
#include <cstdint>
#include <limits>

TEST_SUITE("search") {
    TEST_CASE("tight limits retain a legal fallback move") {
        Board board(test::START_POSITION);
        Search search(&board);
        search.resize_tt(1);

        search.set_limits(1);
        SearchInfo nodes =
            search.iterative_deepening(4, std::numeric_limits<uint32_t>::max());
        CHECK(nodes.stopped);
        CHECK(nodes.nodes == 1);
        REQUIRE_FALSE(nodes.pv.moves.empty());

        search.set_limits();
        SearchInfo time = search.iterative_deepening(4, 0);
        CHECK(time.stopped);
        REQUIRE_FALSE(time.pv.moves.empty());
    }

    TEST_CASE("root checkmate is losing for the side to move") {
        Board board("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
        Search search(&board);
        search.resize_tt(1);

        SearchInfo result =
            search.iterative_deepening(1, std::numeric_limits<uint32_t>::max());
        CHECK(result.score.is_mate());
        CHECK(result.score.raw() < 0);
        CHECK(result.pv.moves.empty());

        board = Board("7k/6Q1/6K1/8/8/8/8/8 b - - 100 1");
        result =
            search.iterative_deepening(1, std::numeric_limits<uint32_t>::max());
        CHECK(result.score.is_mate());
        CHECK(result.score.raw() < 0);
    }

    TEST_CASE("drawn roots retain a legal move") {
        Board board("7k/8/8/8/8/8/8/1B5K w - - 0 1");
        Search search(&board);
        search.resize_tt(1);

        SearchInfo result =
            search.iterative_deepening(1, std::numeric_limits<uint32_t>::max());
        CHECK(result.score.is_draw());
        REQUIRE_FALSE(result.pv.moves.empty());
    }

    TEST_CASE("quiescence searches quiet check evasions") {
        Board board("7k/8/8/8/8/8/8/R5K1 w - - 0 1");
        Search search(&board);
        search.resize_tt(1);

        SearchInfo result =
            search.iterative_deepening(1, std::numeric_limits<uint32_t>::max());
        CHECK(result.score.raw() < Score::INFINITY_SCORE);
        CHECK(result.score.raw() > -Score::INFINITY_SCORE);
    }

    TEST_CASE("transposition scores do not cross halfmove contexts") {
        Board board("7k/8/8/8/8/8/8/R5K1 w - - 0 1");
        Search search(&board);
        search.resize_tt(1);

        SearchInfo fresh =
            search.iterative_deepening(2, std::numeric_limits<uint32_t>::max());
        CHECK_FALSE(fresh.score.is_draw());

        board = Board("7k/8/8/8/8/8/8/R5K1 w - - 99 1");
        SearchInfo near_draw =
            search.iterative_deepening(2, std::numeric_limits<uint32_t>::max());
        CHECK(near_draw.score.is_draw());
    }
}
