#include "board.hpp"
#include "enums.hpp"
#include "move.hpp"
#include <algorithm>
#include <array>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef DEBUG
#include "debug.hpp"
#else
#define debug(...) void(38)
#endif

Board::Board() {
    for (int i = 0; i < 16; i++) {
        get_bb(BBPiece::WHITE) |= 1ull << i;
        get_bb(BBPiece::BLACK) |= 1ull << (i + 48);
    }

    for (int i = 8; i < 16; i++) {
        get_bb(BBPiece::PAWN) |= 1ull << i;
        get_bb(BBPiece::PAWN) |= 1ull << (i + 40);
    }

    get_bb(BBPiece::KNIGHT) = 1ull << 1 | 1ull << 6 | 1ull << 57 | 1ull << 62;
    get_bb(BBPiece::BISHOP) = 1ull << 2 | 1ull << 5 | 1ull << 58 | 1ull << 61;
    get_bb(BBPiece::ROOK) = 1 | 1ull << 7 | 1ull << 56 | 1ull << 63;
    get_bb(BBPiece::QUEEN) = 1ull << 3 | 1ull << 59;
    get_bb(BBPiece::KING) = 1ull << 4 | 1ull << 60;

    moves = std::vector<Move>();
    board_history = std::vector<std::array<uint64_t, 8>>();
    castle_history = std::vector<std::array<bool, 4>>();
}

// assumes move is valid
void Board::make_move(Move move) {
    board_history.emplace_back(pieces);
    castle_history.emplace_back(can_castle);

    int from = move.from();
    int to = move.to();
    int flags = move.flags();

    Piece turn = get_turn();

    if (get_bb(BBPiece::KING) & (1ull << from) ||
        get_bb(BBPiece::KING) & (1ull << to)) {
        auto turn_underlying = std::to_underlying(turn) - 13;
        can_castle[turn_underlying * 2] = can_castle[turn_underlying * 2 + 1] =
            false;
    }

#ifdef DEBUG
    if (get_piece(from) == Piece::EMPTY)
        debug(move, to_string());
    assert(get_piece(from) != Piece::EMPTY);
#endif

    // rook move or capture - clear castling rights for the side that had the
    // rook on the original square
    // white queen-side rook is on square 0 (a1),
    // white king-side rook on 7 (h1),
    // black queen-side rook is on 56 (a8),
    // black king-side rook on 63 (h8)
    if (from == 0 || to == 0)
        can_castle[1] = false; // white queen-side
    if (from == 7 || to == 7)
        can_castle[0] = false; // white king-side
    if (from == 56 || to == 56)
        can_castle[3] = false; // black queen-side
    if (from == 63 || to == 63)
        can_castle[2] = false; // black king-side

    if (flags == 0b0000) { // quiet move
        make_move_bb(from, to, false);
    } else if (flags == 0b0001) { // double pawn push
        make_move_bb(from, to, false);
    } else if (flags == 0b0010) { // king's side castle
        make_move_bb(from, to, false);
        make_move_bb(to + 1, to - 1, false);
    } else if (flags == 0b0011) { // queen's side castle
        make_move_bb(from, to, false);
        make_move_bb(to - 2, to + 1, false);
    } else if (flags == 0b0100) { // captures
        make_move_bb(from, to, true);
    } else if (flags == 0b0101) { // en passant
        make_move_bb(from, to, true);
        clear(to + (turn == Piece::BLACK
                        ? -8
                        : 8)); // remove the pawn according to who's turn it is
    } else {                   // promotion
        set_piece(to, move.promotion_piece(turn));
        clear(from);
    }
    moves.emplace_back(move);
}

void Board::undo_move() {
    pieces = board_history.back();
    board_history.pop_back();
    moves.pop_back();
    can_castle = castle_history.back();
    castle_history.pop_back();
}

Piece Board::get_turn() {
    // Make sure the underlying values of WHITE and BLACK are as expected.
    static_assert(std::to_underlying(Piece::WHITE) == 13);
    static_assert(std::to_underlying(Piece::BLACK) == 14);

    uint8_t turn_underlying = (moves.size() % 2) + 13;
    return static_cast<Piece>(turn_underlying);
}

