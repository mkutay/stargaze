#include "bitboard.hpp"
#include "board.hpp"
#include "mask.hpp"
#include <array>
#include <vector>

std::vector<Move> Board::get_moves() {
    /**
     * Pseudolegal move generation, which include moves that leave the king in
     * check
     */
    std::vector<Move> pseudo;

    int mul = turn == Colour::WHITE ? 1 : -1;

    BitBoard white_pieces = get_bb(Colour::WHITE);
    BitBoard black_pieces = get_bb(Colour::BLACK);
    BitBoard occupied = white_pieces | black_pieces;
    BitBoard empty = ~occupied;
    BitBoard other_pieces = turn == Colour::WHITE ? black_pieces : white_pieces;

    auto process_step = [other_pieces, empty,
                         &pseudo](BitBoard &bb, int8_t move, BitBoard mask,
                                  int8_t from_displacement) {
        bb &= mask;
        bb = bb.direct_move(move);

        auto captures = bb & other_pieces;
        bb = (bb & empty) | captures;

        auto temp = bb;
        while (temp) {
            auto sq = temp.get_square_pop();
            pseudo.emplace_back(sq - from_displacement, sq,
                                Move::create_flags(captures.has_square(sq)));
        }

        bb &= empty;
    };

    auto generate_moves = [&process_step]<bool Sliding, size_t N>(
                              BitBoard bitboard,
                              const std::array<int8_t, N> &moves) {
        for (auto move : moves) {
            auto mask = Mask::moving_mask(move);
            BitBoard bb = bitboard;

            if constexpr (Sliding) {
                for (int8_t m = 0; bb; m++) {
                    process_step(bb, move, mask, (m + 1) * move);
                }
            } else {
                process_step(bb, move, mask, move);
            }
        }
    };

    // These are the final "places" of pawns after these actions.
    BitBoard pawns = get_bb(Piece::PAWN, turn), pawn_push_one = 0,
             pawn_push_two = 0, pawn_capture_left = 0, pawn_capture_right = 0,
             pawn_push_one_promotion = 0, pawn_capture_left_promotion = 0,
             pawn_capture_right_promotion = 0;

    if (turn == Colour::WHITE) {
        pawn_push_one = pawns.north() & empty;
        pawn_push_two =
            ((pawns & Mask::RANK_2).north() & empty).north() & empty;
        pawn_capture_left = pawns.north().west() & other_pieces;
        pawn_capture_right = pawns.north().east() & other_pieces;
        pawn_push_one_promotion = pawn_push_one & Mask::RANK_8;
        pawn_capture_left_promotion = pawn_capture_left & Mask::RANK_8;
        pawn_capture_right_promotion = pawn_capture_right & Mask::RANK_8;
    } else {
        pawn_push_one = pawns.south() & empty;
        pawn_push_two =
            ((pawns & Mask::RANK_7).south() & empty).south() & empty;
        pawn_capture_left = pawns.south().east() & other_pieces;
        pawn_capture_right = pawns.south().west() & other_pieces;
        pawn_push_one_promotion = pawn_push_one & Mask::RANK_1;
        pawn_capture_left_promotion = pawn_capture_left & Mask::RANK_1;
        pawn_capture_right_promotion = pawn_capture_right & Mask::RANK_1;
    }

    if (!moves.empty() && moves.back().flags() == Move::DOUBLE_PAWN_PUSH) {
        auto to = moves.back().to();
        if (pawns.west() & BitBoard(to)) {
            pseudo.emplace_back(to + 1, to + 8 * mul, Move::EN_PASSANT);
        }
        if (pawns.east() & BitBoard(to)) {
            pseudo.emplace_back(to - 1, to + 8 * mul, Move::EN_PASSANT);
        }
    }

    while (pawn_push_one) {
        auto sq = pawn_push_one.get_square_pop();
        auto from = sq - 8 * mul;
        if (pawn_push_one_promotion.has_square(sq)) {
            pseudo.emplace_back(from, sq, Move::KNIGHT_PROMOTION);
            pseudo.emplace_back(from, sq, Move::BISHOP_PROMOTION);
            pseudo.emplace_back(from, sq, Move::ROOK_PROMOTION);
            pseudo.emplace_back(from, sq, Move::QUEEN_PROMOTION);
        } else {
            pseudo.emplace_back(from, sq, Move::QUIET);
        }
    }

    while (pawn_push_two) {
        auto sq = pawn_push_two.get_square_pop();
        pseudo.emplace_back(sq - 16 * mul, sq, Move::DOUBLE_PAWN_PUSH);
    }

    while (pawn_capture_left) {
        auto sq = pawn_capture_left.get_square_pop();
        auto from = sq - 7 * mul;
        if (pawn_capture_left_promotion.has_square(sq)) {
            pseudo.emplace_back(from, sq, Move::KNIGHT_PROMOTION_CAPTURE);
            pseudo.emplace_back(from, sq, Move::BISHOP_PROMOTION_CAPTURE);
            pseudo.emplace_back(from, sq, Move::ROOK_PROMOTION_CAPTURE);
            pseudo.emplace_back(from, sq, Move::QUEEN_PROMOTION_CAPTURE);
        } else {
            pseudo.emplace_back(from, sq, Move::CAPTURE);
        }
    }

    while (pawn_capture_right) {
        auto sq = pawn_capture_right.get_square_pop();
        auto from = sq - 9 * mul;
        if (pawn_capture_right_promotion.has_square(sq)) {
            pseudo.emplace_back(from, sq, Move::KNIGHT_PROMOTION_CAPTURE);
            pseudo.emplace_back(from, sq, Move::BISHOP_PROMOTION_CAPTURE);
            pseudo.emplace_back(from, sq, Move::ROOK_PROMOTION_CAPTURE);
            pseudo.emplace_back(from, sq, Move::QUEEN_PROMOTION_CAPTURE);
        } else {
            pseudo.emplace_back(from, sq, Move::CAPTURE);
        }
    }

    auto knights = get_bb(Piece::KNIGHT, turn);
    generate_moves.operator()<false>(knights, Mask::KNIGHT_MOVES);

    auto bishops_and_queens =
        get_bb(Piece::BISHOP, turn) | get_bb(Piece::QUEEN, turn);
    generate_moves.operator()<true>(bishops_and_queens, Mask::DIAGONAL_MOVES);

    auto rooks_and_queens =
        get_bb(Piece::ROOK, turn) | get_bb(Piece::QUEEN, turn);
    generate_moves.operator()<true>(rooks_and_queens, Mask::CARDINAL_MOVES);

    auto king = get_bb(Piece::KING, turn);
    generate_moves.operator()<false>(king, Mask::CARDINAL_MOVES);
    generate_moves.operator()<false>(king, Mask::DIAGONAL_MOVES);

    std::vector<Move> non_pseudo_moves;
    for (Move move : pseudo) {
        Colour current_turn = turn;
        make_move(move);
        if (!is_in_check(current_turn))
            non_pseudo_moves.emplace_back(move);
        undo_move();
    }

    {
        auto turn_index = std::to_underlying(turn);
        auto SQE1 = SQ::E1.flip(turn), SQF1 = SQ::F1.flip(turn),
             SQG1 = SQ::G1.flip(turn), SQD1 = SQ::D1.flip(turn),
             SQC1 = SQ::C1.flip(turn);
        auto BBF1 = BB::F1.flip(turn), BBG1 = BB::G1.flip(turn),
             BBD1 = BB::D1.flip(turn), BBC1 = BB::C1.flip(turn),
             BBB1 = BB::B1.flip(turn);
        if (can_castle[turn_index * 2] && occupied.empty(BBF1 | BBG1) &&
            !is_attacked(turn, SQE1) && !is_attacked(turn, SQF1) &&
            !is_attacked(turn, SQG1)) {
            non_pseudo_moves.emplace_back(SQE1, SQG1, Move::KING_SIDE_CASTLE);
        }
        if (can_castle[turn_index * 2 + 1] &&
            occupied.empty(BBB1 | BBC1 | BBD1) && !is_attacked(turn, SQE1) &&
            !is_attacked(turn, SQD1) && !is_attacked(turn, SQC1)) {
            non_pseudo_moves.emplace_back(SQE1, SQC1, Move::QUEEN_SIDE_CASTLE);
        }
    }

    return non_pseudo_moves;
}

