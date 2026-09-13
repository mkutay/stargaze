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

    TEST_CASE("cluster retains colliding positions") {
        TT tt(1);
        const Move move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH);

        for (uint64_t i = 1; i <= 2; i++)
            tt.store(i << 32, move, Score(static_cast<int>(i)), i, 0,
                     Bound::LOWER);

        for (uint64_t i = 1; i <= 2; i++) {
            TTEntry *entry = tt.probe(i << 32);
            REQUIRE(entry != nullptr);
            CHECK(entry->score == Score(static_cast<int>(i)));
        }
    }

    TEST_CASE("replacement protects deeper cluster entries") {
        TT tt(1);
        const Move move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH);

        for (uint64_t i = 1; i <= TTCluster::SIZE; i++)
            tt.store(i << 32, move, Score(0), 8, 0, Bound::LOWER);

        constexpr uint64_t overflow_hash =
            static_cast<uint64_t>(TTCluster::SIZE + 1) << 32;
        tt.store(overflow_hash, move, Score(0), 1, 0, Bound::UPPER);
        CHECK(tt.probe(overflow_hash) == nullptr);

        tt.store(overflow_hash, move, Score(0), 12, 0, Bound::EXACT);
        CHECK(tt.probe(overflow_hash) != nullptr);
    }

    TEST_CASE("qsearch result does not replace a deep entry") {
        TT tt(1);
        constexpr uint64_t hash = 0x12345678;
        const Move deep_move(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH);

        tt.store(hash, deep_move, Score(42), 12, 0, Bound::LOWER);
        tt.store(hash, std::nullopt, Score(10), 0, 0, Bound::EXACT);

        TTEntry *entry = tt.probe(hash);
        REQUIRE(entry != nullptr);
        CHECK(entry->depth == 12);
        CHECK(entry->score == Score(42));
        CHECK(entry->best_move == deep_move);
    }

    TEST_CASE("hashfull only counts the current generation") {
        TT tt(1);
        CHECK(tt.hashfull() == 0);

        tt.new_search();
        tt.store(1ull << 32, std::nullopt, Score(0), 0, 0, Bound::UPPER);
        CHECK(tt.hashfull() > 0);

        tt.new_search();
        CHECK(tt.hashfull() == 0);
        CHECK(tt.probe(1ull << 32) != nullptr);
        CHECK(tt.hashfull() > 0);
    }
}