std::string Board::to_string() {
    const static std::unordered_map<Piece, std::string> piece_string = {
        {Piece::W_PAWN, "P"}, {Piece::W_KNIGHT, "N"}, {Piece::W_BISHOP, "B"},
        {Piece::W_ROOK, "R"}, {Piece::W_QUEEN, "Q"},  {Piece::W_KING, "K"},
        {Piece::B_PAWN, "p"}, {Piece::B_KNIGHT, "n"}, {Piece::B_BISHOP, "b"},
        {Piece::B_ROOK, "r"}, {Piece::B_QUEEN, "q"},  {Piece::B_KING, "k"},
        {Piece::EMPTY, "."},
    };

    std::vector<std::string> result;
    std::string temp = "";
    for (int i = 0; i < 64; i++) {
        if (i != 0 && i % 8 == 0)
            result.emplace_back(temp), temp = "";

        temp += piece_string.at(get_piece(i)) + " ";
    }
    result.emplace_back(temp), temp = "";
    reverse(result.begin(), result.end());
    for (std::string i : result)
        temp += i + "\n";
    return temp;
}

Piece Board::get_colour(int i) const {
    static_assert(std::to_underlying(Piece::WHITE) == 13);
    static_assert(std::to_underlying(Piece::BLACK) == 14);

    auto bb = 1ull << i;
    auto is_black = static_cast<bool>(get_bb(BBPiece::BLACK) & bb);
    return static_cast<Piece>(is_black + 13);
}

Piece Board::get_piece(int i) const {
    auto bb = 1ull << i;

    if (get_bb(BBPiece::PAWN) & bb)
        return get_colour(i) == Piece::WHITE ? Piece::W_PAWN : Piece::B_PAWN;
    if (get_bb(BBPiece::KNIGHT) & bb)
        return get_colour(i) == Piece::WHITE ? Piece::W_KNIGHT
                                             : Piece::B_KNIGHT;
    if (get_bb(BBPiece::BISHOP) & bb)
        return get_colour(i) == Piece::WHITE ? Piece::W_BISHOP
                                             : Piece::B_BISHOP;
    if (get_bb(BBPiece::ROOK) & bb)
        return get_colour(i) == Piece::WHITE ? Piece::W_ROOK : Piece::B_ROOK;
    if (get_bb(BBPiece::QUEEN) & bb)
        return get_colour(i) == Piece::WHITE ? Piece::W_QUEEN : Piece::B_QUEEN;
    if (get_bb(BBPiece::KING) & bb)
        return get_colour(i) == Piece::WHITE ? Piece::W_KING : Piece::B_KING;

    return Piece::EMPTY;
}

void Board::make_move_bb(int from, int to, bool is_capture) {
    Piece from_piece = get_piece(from), to_piece = get_piece(to);
    Piece from_colour = get_colour(from), to_colour = get_colour(to);

    uint64_t fromBB = 1ull << from;
    uint64_t toBB = 1ull << to;
    uint64_t bb = fromBB | toBB;

    get_bb(get_bb_piece(from_piece)) ^= bb;
    get_bb(get_bb_piece(from_colour)) ^= bb;
    if (is_capture && to_piece != Piece::EMPTY) {
        get_bb(get_bb_piece(to_piece)) ^= toBB;
        get_bb(get_bb_piece(to_colour)) ^= toBB;
    }
}

void Board::clear(int i) {
    for (int j = 0; j < 8; j++) {
        pieces[j] &= ~(1ull << i);
    }
}

bool Board::is_empty(int i) {
    return !((pieces[std::to_underlying(BBPiece::WHITE)] |
              pieces[std::to_underlying(BBPiece::BLACK)]) &
             (1ull << i));
}

void Board::set_piece(int i, Piece p) {
    clear(i);
    auto bb = 1ull << i;

    get_bb(get_bb_piece(get_piece_colour(p))) |= bb;
    get_bb(get_bb_piece(p)) |= bb;
}

std::array<uint64_t, 8> Board::get_all_pieces() const { return pieces; }
std::array<bool, 4> Board::get_castle_rights() const { return can_castle; }

uint64_t Board::get_piece_bb(Piece piece) const {
    static_assert(std::to_underlying(Piece::WHITE) == 13);
    static_assert(std::to_underlying(Piece::BLACK) == 14);
    static_assert(std::to_underlying(BBPiece::WHITE) == 0);
    static_assert(std::to_underlying(BBPiece::BLACK) == 1);

    auto underlying = std::to_underlying(piece);
    if (underlying >= 13) {
        return get_bb(get_bb_piece(piece));
    }

    if (piece == Piece::EMPTY) {
        auto occupied = get_bb(BBPiece::WHITE) | get_bb(BBPiece::BLACK);
        return ~occupied;
    }

    return get_bb(get_bb_piece(piece)) &
           get_bb(get_bb_piece(get_piece_colour(piece)));
}
