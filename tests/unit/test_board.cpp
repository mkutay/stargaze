#include "board.hpp"
#include "doctest/doctest.h"
#include "test_helpers.hpp"
#include <string>
#include <vector>

void verify_make_undo_recursive(Board &board, int depth) {
    if (depth == 0)
        return;

    std::string fen_before = board.fen();
    uint64_t hash_before = board.get_hash();
    auto castling_before = board.get_castling_rights();
    auto turn_before = board.get_turn();

    std::vector<Move> moves = board.get_moves();
    for (Move move : moves) {
        board.make_move(move);

        verify_make_undo_recursive(board, depth - 1);

        board.undo_move();

        // Check invariants are completely restored:
        CHECK(board.fen() == fen_before);
        CHECK(board.get_hash() == hash_before);
        CHECK(board.get_castling_rights() == castling_before);
        CHECK(board.get_turn() == turn_before);
    }
}

TEST_SUITE("unit") {
    TEST_CASE("Board make/undo invariants recursive") {
        Board board1;
        verify_make_undo_recursive(board1, 3);

        // Kiwipete position to depth 2 (large branching factor).
        Board board2(test::KIWIPETE);
        verify_make_undo_recursive(board2, 2);

        // Castling and en-passant rich position.
        Board board3(test::CASTLING_EP_POSITION);
        verify_make_undo_recursive(board3, 2);
    }

    TEST_CASE("Zobrist and Eval incremental vs scratch") {
        Board board;
        // Simulating a sequence of moves
        std::vector<std::string> moves_seq = {"e2e4", "e7e5", "g1f3", "b8c6",
                                              "f1c4", "g8f6", "d2d3"};

        for (const auto &m_str : moves_seq) {
            auto legal = board.get_moves();
            bool found = false;
            for (Move m : legal) {
                if (m.to_string() == m_str) {
                    board.make_move(m);
                    found = true;
                    break;
                }
            }
            REQUIRE(found);

            // Reconstruct board from FEN to force recalculating evaluation and
            // hash from scratch
            Board scratch_board(board.fen());
            CHECK(board.get_hash() == scratch_board.get_hash());
            CHECK(board.evaluate() == scratch_board.evaluate());
        }
    }
}
