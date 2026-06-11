#include "board.hpp"
#include "enums.hpp"
#include "move.hpp"
#include <algorithm>
#include <array>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>

Board::Board() {
    for (int i = 0; i < 16; i++) {
        get_bb(Colour::WHITE) |= 1ull << i;
        get_bb(Colour::BLACK) |= 1ull << (i + 48);
    }

    for (int i = 8; i < 16; i++) {
        get_bb(Piece::PAWN) |= 1ull << i;
        get_bb(Piece::PAWN) |= 1ull << (i + 40);
    }

    get_bb(Piece::KNIGHT) = 1ull << 1 | 1ull << 6 | 1ull << 57 | 1ull << 62;
    get_bb(Piece::BISHOP) = 1ull << 2 | 1ull << 5 | 1ull << 58 | 1ull << 61;
    get_bb(Piece::ROOK) = 1 | 1ull << 7 | 1ull << 56 | 1ull << 63;
    get_bb(Piece::QUEEN) = 1ull << 3 | 1ull << 59;
    get_bb(Piece::KING) = 1ull << 4 | 1ull << 60;

    moves = std::vector<Move>();
    board_history = std::vector<BoardSnapshot>();
    castle_history = std::vector<std::array<bool, 4>>();
}

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
    board_history.push_back({type_bbs, colour_bbs});
    castle_history.emplace_back(can_castle);

    int from = move.from();
    int to = move.to();
    int flags = move.flags();

    Colour turn = get_turn();

    if (get_bb(Piece::KING) & (1ull << from) ||
        get_bb(Piece::KING) & (1ull << to)) {
        int turn_index = turn == Colour::BLACK ? 1 : 0;
        can_castle[turn_index * 2] = can_castle[turn_index * 2 + 1] = false;
    }

    /**
     * Rook move or capture of the rook -- clear castling rights for the side
     * that had the rook on the original square.
     */
    if (from == 0 || to == 0)   // a1
        can_castle[1] = false;  // white queen-side
    if (from == 7 || to == 7)   // h1
        can_castle[0] = false;  // white king-side
    if (from == 56 || to == 56) // a8
        can_castle[3] = false;  // black queen-side
    if (from == 63 || to == 63) // h8
        can_castle[2] = false;  // black king-side

    if (flags == Move::QUIET) {
        make_move_bb(from, to, false);
    } else if (flags == Move::DOUBLE_PAWN_PUSH) {
        make_move_bb(from, to, false);
    } else if (flags == Move::KING_SIDE_CASTLE) {
        make_move_bb(from, to, false);
        make_move_bb(to + 1, to - 1, false);
    } else if (flags == Move::QUEEN_SIDE_CASTLE) {
        make_move_bb(from, to, false);
        make_move_bb(to - 2, to + 1, false);
    } else if (flags == Move::CAPTURE) {
        make_move_bb(from, to, true);
    } else if (flags == Move::EN_PASSANT) {
        make_move_bb(from, to, true);

        // remove the captured pawn according to whose turn it is
        clear(to + (turn == Colour::BLACK ? -8 : 8));
    } else { // promotion
        set_piece(to, move.promotion_piece(), turn);
        clear(from);
    }

    moves.emplace_back(move);
}

void Board::undo_move() {
    auto snap = board_history.back();
    type_bbs = snap.types;
    colour_bbs = snap.colours;
    board_history.pop_back();
    moves.pop_back();
    can_castle = castle_history.back();
    castle_history.pop_back();
}

Colour Board::get_turn() { return static_cast<Colour>(moves.size() & 1); }

std::string Board::to_string() {
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

std::optional<std::pair<Piece, Colour>> Board::get_piece_colour(int sq) const {
    auto piece = get_piece(sq);
    auto colour = get_colour(sq);

    if (piece && colour) {
        return std::make_pair(*piece, *colour);
    }

    return std::nullopt;
}

std::optional<Piece> Board::get_piece(int sq) const {
    auto bb = 1ull << sq;

    for (auto type : {Piece::PAWN, Piece::KNIGHT, Piece::BISHOP, Piece::ROOK,
                      Piece::QUEEN, Piece::KING}) {
        if (get_bb(type) & bb)
            return type;
    }

    return std::nullopt;
}

std::optional<Colour> Board::get_colour(int sq) const {
    auto bb = 1ull << sq;

    for (auto colour : {Colour::WHITE, Colour::BLACK}) {
        if (get_bb(colour) & bb)
            return colour;
    }

    return std::nullopt;
}

bool Board::has_piece_at(int sq, Piece type, Colour colour) const {
    auto bb = 1ull << sq;
    return (get_bb(type) & get_bb(colour) & bb) != 0;
}

void Board::make_move_bb(int from, int to, bool is_capture) {
    auto from_sq = get_piece_colour(from);
    auto to_sq = get_piece_colour(to);

    auto fromBB = 1ull << from;
    auto toBB = 1ull << to;
    auto bb = fromBB | toBB;

    get_bb(from_sq->first) ^= bb;
    get_bb(from_sq->second) ^= bb;

    if (is_capture && to_sq) {
        get_bb(to_sq->first) ^= toBB;
        get_bb(to_sq->second) ^= toBB;
    }
}

void Board::clear(int i) {
    auto bb = ~(1ull << i);
    for (auto &b : type_bbs)
        b &= bb;
    for (auto &b : colour_bbs)
        b &= bb;
}

void Board::set_piece(int i, Piece type, Colour colour) {
    clear(i);
    auto bb = 1ull << i;
    get_bb(type) |= bb;
    get_bb(colour) |= bb;
}

uint64_t Board::get_piece_bb(Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour);
}

uint64_t Board::get_colour_bb(Colour colour) const { return get_bb(colour); }

uint64_t Board::get_empty_bb() const {
    auto occupied = get_bb(Colour::WHITE) | get_bb(Colour::BLACK);
    return ~occupied;
}

const std::vector<Move> Board::get_move_history() const { return moves; }
const std::array<bool, 4> Board::get_castling_rights() const {
    return can_castle;
}

uint64_t &Board::get_bb(Piece type) {
    return type_bbs[std::to_underlying(type)];
}
uint64_t Board::get_bb(Piece type) const {
    return type_bbs[std::to_underlying(type)];
}

uint64_t &Board::get_bb(Colour colour) {
    return colour_bbs[std::to_underlying(colour)];
}
uint64_t Board::get_bb(Colour colour) const {
    return colour_bbs[std::to_underlying(colour)];
}
