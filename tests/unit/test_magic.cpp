#include "doctest/doctest.h"
#include "magic.hpp"
#include "mask.hpp"

TEST_SUITE("unit") {
    TEST_CASE("Magic Bitboard attacks and ray calculations") {
        // 1. Test rook attacks
        Square d4 = SQ::D4;
        BitBoard rook_empty = Magic::rook_attacks(d4, BitBoard(0ULL));
        // Should contain all squares on D-file and 4th rank (except D4 itself)
        BitBoard expected_rook_empty = (Mask::ROOK_MASKS.at(d4.raw()) | BB::D1 |
                                        BB::D8 | BB::A4 | BB::H4) &
                                       ~BitBoard(d4);
        CHECK(rook_empty == expected_rook_empty);

        // Rook on D4 blocked on D2 and F4
        BitBoard blockers = BitBoard(SQ::D2) | BitBoard(SQ::F4);
        BitBoard rook_blocked = Magic::rook_attacks(d4, blockers);
        CHECK(rook_blocked.has_square(SQ::D3));
        CHECK(rook_blocked.has_square(SQ::D2));
        CHECK(!rook_blocked.has_square(SQ::D1));
        CHECK(rook_blocked.has_square(SQ::E4));
        CHECK(rook_blocked.has_square(SQ::F4));
        CHECK(!rook_blocked.has_square(SQ::G4));
        CHECK(!rook_blocked.has_square(SQ::H4));

        // 2. Test bishop attacks
        BitBoard bishop_empty = Magic::bishop_attacks(d4, BitBoard(0ULL));
        BitBoard expected_bishop_empty = (Mask::BISHOP_MASKS.at(d4.raw()) |
                                          BB::A1 | BB::H8 | BB::G1 | BB::A7) &
                                         ~BitBoard(d4);
        CHECK(bishop_empty == expected_bishop_empty);

        // Bishop on D4 blocked on F6 and C3
        BitBoard bishop_blockers = BitBoard(SQ::F6) | BitBoard(SQ::C3);
        BitBoard bishop_blocked = Magic::bishop_attacks(d4, bishop_blockers);
        CHECK(bishop_blocked.has_square(SQ::E5));
        CHECK(bishop_blocked.has_square(SQ::F6));
        CHECK(!bishop_blocked.has_square(SQ::G7));
        CHECK(!bishop_blocked.has_square(SQ::H8));
        CHECK(bishop_blocked.has_square(SQ::C3));
        CHECK(!bishop_blocked.has_square(SQ::B2));
        CHECK(!bishop_blocked.has_square(SQ::A1));

        // 3. Test RAY_BETWEEN
        BitBoard ray_a1_a8 = Magic::RAY_BETWEEN[SQ::A1.raw()][SQ::A8.raw()];
        BitBoard expected_a1_a8 =
            BB::A2 | BB::A3 | BB::A4 | BB::A5 | BB::A6 | BB::A7;
        CHECK(ray_a1_a8 == expected_a1_a8);

        BitBoard ray_a1_h8 = Magic::RAY_BETWEEN[SQ::A1.raw()][SQ::H8.raw()];
        BitBoard expected_a1_h8 =
            BB::B2 | BB::C3 | BB::D4 | BB::E5 | BB::F6 | BB::G7;
        CHECK(ray_a1_h8 == expected_a1_h8);
        CHECK(Magic::RAY_BETWEEN[SQ::A1.raw()][SQ::B3.raw()] == 0ULL);
        CHECK(Magic::RAY_BETWEEN[SQ::A1.raw()][SQ::A2.raw()] == 0ULL);
    }
}
