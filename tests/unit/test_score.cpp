#include "doctest/doctest.h"
#include "score.hpp"

TEST_SUITE("unit") {
    TEST_CASE("Score wrapper functionality") {
        // Centipawn creation & basic behavior
        Score normal = Score::centipawns(150);
        CHECK(normal.raw() == 150);
        CHECK(!normal.is_mate());
        CHECK(!normal.is_draw());

        // Draw score
        Score draw = Score::draw();
        CHECK(draw.is_draw());
        CHECK(draw.raw() == 0);
        CHECK(!draw.is_mate());

        // Mate scores
        Score mate_winning = Score::mate(3); // winning mate in 3 plies
        CHECK(mate_winning.is_mate());
        CHECK(!mate_winning.is_draw());
        CHECK(mate_winning.mate_plies() == 3);
        CHECK(mate_winning.mate_moves() == 2); // (3+1)/2 = 2 moves

        Score mate_losing = Score::mate(
            -3); // losing mate in 3 plies (represented by -3 plies in factory)
        CHECK(mate_losing.is_mate());
        CHECK(!mate_losing.is_draw());
        CHECK(mate_losing.mate_plies() == 3);
        CHECK(mate_losing.mate_moves() == -2);

        // Implicit conversions and arithmetic
        Score a = 100;
        Score b = 200;
        CHECK(a + b == 300);
        CHECK(b - a == 100);
        CHECK(-a == -100);
        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= b);
        CHECK(b >= a);
        CHECK(a != b);

        // TT translations
        // e.g. winning mate in 3 plies found at search ply 2
        Score original_mate = Score::mate(3);     // 200000 - 3 = 199997
        Score stored_tt = original_mate.to_tt(2); // 199997 + 2 = 199999
        CHECK(stored_tt.raw() == 199999);

        // Probing back from TT at search ply 2
        Score retrieved = stored_tt.from_tt(2); // 199999 - 2 = 199997
        CHECK(retrieved.mate_plies() == 3);

        // e.g. losing mate in 3 plies found at search ply 2
        Score original_losing = Score::mate(-3); // -200000 - (-3) = -199997
        Score stored_tt_losing =
            original_losing.to_tt(2); // -199997 - 2 = -199999
        CHECK(stored_tt_losing.raw() == -199999);

        Score retrieved_losing =
            stored_tt_losing.from_tt(2); // -199999 + 2 = -199997
        CHECK(retrieved_losing.mate_plies() == 3);
    }
}
