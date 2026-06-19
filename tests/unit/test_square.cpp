#include "doctest/doctest.h"
#include "square.hpp"

TEST_SUITE("unit") {
    TEST_CASE("Square basic operations") {
        CHECK(Square("e4").to_string() == "e4");
        CHECK(Square("a1").to_string() == "a1");
        CHECK(Square("h8").to_string() == "h8");

        CHECK(SQ::E4.north() == SQ::E5);
        CHECK(SQ::E4.south() == SQ::E3);
        CHECK(SQ::E4.east() == SQ::F4);
        CHECK(SQ::E4.west() == SQ::D4);
    }
}
