#include "board.hpp"
#include "move.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#ifdef DEBUG
#include "debug.hpp"
#else
#define debug(...) void(38)
#endif

Board::Board() {
    for (int i = 0; i < 16; i++)
        pieces[std::to_underlying(BBPiece::WHITE)] |= 1ull << i;
    for (int i = 48; i < 64; i++)
        pieces[std::to_underlying(BBPiece::BLACK)] |= 1ull << i;
    for (int i = 8; i < 16; i++)
        pieces[std::to_underlying(BBPiece::PAWN)] |= 1ull << i,
            pieces[std::to_underlying(BBPiece::PAWN)] |= 1ull << (i + 40);
    pieces[std::to_underlying(BBPiece::KNIGHT)] =
        1ull << 1 | 1ull << 6 | 1ull << 57 | 1ull << 62;
    pieces[std::to_underlying(BBPiece::BISHOP)] =
        1ull << 2 | 1ull << 5 | 1ull << 58 | 1ull << 61;
    pieces[std::to_underlying(BBPiece::ROOK)] =
        1 | 1ull << 7 | 1ull << 56 | 1ull << 63;
    pieces[std::to_underlying(BBPiece::QUEEN)] = 1ull << 3 | 1ull << 59;
    pieces[std::to_underlying(BBPiece::KING)] = 1ull << 4 | 1ull << 60;

    moves = std::vector<Move>();
    board_history = std::vector<std::array<uint64_t, 8>>();
}

// assumes move is valid
void Board::make_move(Move move) {
    board_history.emplace_back(pieces);
    castle_history.emplace_back(can_castle);

    int from = move.from();
    int to = move.to();
    int flags = move.flags();

    auto turn_underlying = std::to_underlying(turn);

    if (get_piece(from) == Piece::KING || get_piece(to) == Piece::KING)
        can_castle[turn_underlying * 2] = can_castle[turn_underlying * 2 + 1] =
            false;

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
        clear(to + (turn == Colour::BLACK
                        ? -8
                        : 8)); // remove the pawn according to who's turn it is
    } else {                   // promotion
        set_piece(to, move.promotion_piece(), turn);
        clear(from);
    }
    moves.emplace_back(move);
    turn = !turn;
}

void Board::undo_move() {
    pieces = board_history.back();
    board_history.pop_back();
    moves.pop_back();
    can_castle = castle_history.back();
    castle_history.pop_back();
    turn = !turn;
}

std::string Board::to_string() {
    std::vector<std::string> result;
    std::string temp = "";
    for (int i = 0; i < 64; i++) {
        if (i != 0 && i % 8 == 0)
            result.emplace_back(temp), temp = "";
        std::string ch;
        Piece p = get_piece(i);
        Colour c = get_colour(i);
        if (c == Colour::WHITE) {
            switch (p) {
            case Piece::W_PAWN:
                ch = "P";
                break;
            case Piece::KNIGHT:
                ch = "N";
                break;
            case Piece::BISHOP:
                ch = "B";
                break;
            case Piece::ROOK:
                ch = "R";
                break;
            case Piece::QUEEN:
                ch = "Q";
                break;
            case Piece::KING:
                ch = "K";
                break;
            case Piece::EMPTY:
                ch = ".";
                break;
            case Piece::B_PAWN:
                assert(false);
                break;
            }
        } else {
            switch (p) {
            case Piece::B_PAWN:
                ch = "p";
                break;
            case Piece::KNIGHT:
                ch = "n";
                break;
            case Piece::BISHOP:
                ch = "b";
                break;
            case Piece::ROOK:
                ch = "r";
                break;
            case Piece::QUEEN:
                ch = "q";
                break;
            case Piece::KING:
                ch = "k";
                break;
            case Piece::EMPTY:
                ch = ".";
                break;
            case Piece::W_PAWN:
                assert(false);
                break;
            }
        }
        temp += ch + " ";
    }
    result.emplace_back(temp), temp = "";
    reverse(result.begin(), result.end());
    for (std::string i : result)
        temp += i + "\n";
    return temp;
}

Colour Board::get_colour(int i) {
    return pieces[std::to_underlying(BBPiece::WHITE)] & (1ull << i)
               ? Colour::WHITE
               : Colour::BLACK;
}

