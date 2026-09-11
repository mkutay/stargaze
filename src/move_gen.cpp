#include "bitboard.hpp"
#include "board.hpp"
#include "colour.hpp"
#include "magic.hpp"
#include "mask.hpp"
#include "piece.hpp"
#include <vector>

BitBoard Board::pinners(Colour colour, Square square) const {
    Colour opposite = colour.opposite();
    auto occupied = get_bb(CC::WHITE) | get_bb(CC::BLACK);
    auto diagonals = get_bb(PP::BISHOP, opposite) | get_bb(PP::QUEEN, opposite);
    auto cardinals = get_bb(PP::ROOK, opposite) | get_bb(PP::QUEEN, opposite);
    auto candidates =
        (Mask::bishops(square) & diagonals) | (Mask::rooks(square) & cardinals);
    BitBoard pinners;

    while (candidates.has_square()) {
        Square pinner = candidates.get_square_pop();
        BitBoard blockers = Magic::ray_between(square, pinner) & occupied;
        if (blockers.count() == 1 && (blockers & get_bb(colour)).has_square()) {
            pinners.set_square(pinner);
        }
    }

    return pinners;
}

BitBoard Board::pinned(Colour colour, Square square) const {
    BitBoard occupied = get_bb(CC::WHITE) | get_bb(CC::BLACK);
    BitBoard pinning_pieces = pinners(colour, square);
    BitBoard pinned;

    while (pinning_pieces.has_square()) {
        Square pinner = pinning_pieces.get_square_pop();
        pinned |=
            Magic::ray_between(square, pinner) & occupied & get_bb(colour);
    }

    assert(pinned.count() <= 8);
    assert((pinned & get_bb(colour)) == pinned);

    return pinned;
}

BitBoard Board::pin_ray(Square pivot, Square piece, BitBoard pinners) {
    while (pinners.has_square()) {
        Square pinner = pinners.get_square_pop();
        BitBoard ray = Magic::ray_between(pivot, pinner);
        if (ray.has_square(piece)) {
            return ray | BitBoard(pinner);
        }
    }

    assert(false);
    return BitBoard::EMPTY;
}

BitBoard Board::attacked(Colour attacker) const {
    Colour defender = attacker.opposite();
    BitBoard occupied =
        (get_bb(attacker) | get_bb(defender)) & ~get_bb(PP::KING, defender);
    BitBoard attacked = 0;

    auto pawns = get_bb(PP::PAWN, attacker);
    if (attacker == CC::WHITE) {
        attacked |= pawns.north().west() | pawns.north().east();
    } else {
        attacked |= pawns.south().west() | pawns.south().east();
    }

    auto knights = get_bb(PP::KNIGHT, attacker);
    while (knights.has_square()) {
        auto sq = knights.get_square_pop();
        attacked |= Mask::knights(sq);
    }

    auto bishops = get_bb(PP::BISHOP, attacker) | get_bb(PP::QUEEN, attacker);
    while (bishops.has_square()) {
        auto sq = bishops.get_square_pop();
        attacked |= Magic::bishop_attacks(sq, occupied);
    }

    auto rooks = get_bb(PP::ROOK, attacker) | get_bb(PP::QUEEN, attacker);
    while (rooks.has_square()) {
        auto sq = rooks.get_square_pop();
        attacked |= Magic::rook_attacks(sq, occupied);
    }

    auto king = get_bb(PP::KING, attacker).lsb_square();
    attacked |= Mask::kings(king);

    return attacked;
}

BitBoard Board::attackers(Colour attacker, Square square) const {
    BitBoard occupied = get_bb(CC::WHITE) | get_bb(CC::BLACK);
    BitBoard target = square;
    BitBoard attackers = 0;

    if (attacker == CC::WHITE) {
        attackers |= (target.south().west() | target.south().east()) &
                     get_bb(PP::PAWN, attacker);
    } else {
        attackers |= (target.north().west() | target.north().east()) &
                     get_bb(PP::PAWN, attacker);
    }

    attackers |= Mask::knights(square) & get_bb(PP::KNIGHT, attacker);
    attackers |= Mask::kings(square) & get_bb(PP::KING, attacker);
    attackers |= Magic::bishop_attacks(square, occupied) &
                 (get_bb(PP::BISHOP, attacker) | get_bb(PP::QUEEN, attacker));
    attackers |= Magic::rook_attacks(square, occupied) &
                 (get_bb(PP::ROOK, attacker) | get_bb(PP::QUEEN, attacker));

    return attackers;
}

BitBoard Board::check_evasion_targets(Square king, BitBoard checkers) const {
    assert(checkers.count() <= 1);
    if (!checkers.has_square()) {
        return BitBoard::ALL_SQUARES;
    }

    Square checker = checkers.lsb_square();
    Piece piece = *get_piece(checker);
    if (piece == PP::BISHOP || piece == PP::ROOK || piece == PP::QUEEN) {
        return Magic::ray_between(king, checker) | BitBoard(checker);
    }
    return BitBoard(checker);
}

