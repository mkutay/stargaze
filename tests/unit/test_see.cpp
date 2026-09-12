#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "stargaze/eval.hpp"
#include "stargaze/move_picker.hpp"
#include <string_view>

namespace {
Move find_move(Board &board, std::string_view notation) {
    for (Move move : board.get_moves()) {
        if (move.to_string() == notation)
            return move;
    }
    FAIL("legal move not found: ", notation);
    return Move();
}
} // namespace

TEST_SUITE("static exchange evaluation") {
    TEST_CASE("scores undefended and defended captures") {
        Board free_queen("7k/8/8/4q3/3P4/8/8/K7 w - - 0 1");
        CHECK(MovePicker::see(free_queen, find_move(free_queen, "d4e5")) ==
              Eval::value(PP::QUEEN));

        Board defended_pawn("7k/8/5p2/4p3/3Q4/8/8/K7 w - - 0 1");
        CHECK(
            MovePicker::see(defended_pawn, find_move(defended_pawn, "d4e5")) ==
            Eval::value(PP::PAWN) - Eval::value(PP::QUEEN));
    }

    TEST_CASE("discovers x-rays after en passant") {
        Board board("3r3k/8/8/3pP3/8/8/8/K7 w - d6 0 1");
        CHECK(MovePicker::see(board, find_move(board, "e5d6")) == 0);
    }

    TEST_CASE("excludes pinned recaptures") {
        Board board("4k3/8/4p3/3p4/5N2/8/8/K3R3 w - - 0 1");
        CHECK(MovePicker::see(board, find_move(board, "f4d5")) ==
              Eval::value(PP::PAWN));
    }

    TEST_CASE("includes promotion material") {
        Board board("k6r/6P1/8/8/8/8/8/K7 w - - 0 1");
        const int expected = Eval::value(PP::ROOK) + Eval::value(PP::QUEEN) -
                             Eval::value(PP::PAWN);
        CHECK(MovePicker::see(board, find_move(board, "g7h8q")) == expected);
    }

    TEST_CASE("matches swap algorithm reference positions") {
        Board rook_takes_pawn(
            "1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1");
        CHECK(MovePicker::see(rook_takes_pawn,
                              find_move(rook_takes_pawn, "e1e5")) ==
              Eval::value(PP::PAWN));

        Board knight_takes_pawn(
            "1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - 0 1");
        CHECK(MovePicker::see(knight_takes_pawn,
                              find_move(knight_takes_pawn, "d3e5")) ==
              Eval::value(PP::PAWN) - Eval::value(PP::KNIGHT));
    }
}
