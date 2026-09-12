#include "doctest/doctest.h"
#include "stargaze/board.hpp"
#include "test_helpers.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace {
void sort_moves(std::vector<Move> &moves) {
    std::ranges::sort(
        moves, [](Move a, Move b) { return a.to_string() < b.to_string(); });
}

template <typename Predicate>
std::vector<Move> matching_moves(const std::vector<Move> &moves,
                                 Predicate predicate) {
    std::vector<Move> matching;
    std::ranges::copy_if(moves, std::back_inserter(matching), predicate);
    sort_moves(matching);
    return matching;
}

std::vector<Move> checking_moves(Board &board, const std::vector<Move> &moves) {
    std::vector<Move> checks;
    for (Move move : moves) {
        board.make_move(move);
        if (board.in_check())
            checks.push_back(move);
        board.undo_move();
    }
    sort_moves(checks);
    return checks;
}

void check_same_moves(std::vector<Move> actual, std::vector<Move> expected) {
    sort_moves(actual);
    sort_moves(expected);
    CHECK(actual == expected);
}
} // namespace

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

        Board board2(test::KIWIPETE);
        verify_make_undo_recursive(board2, 2);

        Board board3(test::CASTLING_EP_POSITION);
        verify_make_undo_recursive(board3, 2);
    }

    TEST_CASE("Zobrist and Eval incremental vs scratch") {
        Board board;
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

    TEST_CASE("Move generation categories") {
        Board board(test::MOVE_GEN_CATEGORIES);

        auto captures = board.get_moves<true, false, false, false, false>();
        CHECK(std::ranges::all_of(captures,
                                  [](Move move) { return move.is_capture(); }));

        auto promotions = board.get_moves<false, false, false, true, false>();
        CHECK(promotions.size() == 4);
        CHECK(std::ranges::all_of(
            promotions, [](Move move) { return move.is_promotion(); }));

        auto king_moves = board.get_moves<false, true, false, false, false>();
        CHECK(std::ranges::all_of(
            king_moves, [](Move move) { return move.from() == SQ::E1; }));

        auto tactical = board.get_moves<true, false, false, true, false>();
        CHECK(std::ranges::all_of(tactical, [](Move move) {
            return move.is_capture() || move.is_promotion();
        }));
    }

    TEST_CASE("Move generation emitters complete or stop on request") {
        Board board;
        const auto expected = board.get_moves();

        std::vector<Move> emitted;
        const bool completed =
            board.generate_moves([&](Move move) { emitted.push_back(move); });
        CHECK(completed);
        CHECK(emitted == expected);

        int emitted_before_stop = 0;
        const bool stopped = board.generate_moves([&](Move) {
            emitted_before_stop++;
            return false;
        });
        CHECK_FALSE(stopped);
        CHECK(emitted_before_stop == 1);
    }

    TEST_CASE("Legal move existence stops without collecting moves") {
        Board initial;
        CHECK(initial.has_legal_move());
        CHECK(initial.get_move() == initial.get_moves().front());

        Board checkmate("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
        CHECK(checkmate.in_check());
        CHECK_FALSE(checkmate.has_legal_move());
        CHECK_FALSE(checkmate.get_move().has_value());

        Board stalemate("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        CHECK_FALSE(stalemate.in_check());
        CHECK_FALSE(stalemate.has_legal_move());
    }

    TEST_CASE("Checking move generation") {
        Board direct(test::DIRECT_ROOK_CHECK);
        auto direct_checks =
            direct.get_moves<false, false, true, false, false>();
        CHECK(std::ranges::any_of(direct_checks, [](Move move) {
            return move.to_string() == "a1a8";
        }));

        Board discovered(test::DISCOVERED_ROOK_CHECK);
        auto discovered_checks =
            discovered.get_moves<false, false, true, false, false>();
        CHECK(std::ranges::any_of(discovered_checks, [](Move move) {
            return move.to_string() == "e2f3";
        }));

        Board promotion(test::PROMOTION_CHECKS);
        auto promotion_checks =
            promotion.get_moves<false, false, true, false, false>();
        CHECK(std::ranges::any_of(promotion_checks, [](Move move) {
            return move.to_string() == "g7g8r";
        }));
        CHECK(std::ranges::any_of(promotion_checks, [](Move move) {
            return move.to_string() == "g7g8q";
        }));
    }

    TEST_CASE("Move generation categories match full legal move set") {
        for (const auto fen : test::MOVE_GEN_FENS) {
            Board board(fen);
            auto all = board.get_moves();

            auto captures = matching_moves(
                all, [](Move move) { return move.is_capture(); });
            auto promotions = matching_moves(
                all, [](Move move) { return move.is_promotion(); });
            auto quiets = matching_moves(all, [](Move move) {
                return !move.is_capture() && !move.is_promotion();
            });
            auto king = matching_moves(all, [&board](Move move) {
                return board.get_piece(move.from()) == PP::KING;
            });
            auto checks = checking_moves(board, all);

            check_same_moves(
                board.get_moves<true, false, false, false, false>(), captures);
            check_same_moves(
                board.get_moves<false, true, false, false, false>(), king);
            check_same_moves(
                board.get_moves<false, false, true, false, false>(), checks);
            check_same_moves(
                board.get_moves<false, false, false, true, false>(),
                promotions);
            check_same_moves(
                board.get_moves<false, false, false, false, true>(), quiets);

            auto tactical = matching_moves(all, [](Move move) {
                return move.is_capture() || move.is_promotion();
            });
            check_same_moves(board.get_moves<true, false, false, true, false>(),
                             tactical);

            auto forcing = matching_moves(all, [&](Move move) {
                return move.is_capture() || move.is_promotion() ||
                       std::ranges::find(checks, move) != checks.end();
            });
            check_same_moves(board.get_moves<true, false, true, true, false>(),
                             forcing);

            check_same_moves(board.get_moves<true, true, true, true, true>(),
                             all);
        }
    }

    TEST_CASE("Dead positions are draws") {
        CHECK(Board("7k/8/8/8/8/8/8/7K w - - 0 1").is_draw());
        CHECK(Board("7k/8/8/8/8/8/8/1B5K w - - 0 1").is_draw());
        CHECK(Board("7k/8/8/8/8/8/8/1N5K w - - 0 1").is_draw());
        CHECK(Board("2b4k/8/8/8/8/8/8/1B5K w - - 0 1").is_draw());
        CHECK_FALSE(Board("7k/8/8/8/8/8/8/1R5K w - - 0 1").is_draw());
    }

    TEST_CASE("Null moves do not advance draw counters") {
        Board board("7k/8/8/8/3pP3/8/8/R6K w - d6 99 1");
        const std::string fen = board.fen();
        const uint64_t hash = board.get_hash();

        board.make_null_move();
        CHECK(board.get_halfmove_clock() == 99);
        CHECK_FALSE(board.is_draw());
        const uint64_t null_hash = board.get_hash();
        const auto replies = board.get_moves();
        REQUIRE_FALSE(replies.empty());
        board.make_move(replies.front());
        board.undo_move();
        CHECK(board.get_hash() == null_hash);
        board.undo_null_move();

        CHECK(board.fen() == fen);
        CHECK(board.get_hash() == hash);
    }
}