template <bool CapturesOnly> std::vector<Move> Board::get_moves() {
    std::vector<Move> legal_moves;

    Colour opponent = turn.opposite();
    BitBoard us = get_bb(turn);
    BitBoard them = get_bb(opponent);
    BitBoard occupied = us | them;

    Square king_sq = get_bb(PP::KING, turn).lsb_square();
    BitBoard king_danger = attacked(opponent);
    BitBoard pinned_pieces = pinned(turn, king_sq);

    auto emit_moves = [&legal_moves, them](Square from, BitBoard targets) {
        while (targets.has_square()) {
            Square to = targets.get_square_pop();
            bool capture = them.has_square(to);
            legal_moves.emplace_back(from, to, Move::create_flags(capture));
        }
    };

    BitBoard king_targets = Mask::kings(king_sq) & ~us & ~king_danger;
    if constexpr (CapturesOnly) {
        king_targets &= them;
    }

    emit_moves(king_sq, king_targets);

    BitBoard checkers = attackers(opponent, king_sq);
    int num_checkers = checkers.count();

    // double check: only king moves are legal
    if (num_checkers >= 2) {
        return legal_moves;
    }

    BitBoard evasion_targets = check_evasion_targets(king_sq, checkers);
    BitBoard pinning_pieces = pinners(turn, king_sq);
    BitBoard active_own = num_checkers == 0 ? us : us & ~pinned_pieces;
    int mul = turn.weight();

    // Pawns
    {
        BitBoard pawns = get_bb(PP::PAWN, turn) & active_own;
        while (pawns.has_square()) {
            Square from = pawns.get_square_pop();
            BitBoard allowed_targets =
                pinned_pieces.has_square(from)
                    ? pin_ray(king_sq, from, pinning_pieces)
                    : BitBoard::ALL_SQUARES;
            BitBoard target_mask = evasion_targets & allowed_targets;

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

            BitBoard pawn = from;
            BitBoard captures = turn == CC::WHITE
                                    ? pawn.north().west() | pawn.north().east()
                                    : pawn.south().west() | pawn.south().east();
            captures &= them & target_mask;

            while (captures.has_square()) {
                Square to = captures.get_square_pop();
                if (from_rel.rank() == 6) {
                    for (auto promo : Move::PROMOTION_CAPTURE_PIECES) {
                        legal_moves.emplace_back(from, to, promo);
                    }
                } else {
                    legal_moves.emplace_back(from, to, Move::CAPTURE);
                }
            }

            // En Passant
            if (ep_square) {
                Square ep = *ep_square;
                Square ep_rel = ep.flip(turn);
                Square captured_pawn_sq = ep - 8 * mul;

                bool can_ep = (from_rel.file() > 0 && ep_rel == from_rel + 7) ||
                              (from_rel.file() < 7 && ep_rel == from_rel + 9);
                bool allowed = evasion_targets.has_square(captured_pawn_sq) &&
                               allowed_targets.has_square(ep);

                if (can_ep && allowed) {
                    BitBoard occupied_after = occupied;
                    occupied_after.erase_square(from);
                    occupied_after.erase_square(captured_pawn_sq);
                    occupied_after.set_square(ep);

                    BitBoard discovered_attackers =
                        (Magic::rook_attacks(king_sq, occupied_after) &
                         (get_bb(PP::ROOK, opponent) |
                          get_bb(PP::QUEEN, opponent))) |
                        (Magic::bishop_attacks(king_sq, occupied_after) &
                         (get_bb(PP::BISHOP, opponent) |
                          get_bb(PP::QUEEN, opponent)));
                    if (!discovered_attackers.has_square()) {
                        legal_moves.emplace_back(from, ep, Move::EN_PASSANT);
                    }
                }
            }
        }
    }

    for (Piece piece = PP::KNIGHT; piece <= PP::QUEEN; piece++) {
        BitBoard pieces = get_bb(piece, turn) & active_own;
        while (pieces.has_square()) {
            Square from = pieces.get_square_pop();
            BitBoard targets = pinned_pieces.has_square(from)
                                   ? pin_ray(king_sq, from, pinning_pieces)
                                   : BitBoard::ALL_SQUARES;

            if (piece == PP::KNIGHT) {
                targets &= Mask::knights(from);
            } else if (piece == PP::BISHOP) {
                targets &= Magic::bishop_attacks(from, occupied);
            } else if (piece == PP::ROOK) {
                targets &= Magic::rook_attacks(from, occupied);
            } else {
                targets &= Magic::bishop_attacks(from, occupied) |
                           Magic::rook_attacks(from, occupied);
            }

            targets &= ~us & evasion_targets;
            if constexpr (CapturesOnly) {
                targets &= them;
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
            if (can_castle[turn.raw() * 2] && occupied.empty(BBF1 | BBG1) &&
                king_danger.empty(BitBoard(SQE1) | BBF1 | BitBoard(SQG1))) {
                legal_moves.emplace_back(SQE1, SQG1, Move::KING_SIDE_CASTLE);
            }

            // Queen side castling
            if (can_castle[turn.raw() * 2 + 1] &&
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
    return attackers(by_colour.opposite(), bb.lsb_square()).has_square();
}

template std::vector<Move> Board::get_moves<true>();
template std::vector<Move> Board::get_moves<false>();
