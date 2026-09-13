#include "doctest/doctest.h"
#include "stargaze/search_history.hpp"

TEST_SUITE("search history") {
    TEST_CASE("continuation scores depend on the previous move") {
        SearchHistory history;
        const auto previous =
            SearchHistory::context(CC::BLACK, PP::KNIGHT, SQ::F6);
        const auto current =
            SearchHistory::context(CC::WHITE, PP::PAWN, SQ::E4);

        history.update(current, 100, std::nullopt);
        CHECK(history.score(current, std::nullopt) == 100);
        CHECK(history.score(current, previous) == 100);

        history.update(current, 200, previous);
        CHECK(history.score(current, previous) >
              history.score(current, std::nullopt));
    }

    TEST_CASE("cutoffs record a countermove") {
        SearchHistory history;
        const auto previous =
            SearchHistory::context(CC::BLACK, PP::PAWN, SQ::E5);
        const auto current =
            SearchHistory::context(CC::WHITE, PP::KNIGHT, SQ::F3);
        const Move move(SQ::G1, SQ::F3, Move::QUIET);

        CHECK_FALSE(history.countermove(std::nullopt));
        CHECK_FALSE(history.countermove(previous));
        history.record_cutoff(current, previous, move, 100);
        CHECK(history.countermove(previous) == move);
    }

    TEST_CASE("gravity updates remain bounded") {
        SearchHistory history;
        const auto context =
            SearchHistory::context(CC::WHITE, PP::BISHOP, SQ::C4);

        for (int i = 0; i < 100; i++)
            history.update(context, SearchHistory::MAX_SCORE, std::nullopt);
        CHECK(history.score(context, std::nullopt) == SearchHistory::MAX_SCORE);

        for (int i = 0; i < 100; i++)
            history.update(context, -SearchHistory::MAX_SCORE, std::nullopt);
        CHECK(history.score(context, std::nullopt) ==
              -SearchHistory::MAX_SCORE);
    }
}
