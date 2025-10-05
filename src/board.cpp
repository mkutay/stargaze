#include "board.hpp"

Board::Board() {
    for (int i = 0; i < 16; i++) pieces[n_white] |= 1ull << i;
    for (int i = 48; i < 64; i++) pieces[n_black] |= 1ull << i;
    for (int i = 8; i < 16; i++) pieces[n_pawn] |= 1ull << i, pieces[n_pawn] |= 1ull << (i + 40);
    pieces[n_knight] = 1ull << 1 | 1ull << 6 | 1ull << 57 | 1ull << 62;
    pieces[n_bishop] = 1ull << 2 | 1ull << 5 | 1ull << 58 | 1ull << 61;
    pieces[n_rook] = 1 | 1ull << 7 | 1ull << 56 | 1ull << 63;
    pieces[n_queen] = 1ull << 3 | 1ull << 59;
    pieces[n_king] = 1ull << 4 | 1ull << 60;

    moves = std::vector<Move>();
    board_history = std::vector<std::array<u_int64_t, 8>>();
}

void Board::debug_print() {
    std::cerr << to_string() << turn << std::endl;
}

// assumes move is valid
void Board::make_move(Move move) {
    board_history.emplace_back(pieces);

    int from = move.from();
    int to = move.to();
    int flags = move.flags();

    if (get_piece(from) == KING) can_castle[turn * 2] = can_castle[turn * 2 + 1] = false;

#ifdef DEBUG
    assert(get_piece(from) != EMPTY);
#endif

    if (((from == 0 || to == 0) && !turn) || ((from == 56 || to == 56) && turn)) can_castle[turn * 2 + 1] = false;
    if (((from == 7 || to == 7) && !turn) || ((from == 63 || to == 63) && turn)) can_castle[turn * 2] = false;

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
        clear(to + (turn ? -8 : 8)); // remove the pawn according to who's turn it is
    } else { // promotion
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
    turn = !turn;
}

std::string Board::to_string() {
    std::vector<std::string> result;
    std::string temp = "";
    for (int i = 0; i < 64; i++) {
        if (i != 0 && i % 8 == 0) result.emplace_back(temp), temp = "";
        std::string ch;
        Piece p = get_piece(i);
        Colour c = get_colour(i);
        if (c == WHITE) {
            switch (p) {
                case W_PAWN: ch = "P"; break;
                case KNIGHT: ch = "N"; break;
                case BISHOP: ch = "B"; break;
                case ROOK: ch = "R"; break;
                case QUEEN: ch = "Q"; break;
                case KING: ch = "K"; break;
                case EMPTY: ch = "."; break;
                case B_PAWN: assert(false); break;
            }
        } else {
            switch (p) {
                case B_PAWN: ch = "p"; break;
                case KNIGHT: ch = "n"; break;
                case BISHOP: ch = "b"; break;
                case ROOK: ch = "r"; break;
                case QUEEN: ch = "q"; break;
                case KING: ch = "k"; break;
                case EMPTY: ch = "."; break;
                case W_PAWN: assert(false); break;
            }
        }
        temp += ch + " ";
    }
    result.emplace_back(temp), temp = "";
    reverse(result.begin(), result.end());
    for (std::string i : result) temp += i + "\n";
    return temp;
}

Colour Board::get_colour(int i) {
    return pieces[n_white] & (1ull << i) ? WHITE : BLACK;
}

Piece Board::get_piece(int i) {
    if (pieces[n_pawn] & (1ull << i)) return pieces[n_white] & (1ull << i) ? W_PAWN : B_PAWN;
    if (pieces[n_knight] & (1ull << i)) return KNIGHT;
    if (pieces[n_bishop] & (1ull << i)) return BISHOP;
    if (pieces[n_rook] & (1ull << i)) return ROOK;
    if (pieces[n_queen] & (1ull << i)) return QUEEN;
    if (pieces[n_king] & (1ull << i)) return KING;
    return EMPTY;
}

void Board::make_move_bb(int from, int to, bool is_capture) {
    Piece from_piece = get_piece(from), to_piece = get_piece(to);
    Colour from_colour = get_colour(from), to_colour = get_colour(to);
    u_int64_t fromBB = 1ull << from;
    u_int64_t toBB = 1ull << to;
    u_int64_t bb = fromBB | toBB;
    pieces[piece_code(from_piece)] ^= bb;
    pieces[from_colour] ^= bb;
    if (is_capture && to_piece != EMPTY) pieces[piece_code(to_piece)] ^= toBB, pieces[to_colour] ^= toBB;
}

void Board::clear(int i) {
    for (int j = 0; j < 8; j++) {
        pieces[j] &= ~(1ull << i);
    }
}

bool Board::is_empty(int i) {
    return !((pieces[n_white] | pieces[n_black]) & (1ull << i));
}

void Board::set_piece(int i, Piece p, Colour c) {
    clear(i);
    pieces[c == WHITE ? n_white : n_black] |= (1ull << i);
    pieces[piece_code(p)] |= (1ull << i);
}

int Board::piece_code(Piece p) { // get enum bb piece code from piece
    if (p == W_PAWN || p == B_PAWN) p = B_PAWN;
    return int(p);
}

u_int64_t Board::get_white_pieces() { return pieces[n_white]; }
u_int64_t Board::get_black_pieces() { return pieces[n_black]; }
u_int64_t Board::get_white_pawns() { return pieces[n_pawn] & pieces[n_white]; }
u_int64_t Board::get_black_pawns() { return pieces[n_pawn] & pieces[n_black]; }
u_int64_t Board::get_white_knights() { return pieces[n_knight] & pieces[n_white]; }
u_int64_t Board::get_black_knights() { return pieces[n_knight] & pieces[n_black]; }
u_int64_t Board::get_white_bishops() { return pieces[n_bishop] & pieces[n_white]; }
u_int64_t Board::get_black_bishops() { return pieces[n_bishop] & pieces[n_black]; }
u_int64_t Board::get_white_rooks() { return pieces[n_rook] & pieces[n_white]; }
u_int64_t Board::get_black_rooks() { return pieces[n_rook] & pieces[n_black]; }
u_int64_t Board::get_white_queens() { return pieces[n_queen] & pieces[n_white]; }
u_int64_t Board::get_black_queens() { return pieces[n_queen] & pieces[n_black]; }
u_int64_t Board::get_white_king() { return pieces[n_king] & pieces[n_white]; }
u_int64_t Board::get_black_king() { return pieces[n_king] & pieces[n_black]; }
u_int64_t Board::get_knights() { return pieces[n_knight]; }
u_int64_t Board::get_bishops() { return pieces[n_bishop]; }
u_int64_t Board::get_rooks() { return pieces[n_rook]; }
u_int64_t Board::get_queens() { return pieces[n_queen]; }
u_int64_t Board::get_kings() { return pieces[n_king]; }
u_int64_t Board::get_pawns() { return pieces[n_pawn]; }
std::array<u_int64_t, 8> Board::get_all_pieces() const { return pieces; }