bool Board::is_in_check(Colour by_colour) const {
    auto king_bb = get_bb(Piece::KING, by_colour);
    return is_attacked(by_colour, king_bb);
}

bool Board::is_attacked(Colour by_colour, BitBoard bb) const {
    assert(bb.count() == 1);

    Square sq = bb.get_lsb_square();
    Colour other = !by_colour;
    auto other_queens = get_bb(Piece::QUEEN, other);
    auto other_rooks = get_bb(Piece::ROOK, other);
    auto other_bishops = get_bb(Piece::BISHOP, other);
    auto other_knights = get_bb(Piece::KNIGHT, other);
    auto other_king = get_bb(Piece::KING, other);
    auto other_pawns = get_bb(Piece::PAWN, other);
    auto occupied = get_bb(Colour::WHITE) | get_bb(Colour::BLACK);
    auto empty = ~occupied;

    if (other_king & Mask::KING_MASKS.at(sq))
        return true;

    if (by_colour == Colour::WHITE) {
        if ((bb.north().west() | bb.north().east()) & other_pawns)
            return true;
    } else {
        if ((bb.south().west() | bb.south().east()) & other_pawns)
            return true;
    }

    if (other_knights & Mask::KNIGHT_MASKS.at(sq))
        return true;

    auto diagonals = other_bishops | other_queens;
    auto cardinals = other_rooks | other_queens;

    if ((diagonals & Mask::BISHOP_MASKS.at(sq)) ||
        (cardinals & Mask::ROOK_MASKS.at(sq))) {

        auto check_attack_sliding =
            [empty, bb]<size_t N>(const std::array<int8_t, N> &moves,
                                  BitBoard attacking_pieces) {
                for (auto move : moves) {
                    auto mask = Mask::moving_mask(move);
                    auto temp = bb;
                    while (temp) {
                        temp &= mask;
                        temp = temp.direct_move(move);
                        if (temp & attacking_pieces)
                            return true;
                        temp &= empty;
                    }
                }
                return false;
            };

        if (check_attack_sliding(Mask::DIAGONAL_MOVES, diagonals) ||
            check_attack_sliding(Mask::CARDINAL_MOVES, cardinals))
            return true;
    }

    return false;
}
