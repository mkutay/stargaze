#include "doctest/doctest.h"
#include "move.hpp"
#include "piece.hpp"
#include "square.hpp"

TEST_SUITE("unit") {
    TEST_CASE("Move basic encoding and decoding") {
        Move m1(SQ::E2, SQ::E4, Move::DOUBLE_PAWN_PUSH);
        CHECK(m1.from() == SQ::E2);
        CHECK(m1.to() == SQ::E4);
        CHECK(m1.flags() == Move::DOUBLE_PAWN_PUSH);
        CHECK(m1.is_double_pawn_push());
        CHECK(!m1.is_capture());
        CHECK(!m1.is_promotion());
        CHECK(m1.to_string() == "e2e4");

        Move m2(SQ::D7, SQ::C8, Move::QUEEN_PROMOTION_CAPTURE);
        CHECK(m2.from() == SQ::D7);
        CHECK(m2.to() == SQ::C8);
        CHECK(m2.flags() == Move::QUEEN_PROMOTION_CAPTURE);
        CHECK(m2.is_capture());
        CHECK(m2.is_promotion());
        CHECK(m2.promotion_piece() == PP::QUEEN);
        CHECK(m2.to_string() == "d7c8q");

        Move m3(SQ::E5, SQ::F6, Move::EN_PASSANT);
        CHECK(m3.from() == SQ::E5);
        CHECK(m3.to() == SQ::F6);
        CHECK(m3.flags() == Move::EN_PASSANT);
        CHECK(m3.is_capture());
        CHECK(m3.is_en_passant());
        CHECK(m3.to_string() == "e5f6");
    }
}
