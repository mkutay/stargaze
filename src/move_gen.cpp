#include "stargaze/bitboard.hpp"
#include "stargaze/board.hpp"
#include "stargaze/colour.hpp"
#include "stargaze/magic.hpp"
#include "stargaze/mask.hpp"
#include "stargaze/piece.hpp"
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
    return attackers(attacker, square, occupied);
}

BitBoard Board::attackers(Colour attacker, Square square, BitBoard occupied,
                          BitBoard excluded) const {
    const BitBoard ours = get_bb(attacker) & occupied & ~excluded;
    BitBoard target = square;
    BitBoard attackers = 0;

    if (attacker == CC::WHITE) {
        attackers |= (target.south().west() | target.south().east()) &
                     get_bb(PP::PAWN) & ours;
    } else {
        attackers |= (target.north().west() | target.north().east()) &
                     get_bb(PP::PAWN) & ours;
    }

    attackers |= Mask::knights(square) & get_bb(PP::KNIGHT) & ours;
    attackers |= Mask::kings(square) & get_bb(PP::KING) & ours;
    attackers |= Magic::bishop_attacks(square, occupied) &
                 (get_bb(PP::BISHOP) | get_bb(PP::QUEEN)) & ours;
    attackers |= Magic::rook_attacks(square, occupied) &
                 (get_bb(PP::ROOK) | get_bb(PP::QUEEN)) & ours;

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

bool Board::gives_check(Move move, BitBoard occupied, Square square) const {
    Square from = move.from();
    Square to = move.to();
    Piece piece =
        move.is_promotion() ? move.promotion_piece() : *get_piece(from);
    BitBoard occupied_after = occupied;
    occupied_after.erase_square(from);
    occupied_after.erase_square(to);
    occupied_after.set_square(to);

    BitBoard diagonal = get_bb(PP::BISHOP, turn) | get_bb(PP::QUEEN, turn);
    BitBoard orthogonal = get_bb(PP::ROOK, turn) | get_bb(PP::QUEEN, turn);
    diagonal.erase_square(from);
    orthogonal.erase_square(from);

    if (move.is_en_passant()) {
        occupied_after.erase_square(to - 8 * turn.weight());
    } else if (move.is_castle()) {
        Square rook_from =
            move.flags() == Move::KING_SIDE_CASTLE ? to + 1 : to - 2;
        Square rook_to =
            move.flags() == Move::KING_SIDE_CASTLE ? to - 1 : to + 1;
        occupied_after.erase_square(rook_from);
        occupied_after.set_square(rook_to);
        orthogonal.erase_square(rook_from);
        orthogonal.set_square(rook_to);
    }

    if (piece == PP::BISHOP || piece == PP::QUEEN)
        diagonal.set_square(to);
    if (piece == PP::ROOK || piece == PP::QUEEN)
        orthogonal.set_square(to);

    bool direct =
        (piece == PP::PAWN &&
         (turn == CC::WHITE
              ? BitBoard(to).north().west() | BitBoard(to).north().east()
              : BitBoard(to).south().west() | BitBoard(to).south().east())
             .has_square(square)) ||
        (piece == PP::KNIGHT && Mask::knights(to).has_square(square)) ||
        (piece == PP::KING && Mask::kings(to).has_square(square));

    return direct ||
           (Magic::bishop_attacks(square, occupied_after) & diagonal)
               .has_square() ||
           (Magic::rook_attacks(square, occupied_after) & orthogonal)
               .has_square();
}

bool Board::gives_check(Move move) const {
    return gives_check(move, get_bb(turn) | get_bb(turn.opposite()),
                       get_bb(PP::KING, turn.opposite()).lsb_square());
}

std::optional<Move> Board::get_move() {
    std::optional<Move> result;
    generate_moves([&](Move move) {
        result = move;
        return false;
    });
    return result;
}

bool Board::has_legal_move() {
    bool found = false;
    generate_moves([&](Move) {
        found = true;
        return false;
    });
    return found;
}

template <bool Captures, bool KingMoves, bool Checks, bool Promotions,
          bool Quiets>
std::vector<Move> Board::get_moves() {
    std::vector<Move> legal_moves;
    legal_moves.reserve(128);
    generate_moves<Captures, KingMoves, Checks, Promotions, Quiets>(
        [&](Move move) { legal_moves.push_back(move); });
    return legal_moves;
}

bool Board::in_check() const {
    auto king_bb = get_bb(PP::KING, turn);
    return is_attacked(turn, king_bb);
}

bool Board::is_attacked(Colour by_colour, BitBoard bb) const {
    assert(bb.count() == 1);
    return attackers(by_colour.opposite(), bb.lsb_square()).has_square();
}

template std::vector<Move> Board::get_moves<true, true, true, true, true>();
template std::vector<Move> Board::get_moves<true, false, false, true, false>();
template std::vector<Move> Board::get_moves<true, false, false, false, false>();
template std::vector<Move> Board::get_moves<false, true, false, false, false>();
template std::vector<Move> Board::get_moves<false, false, false, true, false>();
template std::vector<Move> Board::get_moves<false, false, true, false, false>();
template std::vector<Move> Board::get_moves<false, false, false, false, true>();
template std::vector<Move> Board::get_moves<true, false, true, true, false>();
