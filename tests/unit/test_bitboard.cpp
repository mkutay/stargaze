#include "bitboard.hpp"
#include "doctest/doctest.h"

TEST_SUITE("unit") {
    TEST_CASE("BitBoard basic operations") {
        BitBoard empty_bb(BitBoard::EMPTY);
        CHECK(empty_bb.count() == 0);

        BitBoard b1(BB::B1);
        CHECK(b1.count() == 1);
        CHECK(b1.get_lsb_square() == SQ::B1);
        CHECK(b1.get_msb_square() == SQ::B1);

        BitBoard combined = BB::A1 | BB::H8;
        CHECK(combined.count() == 2);
        CHECK(combined.get_lsb_square() == SQ::A1);
        CHECK(combined.get_msb_square() == SQ::H8);

        CHECK(combined.has_square(SQ::A1));
        CHECK(combined.has_square(SQ::H8));
        CHECK(!combined.has_square(SQ::E4));

        combined.erase_bit(SQ::A1);
        CHECK(combined.count() == 1);
        CHECK(!combined.has_square(SQ::A1));

        combined.set_bit(SQ::C3);
        CHECK(combined.count() == 2);
        CHECK(combined.has_square(SQ::C3));

        Square popped = combined.get_square_pop();
        CHECK(popped == SQ::C3);
        CHECK(combined.count() == 1);
        CHECK(combined.has_square(SQ::H8));
    }

    TEST_CASE("BitBoard shifting and flipping") {
        CHECK(BB::E4.north() == BB::E5);
        CHECK(BB::E4.south() == BB::E3);
        CHECK(BB::E4.east() == BB::F4);
        CHECK(BB::E4.west() == BB::D4);

        // Border shifts
        CHECK(BB::H4.east() == BitBoard::EMPTY);
        CHECK(BB::A4.west() == BitBoard::EMPTY);
        CHECK(BB::E8.north() == BitBoard::EMPTY);
        CHECK(BB::E1.south() == BitBoard::EMPTY);

        // Flip vertically depending on color
        CHECK(BB::A1.flip(Colour::WHITE) == BB::A1);
        CHECK(BB::A1.flip(Colour::BLACK) == BB::A8);
        CHECK(BB::E4.flip(Colour::WHITE) == BB::E4);
        CHECK(BB::E4.flip(Colour::BLACK) == BB::E5);
    }
}
