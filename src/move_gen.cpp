#include "bitboard.hpp"
#include "board.hpp"
#include "colour.hpp"
#include "magic.hpp"
#include "mask.hpp"
#include "piece.hpp"
#include <array>
#include <vector>

template <bool CapturesOnly> std::vector<Move> Board::get_moves() {
    std::vector<Move> legal_moves;

    Colour us = turn;
    Colour them = !turn;
    BitBoard own_pieces = get_bb(us);
    BitBoard other_pieces = get_bb(them);
    BitBoard occupied = own_pieces | other_pieces;

    Square king_sq = get_bb(PP::KING, us).get_lsb_square();

    // King Danger Map (squares attacked by the opponent, with king removed
    // from occupied)
    BitBoard occupied_no_king = occupied ^ BitBoard(king_sq);
    BitBoard king_danger = 0;

    BitBoard opponent_pawns = get_bb(PP::PAWN, them);
    if (them == CC::WHITE) {
        king_danger |=
            opponent_pawns.north().west() | opponent_pawns.north().east();
    } else {
        king_danger |=
            opponent_pawns.south().west() | opponent_pawns.south().east();
    }

    BitBoard opponent_knights = get_bb(PP::KNIGHT, them);
    while (opponent_knights) {
        Square sq = opponent_knights.get_square_pop();
        king_danger |= Mask::KNIGHT_MASKS.at(sq);
    }

    Square opponent_king_sq = get_bb(PP::KING, them).get_lsb_square();
    king_danger |= Mask::KING_MASKS.at(opponent_king_sq);

    BitBoard opponent_bishops =
        get_bb(PP::BISHOP, them) | get_bb(PP::QUEEN, them);
    while (opponent_bishops) {
        Square sq = opponent_bishops.get_square_pop();
        king_danger |= Magic::bishop_attacks(sq, occupied_no_king);
    }

    BitBoard opponent_rooks = get_bb(PP::ROOK, them) | get_bb(PP::QUEEN, them);
    while (opponent_rooks) {
        Square sq = opponent_rooks.get_square_pop();
        king_danger |= Magic::rook_attacks(sq, occupied_no_king);
    }

    auto emit_moves = [&legal_moves, other_pieces](Square from,
                                                   BitBoard targets) {
        while (targets) {
            Square to = targets.get_square_pop();
            bool capture = other_pieces.has_square(to);
            legal_moves.emplace_back(from, to, Move::create_flags(capture));
        }
    };

    BitBoard king_targets =
        Mask::KING_MASKS.at(king_sq) & ~own_pieces & ~king_danger;
    if constexpr (CapturesOnly) {
        king_targets &= other_pieces;
    }
    emit_moves(king_sq, king_targets);

    // find checkers of the king
    BitBoard checkers = 0;
    if (us == CC::WHITE) {
        BitBoard king_bb = get_bb(PP::KING, us);
        checkers |= (king_bb.north().west() | king_bb.north().east()) &
                    get_bb(PP::PAWN, them);
    } else {
        BitBoard king_bb = get_bb(PP::KING, us);
        checkers |= (king_bb.south().west() | king_bb.south().east()) &
                    get_bb(PP::PAWN, them);
    }
    checkers |= Mask::KNIGHT_MASKS.at(king_sq) & get_bb(PP::KNIGHT, them);
    checkers |= Magic::bishop_attacks(king_sq, occupied) &
                (get_bb(PP::BISHOP, them) | get_bb(PP::QUEEN, them));
    checkers |= Magic::rook_attacks(king_sq, occupied) &
                (get_bb(PP::ROOK, them) | get_bb(PP::QUEEN, them));

    int num_checkers = checkers.count();

    // double check: only king moves are legal
    if (num_checkers >= 2) {
        return legal_moves;
    }

    BitBoard check_mask = BitBoard(BitBoard::ALL_SQUARES);
    if (num_checkers == 1) {
        Square checker_sq = checkers.get_lsb_square();
        Piece checker_type = *get_piece(checker_sq);
        if (checker_type == PP::BISHOP || checker_type == PP::ROOK ||
            checker_type == PP::QUEEN) {
            check_mask =
                Magic::RAY_BETWEEN[king_sq][checker_sq] | BitBoard(checker_sq);
        } else {
            check_mask = BitBoard(checker_sq);
        }
    }

    // pinned pieces and pin masks
    std::array<BitBoard, 64> pin_mask;
    pin_mask.fill(BitBoard::ALL_SQUARES);

    BitBoard pinned_pieces = 0;

    BitBoard pinners = (Mask::BISHOP_MASKS.at(king_sq) &
                        (get_bb(PP::BISHOP, them) | get_bb(PP::QUEEN, them))) |
                       (Mask::ROOK_MASKS.at(king_sq) &
                        (get_bb(PP::ROOK, them) | get_bb(PP::QUEEN, them)));

    while (pinners) {
        Square pinner_sq = pinners.get_square_pop();
        BitBoard between = Magic::RAY_BETWEEN[king_sq][pinner_sq];
        BitBoard pieces_between = between & occupied;
        if (pieces_between.count() == 1) {
            BitBoard own_pinned = pieces_between & own_pieces;
            if (own_pinned) {
                Square pinned_sq = own_pinned.get_lsb_square();
                pin_mask[pinned_sq] = between | BitBoard(pinner_sq);
                pinned_pieces |= own_pinned;
            }
        }
    }

    // 7. Pinned pieces cannot move when in check
    BitBoard active_own = own_pieces;
    if (num_checkers > 0) {
        active_own &= ~pinned_pieces;
    }

    int mul = us.weight();
    BitBoard movable_mask = check_mask;

    // Pawns
    {
        BitBoard pawns = get_bb(PP::PAWN, turn) & active_own;
        while (pawns) {
            Square from = pawns.get_square_pop();
            BitBoard pin_m = pin_mask[from];
            BitBoard target_mask = movable_mask & pin_m;

            Square from_rel = from.flip(turn);

            // Push one
            Square push1_rel = from_rel + 8;
            Square push1_to = push1_rel.flip(turn);
            if (!occupied.has_square(push1_to) &&
                target_mask.has_square(push1_to)) {
                if (push1_rel.rank() == 7) { // Promotion rank
                    for (auto promo : Move::PROMOTION_PIECES) {
                        legal_moves.emplace_back(from, push1_to, promo);
                    }
                } else if constexpr (!CapturesOnly) {
                    legal_moves.emplace_back(from, push1_to, Move::QUIET);
                }
            }

            // Push two
            if (from_rel.rank() == 1) { // Starting rank
                Square push2_rel = from_rel + 16;
                Square push2_to = push2_rel.flip(turn);
                if (!occupied.has_square(push1_to) &&
                    !occupied.has_square(push2_to) &&
                    target_mask.has_square(push2_to)) {
                    if constexpr (!CapturesOnly) {
                        legal_moves.emplace_back(from, push2_to,
                                                 Move::DOUBLE_PAWN_PUSH);
                    }
                }
            }

            // Left capture
            if (from_rel.file() > 0) {
                Square cap_rel = from_rel + 7;
                Square cap_to = cap_rel.flip(turn);
                if (other_pieces.has_square(cap_to) &&
                    target_mask.has_square(cap_to)) {
                    if (cap_rel.rank() == 7) {
                        for (auto promo : Move::PROMOTION_CAPTURE_PIECES) {
                            legal_moves.emplace_back(from, cap_to, promo);
                        }
                    } else {
                        legal_moves.emplace_back(from, cap_to, Move::CAPTURE);
                    }
                }
            }

            // Right capture
            if (from_rel.file() < 7) {
                Square cap_rel = from_rel + 9;
                Square cap_to = cap_rel.flip(turn);
                if (other_pieces.has_square(cap_to) &&
                    target_mask.has_square(cap_to)) {
                    if (cap_rel.rank() == 7) {
                        for (auto promo : Move::PROMOTION_CAPTURE_PIECES) {
                            legal_moves.emplace_back(from, cap_to, promo);
                        }
                    } else {
                        legal_moves.emplace_back(from, cap_to, Move::CAPTURE);
                    }
                }
            }

            // En Passant
            if (ep_square) {
                Square ep = *ep_square;
                Square ep_rel = ep.flip(turn);
                Square captured_pawn_sq = ep - 8 * mul;

                bool can_ep = (from_rel.file() > 0 && ep_rel == from_rel + 7) ||
                              (from_rel.file() < 7 && ep_rel == from_rel + 9);

                if (can_ep) {
                    if (movable_mask.has_square(captured_pawn_sq) &&
                        pin_m.has_square(ep)) {
                        bool ep_legal = true;
                        BitBoard occupied_after = occupied;
                        occupied_after.erase_bit(from);
                        occupied_after.erase_bit(captured_pawn_sq);
                        occupied_after.set_bit(ep);

                        BitBoard sliders =
                            get_bb(PP::ROOK, !turn) | get_bb(PP::QUEEN, !turn);
                        if (Magic::rook_attacks(king_sq, occupied_after) &
                            sliders) {
                            ep_legal = false;
                        }
                        BitBoard diag_sliders = get_bb(PP::BISHOP, !turn) |
                                                get_bb(PP::QUEEN, !turn);
                        if (Magic::bishop_attacks(king_sq, occupied_after) &
                            diag_sliders) {
                            ep_legal = false;
                        }

                        if (ep_legal) {
                            legal_moves.emplace_back(from, ep,
                                                     Move::EN_PASSANT);
                        }
                    }
                }
            }
        }
    }

    // Knights
    {
        BitBoard knights = get_bb(PP::KNIGHT, turn) & active_own;
        while (knights) {
            Square from = knights.get_square_pop();
            BitBoard targets = Mask::KNIGHT_MASKS.at(from) & ~own_pieces &
                               movable_mask & pin_mask[from];
            if constexpr (CapturesOnly) {
                targets &= other_pieces;
            }
            emit_moves(from, targets);
        }
    }

    // Bishops
    {
        BitBoard bishops = get_bb(PP::BISHOP, turn) & active_own;
        while (bishops) {
            Square from = bishops.get_square_pop();
            BitBoard targets = Magic::bishop_attacks(from, occupied) &
                               ~own_pieces & movable_mask & pin_mask[from];
            if constexpr (CapturesOnly) {
                targets &= other_pieces;
            }
            emit_moves(from, targets);
        }
    }

    // Rooks
    {
        BitBoard rooks = get_bb(PP::ROOK, turn) & active_own;
        while (rooks) {
            Square from = rooks.get_square_pop();
            BitBoard targets = Magic::rook_attacks(from, occupied) &
                               ~own_pieces & movable_mask & pin_mask[from];
            if constexpr (CapturesOnly) {
                targets &= other_pieces;
            }
            emit_moves(from, targets);
        }
    }

    // Queens
    {
        BitBoard queens = get_bb(PP::QUEEN, turn) & active_own;
        while (queens) {
            Square from = queens.get_square_pop();
            BitBoard targets = (Magic::bishop_attacks(from, occupied) |
                                Magic::rook_attacks(from, occupied)) &
                               ~own_pieces & movable_mask & pin_mask[from];
            if constexpr (CapturesOnly) {
                targets &= other_pieces;
            }
            emit_moves(from, targets);
        }
    }

    // Castling
    if constexpr (!CapturesOnly) {
        if (num_checkers == 0) {
            auto SQE1 = SQ::E1.flip(turn), SQG1 = SQ::G1.flip(turn),
                 SQC1 = SQ::C1.flip(turn);
            auto BBF1 = BB::F1.flip(turn), BBG1 = BB::G1.flip(turn),
                 BBD1 = BB::D1.flip(turn), BBC1 = BB::C1.flip(turn),
                 BBB1 = BB::B1.flip(turn);

            // King side castling
            if (can_castle[turn * 2] && occupied.empty(BBF1 | BBG1) &&
                king_danger.empty(BitBoard(SQE1) | BBF1 | BitBoard(SQG1))) {
                legal_moves.emplace_back(SQE1, SQG1, Move::KING_SIDE_CASTLE);
            }

            // Queen side castling
            if (can_castle[turn * 2 + 1] &&
                occupied.empty(BBB1 | BBC1 | BBD1) &&
                king_danger.empty(BitBoard(SQE1) | BBD1 | BitBoard(SQC1))) {
                legal_moves.emplace_back(SQE1, SQC1, Move::QUEEN_SIDE_CASTLE);
            }
        }
    }

    return legal_moves;
}

