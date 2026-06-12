#include "bitboard.hpp"
#include "board.hpp"
#include "enums.hpp"
#include "move.hpp"
#include "square.hpp"
#include <algorithm>
#include <array>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>

Board::Board()
    : piece_bbs{
          BB::A2 | BB::B2 | BB::C2 | BB::D2 | BB::E2 | BB::F2 | BB::G2 |
              BB::H2 | BB::A7 | BB::B7 | BB::C7 | BB::D7 | BB::E7 | BB::F7 |
              BB::G7 | BB::H7,               // PAWN
          BB::B1 | BB::G1 | BB::B8 | BB::G8, // KNIGHT
          BB::C1 | BB::F1 | BB::C8 | BB::F8, // BISHOP
          BB::A1 | BB::H1 | BB::A8 | BB::H8, // ROOK
          BB::D1 | BB::D8,                   // QUEEN
          BB::E1 | BB::E8,                   // KING
      },
      colour_bbs{
          BB::A1 | BB::B1 | BB::C1 | BB::D1 | BB::E1 | BB::F1 | BB::G1 |
              BB::H1 | BB::A2 | BB::B2 | BB::C2 | BB::D2 | BB::E2 | BB::F2 |
              BB::G2 | BB::H2, // WHITE
          BB::A8 | BB::B8 | BB::C8 | BB::D8 | BB::E8 | BB::F8 | BB::G8 |
              BB::H8 | BB::A7 | BB::B7 | BB::C7 | BB::D7 | BB::E7 | BB::F7 |
              BB::G7 | BB::H7, // BLACK
      },
      can_castle{true, true, true, true}, turn{Colour::WHITE} {}

/**
 * Make a move on the board, updating the board state accordingly. This includes
 * updating the pieces, castling rights, and move history.
 *
 * Note that we assume the move is valid and legal. That is, we don't check if
 * the move is actually possible, such as moving a piece that isn't there, or
 * moving to a square occupied by your own piece, or moving into check. We also
 * don't check if the move is legal in terms of the rules of chess, such as
 * castling through check or en passant when not possible.
 */
