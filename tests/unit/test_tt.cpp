#include "doctest/doctest.h"
#include "stargaze/move.hpp"
#include "stargaze/tt.hpp"

TEST_SUITE("transposition table") {
    TEST_CASE("halfmove context replaces an entry for the same position") {
        TT tt(1);
        constexpr uint64_t hash = 0x12345678;
        const Move first(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH);
        const Move second(SQ::D2, SQ::D4, Move::DOUBLE_PAWN_PUSH);

        tt.store(hash, first, Score(10), 8, 0, Bound::EXACT);
        TTEntry *entry = tt.probe(hash);
        REQUIRE(entry != nullptr);
        CHECK(entry->halfmove_clock == 0);

        tt.store(hash, second, Score(20), 1, 99, Bound::EXACT);
        entry = tt.probe(hash);
        REQUIRE(entry != nullptr);
        CHECK(entry->halfmove_clock == 99);
        CHECK(entry->best_move == second);
        CHECK(entry->score == Score(20));
    }
}