bool Board::is_in_check(Colour by_colour) const {
    auto king_bb = get_bb(PP::KING, by_colour);
    return is_attacked(by_colour, king_bb);
}

bool Board::is_attacked(Colour by_colour, BitBoard bb) const {
    assert(bb.count() == 1);

    Square sq = bb.get_lsb_square();
    Colour other = !by_colour;
    auto occupied = get_bb(CC::WHITE) | get_bb(CC::BLACK);

    if (get_bb(PP::KING, other) & Mask::KING_MASKS.at(sq))
        return true;

    if (by_colour == CC::WHITE) {
        if ((bb.north().west() | bb.north().east()) & get_bb(PP::PAWN, other))
            return true;
    } else {
        if ((bb.south().west() | bb.south().east()) & get_bb(PP::PAWN, other))
            return true;
    }

    if (get_bb(PP::KNIGHT, other) & Mask::KNIGHT_MASKS.at(sq))
        return true;

    if (Magic::bishop_attacks(sq, occupied) &
        (get_bb(PP::BISHOP, other) | get_bb(PP::QUEEN, other)))
        return true;

    if (Magic::rook_attacks(sq, occupied) &
        (get_bb(PP::ROOK, other) | get_bb(PP::QUEEN, other)))
        return true;

    return false;
}

template std::vector<Move> Board::get_moves<true>();
template std::vector<Move> Board::get_moves<false>();