void Board::make_move(Move move) {
    board_history.emplace_back(piece_bbs, colour_bbs);
    castle_history.emplace_back(can_castle);

    auto from = move.from();
    auto to = move.to();
    auto flags = move.flags();

    /**
     * King move -- clear both castling rights for the side that moved, since
     * the king can never castle again after it has moved.
     */
    if ((get_bb(Piece::KING) & BitBoard(from)) ||
        (get_bb(Piece::KING) & BitBoard(to))) {
        int turn_index = std::to_underlying(turn);
        can_castle[turn_index * 2] = can_castle[turn_index * 2 + 1] = false;
    }

    /**
     * Rook move or capture of the rook -- clear castling rights for the side
     * that had the rook on the original square.
     */
    if (from == SQ::A1 || to == SQ::A1)
        can_castle[1] = false; // white queen-side
    if (from == SQ::H1 || to == SQ::H1)
        can_castle[0] = false; // white king-side
    if (from == SQ::A8 || to == SQ::A8)
        can_castle[3] = false; // black queen-side
    if (from == SQ::H8 || to == SQ::H8)
        can_castle[2] = false; // black king-side

    switch (flags) {
    case Move::QUIET:
    case Move::DOUBLE_PAWN_PUSH:
        make_move_bb<false>(from, to);
        break;
    case Move::KING_SIDE_CASTLE:
        make_move_bb<false>(from, to);
        make_move_bb<false>(to + 1, to - 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        make_move_bb<false>(from, to);
        make_move_bb<false>(to - 2, to + 1);
        break;
    case Move::CAPTURE:
        make_move_bb<true>(from, to);
        break;
    case Move::EN_PASSANT: {
        make_move_bb<false>(from, to);
        // remove the captured pawn according to whose turn it is
        auto turn_underlying = std::to_underlying(turn);

        // if black, add 8; if white, remove 8
        auto addition = turn_underlying * 16 - 8;
        BitBoard bb = to + addition;

        get_bb(Piece::PAWN) &= ~bb;
        get_bb(!turn) &= ~bb;
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE: {
        /**
         * With a capture, we need to clear the `to` square for the piece that
         * is being captured.
         */

        auto piece = get_piece(to);
        BitBoard bb = to;

        get_bb(*piece) &= ~bb;
        get_bb(!turn) &= ~bb;

        [[fallthrough]];
    }
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION: {
        /**
         * For all promotions, we need to clear the `from` square for the
         * pawn that is moving, and set the `to` square for the piece that
         * is being promoted to.
         *
         * Captures are handled with the fallthrough from above.
         */

        BitBoard from_bb = from;
        BitBoard to_bb = to;
        Piece promoted_piece = move.promotion_piece();

        get_bb(Piece::PAWN) &= ~from_bb;
        get_bb(turn) &= ~from_bb;

        get_bb(promoted_piece) |= to_bb;
        get_bb(turn) |= to_bb;
        break;
    }
    }

    moves.emplace_back(move);
    turn = !turn;
}

void Board::undo_move() {
    auto snap = board_history.back();
    piece_bbs = snap.piece_bbs;
    colour_bbs = snap.colour_bbs;
    board_history.pop_back();
    moves.pop_back();
    can_castle = castle_history.back();
    castle_history.pop_back();
    turn = !turn;
}

Colour Board::get_turn() const { return turn; }

std::string Board::to_string() const {
    const static std::unordered_map<Piece, std::array<std::string, 2>>
        piece_string = {
            {Piece::PAWN, {"♟", "♙"}},   {Piece::KNIGHT, {"♞", "♘"}},
            {Piece::BISHOP, {"♝", "♗"}}, {Piece::ROOK, {"♜", "♖"}},
            {Piece::QUEEN, {"♛", "♕"}},  {Piece::KING, {"♚", "♔"}},
        };

    std::vector<std::string> result;
    std::string temp = "";
    for (int i = 0; i < 64; i++) {
        if (i != 0 && i % 8 == 0)
            result.emplace_back(temp), temp = "";

        auto sq = get_piece_colour(i);
        if (sq) {
            temp +=
                piece_string.at(sq->first).at(std::to_underlying(sq->second)) +
                " ";
        } else {
            temp += ". ";
        }
    }
    result.emplace_back(temp);
    temp = "";
    reverse(result.begin(), result.end());
    for (std::string i : result)
        temp += i + "\n";
    return temp;
}

std::optional<std::pair<Piece, Colour>>
Board::get_piece_colour(Square sq) const {
    auto piece = get_piece(sq);
    auto colour = get_colour(sq);

    if (piece && colour) {
        return std::make_pair(*piece, *colour);
    }

    return std::nullopt;
}

std::optional<Piece> Board::get_piece(Square sq) const {
    BitBoard bb = sq;

    for (auto type : PIECES)
        if (get_bb(type) & bb)
            return type;

    return std::nullopt;
}

std::optional<Colour> Board::get_colour(Square sq) const {
    BitBoard bb = sq;

    for (auto colour : COLOURS)
        if (get_bb(colour) & bb)
            return colour;

    return std::nullopt;
}

bool Board::has_piece_at(BitBoard bb, Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour) & bb;
}

template <bool is_capture> void Board::make_move_bb(Square from, Square to) {
    auto from_pc = get_piece_colour(from);

    // Snapshot the captured piece BEFORE modifying any bitboard. If we read
    // it after the XOR below, the moving piece has already landed on `to`,
    // so get_piece_colour would return the mover (or nullopt for same-type
    // captures where the XOR clears the bit), corrupting the board state.
    [[maybe_unused]] auto to_pc = get_piece_colour(to);

    BitBoard from_bb = from;
    BitBoard to_bb = to;
    auto bb = from_bb | to_bb;

    get_bb(from_pc->first) ^= bb;
    get_bb(from_pc->second) ^= bb;

    if constexpr (is_capture) {
        get_bb(to_pc->first) ^= to_bb;
        get_bb(to_pc->second) ^= to_bb;
    }
}

const std::vector<Move> Board::get_move_history() const { return moves; }
const std::array<bool, 4> Board::get_castling_rights() const {
    return can_castle;
}

BitBoard &Board::get_bb(Piece type) {
    return piece_bbs[std::to_underlying(type)];
}

BitBoard Board::get_bb(Piece type) const {
    return piece_bbs[std::to_underlying(type)];
}

BitBoard &Board::get_bb(Colour colour) {
    return colour_bbs[std::to_underlying(colour)];
}

BitBoard Board::get_bb(Colour colour) const {
    return colour_bbs[std::to_underlying(colour)];
}

BitBoard Board::get_bb(Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour);
}
