#include "bitboard.hpp"
#include "doctest/doctest.h"

TEST_SUITE("unit") {
    TEST_CASE("BitBoard basic operations (runtime)") {
        BitBoard empty_bb(BitBoard::EMPTY);
        CHECK(empty_bb.count() == 0);

        BitBoard b1(BB::B1);
        CHECK(b1.count() == 1);
        CHECK(b1.lsb_square() == SQ::B1);
        CHECK(b1.msb_square() == SQ::B1);

        BitBoard combined = BB::A1 | BB::H8;
        CHECK(combined.count() == 2);
        CHECK(combined.lsb_square() == SQ::A1);
        CHECK(combined.msb_square() == SQ::H8);

        CHECK(combined.has_square(SQ::A1));
        CHECK(combined.has_square(SQ::H8));
        CHECK(!combined.has_square(SQ::E4));

        combined.erase_square(SQ::A1);
        CHECK(combined.count() == 1);
        CHECK(!combined.has_square(SQ::A1));

        combined.set_square(SQ::C3);
        CHECK(combined.count() == 2);
        CHECK(combined.has_square(SQ::C3));

        Square popped = combined.get_square_pop();
        CHECK(popped == SQ::C3);
        CHECK(combined.count() == 1);
        CHECK(combined.has_square(SQ::H8));
    }
}