Piece Board::get_piece(int i) {
    if (pieces[std::to_underlying(BBPiece::PAWN)] & (1ull << i))
        return pieces[std::to_underlying(BBPiece::WHITE)] & (1ull << i)
                   ? Piece::W_PAWN
                   : Piece::B_PAWN;
    if (pieces[std::to_underlying(BBPiece::KNIGHT)] & (1ull << i))
        return Piece::KNIGHT;
    if (pieces[std::to_underlying(BBPiece::BISHOP)] & (1ull << i))
        return Piece::BISHOP;
    if (pieces[std::to_underlying(BBPiece::ROOK)] & (1ull << i))
        return Piece::ROOK;
    if (pieces[std::to_underlying(BBPiece::QUEEN)] & (1ull << i))
        return Piece::QUEEN;
    if (pieces[std::to_underlying(BBPiece::KING)] & (1ull << i))
        return Piece::KING;
    return Piece::EMPTY;
}

void Board::make_move_bb(int from, int to, bool is_capture) {
    Piece from_piece = get_piece(from), to_piece = get_piece(to);
    Colour from_colour = get_colour(from), to_colour = get_colour(to);
    uint64_t fromBB = 1ull << from;
    uint64_t toBB = 1ull << to;
    uint64_t bb = fromBB | toBB;
    pieces[piece_code(from_piece)] ^= bb;
    pieces[std::to_underlying(from_colour)] ^= bb;
    if (is_capture && to_piece != Piece::EMPTY)
        pieces[piece_code(to_piece)] ^= toBB,
            pieces[std::to_underlying(to_colour)] ^= toBB;
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

void Board::set_piece(int i, Piece p, Colour c) {
    clear(i);
    pieces[std::to_underlying(
        c == Colour::WHITE ? BBPiece::WHITE : BBPiece::BLACK)] |= (1ull << i);
    pieces[piece_code(p)] |= (1ull << i);
}

int Board::piece_code(Piece p) { // get enum bb piece code from piece
    if (p == Piece::W_PAWN || p == Piece::B_PAWN)
        p = Piece::B_PAWN;
    return int(p);
}

uint64_t Board::get_white_pieces() {
    return pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_pieces() {
    return pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_white_pawns() {
    return pieces[std::to_underlying(BBPiece::PAWN)] &
           pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_pawns() {
    return pieces[std::to_underlying(BBPiece::PAWN)] &
           pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_white_knights() {
    return pieces[std::to_underlying(BBPiece::KNIGHT)] &
           pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_knights() {
    return pieces[std::to_underlying(BBPiece::KNIGHT)] &
           pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_white_bishops() {
    return pieces[std::to_underlying(BBPiece::BISHOP)] &
           pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_bishops() {
    return pieces[std::to_underlying(BBPiece::BISHOP)] &
           pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_white_rooks() {
    return pieces[std::to_underlying(BBPiece::ROOK)] &
           pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_rooks() {
    return pieces[std::to_underlying(BBPiece::ROOK)] &
           pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_white_queens() {
    return pieces[std::to_underlying(BBPiece::QUEEN)] &
           pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_queens() {
    return pieces[std::to_underlying(BBPiece::QUEEN)] &
           pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_white_king() {
    return pieces[std::to_underlying(BBPiece::KING)] &
           pieces[std::to_underlying(BBPiece::WHITE)];
}
uint64_t Board::get_black_king() {
    return pieces[std::to_underlying(BBPiece::KING)] &
           pieces[std::to_underlying(BBPiece::BLACK)];
}
uint64_t Board::get_knights() {
    return pieces[std::to_underlying(BBPiece::KNIGHT)];
}
uint64_t Board::get_bishops() {
    return pieces[std::to_underlying(BBPiece::BISHOP)];
}
uint64_t Board::get_rooks() {
    return pieces[std::to_underlying(BBPiece::ROOK)];
}
uint64_t Board::get_queens() {
    return pieces[std::to_underlying(BBPiece::QUEEN)];
}
uint64_t Board::get_kings() {
    return pieces[std::to_underlying(BBPiece::KING)];
}
uint64_t Board::get_pawns() {
    return pieces[std::to_underlying(BBPiece::PAWN)];
}
std::array<uint64_t, 8> Board::get_all_pieces() const { return pieces; }
std::array<bool, 4> Board::get_castle_rights() const { return can_castle; }
