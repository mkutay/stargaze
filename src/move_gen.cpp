#include "move_gen.hpp"
#include "board.hpp"

const int knight_moves_d[8] = {-10, -6, -17, -15, 6, 10, 15, 17};
const int diagonal_moves_d[4] = {7, 9, -7, -9};
const int cardinal_moves_d[4] = {8, 1, -8, -1};
const uint64_t knight_masks[8] = {
    0xfcfcfcfcfcfcfc00, 0x3f3f3f3f3f3f3f00, 0xfefefefefefe0000,
    0x7f7f7f7f7f7f0000, 0x00fcfcfcfcfcfcfc, 0x003f3f3f3f3f3f3f,
    0x0000fefefefefefe, 0x00007f7f7f7f7f7f,
};
const uint64_t diagonal_masks[4] = {
    0x7f7f7f7f7f7f7f00,
    0xfefefefefefefe00,
    0x00fefefefefefefe,
    0x007f7f7f7f7f7f7f,
};
const uint64_t cardinal_masks[4] = {
    0xffffffffffffff00,
    0xfefefefefefefefe,
    0x00ffffffffffffff,
    0x7f7f7f7f7f7f7f7f,
};

const uint8_t bit_scan_index64[64] = {
    0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
    54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
    46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
    25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};
const uint64_t debruijn64 = 0x03f79d71b4cb0a89;

int bit_scan_forward(uint64_t bb) {
#ifdef DEBUG
    assert(bb != 0);
#endif
    return bit_scan_index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}

std::vector<Move> Board::get_moves() {
    /**
     * Pseudolegal move generation, which include moves that leave the king in
     * check
     */
    std::vector<Move> pseudo;

    Colour turn = get_turn();
    int mul = turn == Colour::WHITE ? 1 : -1;

    uint64_t white_pieces = get_colour_bb(Colour::WHITE);
    uint64_t black_pieces = get_colour_bb(Colour::BLACK);
    uint64_t occupied = white_pieces | black_pieces;
    uint64_t empty = ~occupied;
    uint64_t other_pieces = turn == Colour::WHITE ? black_pieces : white_pieces;

    uint64_t pawns, pawn_push_one, pawn_push_two, pawn_capture_left,
        pawn_capture_right, pawn_push_one_promotion,
        pawn_capture_left_promotion, pawn_capture_right_promotion;
    // pawns
    if (turn == Colour::WHITE) {
        pawns = get_piece_bb(Piece::PAWN, Colour::WHITE);
        pawn_push_one = (pawns << 8) & empty;
        pawn_push_two = ((pawn_push_one & (0xffull << 16)) << 8) & empty;
        pawn_capture_left = ((pawns & 0xfefefefefefefefe) << 7) & other_pieces;
        pawn_capture_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << 9) & other_pieces;
        pawn_push_one_promotion = pawn_push_one & (0xffull << 56);
        pawn_capture_left_promotion = pawn_capture_left & (0xffull << 56);
        pawn_capture_right_promotion = pawn_capture_right & (0xffull << 56);
    } else {
        pawns = get_piece_bb(Piece::PAWN, Colour::BLACK);
        pawn_push_one = (pawns >> 8) & empty;
        pawn_push_two = ((pawn_push_one & (0xffull << 40)) >> 8) & empty;
        pawn_capture_left = ((pawns & 0x7f7f7f7f7f7f7f7f) >> 7) & other_pieces;
        pawn_capture_right = ((pawns & 0xfefefefefefefefe) >> 9) & other_pieces;
        pawn_push_one_promotion = pawn_push_one & 0xff;
        pawn_capture_left_promotion = pawn_capture_left & 0xff;
        pawn_capture_right_promotion = pawn_capture_right & 0xff;
    }
    if (!moves.empty() && moves.back().flags() == 0b0001) {
        int to = moves.back().to();
        if (pawns & (1ull << (to + 1)) & 0xfefefefefefefefe) {
            pseudo.emplace_back(Move(to + 1, to + 8 * mul, 0b0101));
        }
        if (pawns & (1ull << (to - 1)) & 0x7f7f7f7f7f7f7f7f) {
            pseudo.emplace_back(Move(to - 1, to + 8 * mul, 0b0101));
        }
    }
    for (int i = 0; i < 64; i++) {
        uint64_t bit = 1ull << i;
        if (pawn_push_one_promotion & bit) {
            pseudo.emplace_back(Move(i - 8 * mul, i, 0b1000));
            pseudo.emplace_back(Move(i - 8 * mul, i, 0b1001));
            pseudo.emplace_back(Move(i - 8 * mul, i, 0b1010));
            pseudo.emplace_back(Move(i - 8 * mul, i, 0b1011));
        } else if (pawn_push_one & bit) {
            pseudo.emplace_back(Move(i - 8 * mul, i, 0b0000));
        }

        if (pawn_push_two & bit) {
            pseudo.emplace_back(Move(i - 16 * mul, i, 0b0001));
        }

        if (pawn_capture_left_promotion & bit) {
            pseudo.emplace_back(Move(i - 7 * mul, i, 0b1100));
            pseudo.emplace_back(Move(i - 7 * mul, i, 0b1101));
            pseudo.emplace_back(Move(i - 7 * mul, i, 0b1110));
            pseudo.emplace_back(Move(i - 7 * mul, i, 0b1111));
        } else if (pawn_capture_left & bit) {
            pseudo.emplace_back(Move(i - 7 * mul, i, 0b0100));
        }

        if (pawn_capture_right_promotion & bit) {
            pseudo.emplace_back(Move(i - 9 * mul, i, 0b1100));
            pseudo.emplace_back(Move(i - 9 * mul, i, 0b1101));
            pseudo.emplace_back(Move(i - 9 * mul, i, 0b1110));
            pseudo.emplace_back(Move(i - 9 * mul, i, 0b1111));
        } else if (pawn_capture_right & bit) {
            pseudo.emplace_back(Move(i - 9 * mul, i, 0b0100));
        }
    }

    // knight
    uint64_t knights = get_piece_bb(Piece::KNIGHT, turn);
    for (int d = 0; d < 8; d++) {
        uint64_t temp = knights & knight_masks[d];
        if (d < 4)
            temp >>= -knight_moves_d[d];
        else
            temp <<= knight_moves_d[d];
        uint64_t temp_captures = temp & other_pieces;
        temp = (temp & empty) | temp_captures;
        while (temp) {
            uint64_t ls1b = temp & -temp;
            int i = bit_scan_forward(ls1b);
            pseudo.emplace_back(Move(i - knight_moves_d[d], i,
                                     bool(temp_captures & ls1b) << 2));
            temp ^= ls1b;
        }
    }

    // bishop and queen
    uint64_t bishops = get_piece_bb(Piece::BISHOP, turn);
    uint64_t queens = get_piece_bb(Piece::QUEEN, turn);
    for (int d = 0; d < 4; d++) {
        uint64_t temp_bishops = bishops;
        uint64_t temp_queens = queens;
        for (int m = 0; temp_bishops || temp_queens; m++) {
            if (d < 2)
                temp_bishops <<= diagonal_moves_d[d],
                    temp_queens <<= diagonal_moves_d[d];
            else
                temp_bishops >>= -diagonal_moves_d[d],
                    temp_queens >>= -diagonal_moves_d[d];
            temp_bishops &= diagonal_masks[d];
            temp_queens &= diagonal_masks[d];
            uint64_t temp_bishops_captures = temp_bishops & other_pieces;
            uint64_t temp_queens_captures = temp_queens & other_pieces;
            temp_bishops = (temp_bishops & empty) | temp_bishops_captures;
            temp_queens = (temp_queens & empty) | temp_queens_captures;
            uint64_t temp = temp_bishops;
            while (temp) {
                uint64_t ls1b = temp & -temp;
                int i = bit_scan_forward(ls1b);
                pseudo.emplace_back(
                    Move(i - (m + 1) * diagonal_moves_d[d], i,
                         bool(temp_bishops_captures & ls1b) << 2));
                temp ^= ls1b;
            }
            temp = temp_queens;
            while (temp) {
                uint64_t ls1b = temp & -temp;
                int i = bit_scan_forward(ls1b);
                pseudo.emplace_back(
                    Move(i - (m + 1) * diagonal_moves_d[d], i,
                         bool(temp_queens_captures & ls1b) << 2));
                temp ^= ls1b;
            }
            temp_bishops &= empty;
            temp_queens &= empty;
        }
    }

    // rook and queen
    uint64_t rooks = get_piece_bb(Piece::ROOK, turn);
    for (int d = 0; d < 4; d++) {
        int cc = cardinal_moves_d[d];
        uint64_t temp_rooks = rooks;
        uint64_t temp_queens = queens;
        for (int m = 0; temp_rooks || temp_queens; m++) {
            if (d < 2)
                temp_rooks <<= cc, temp_queens <<= cc;
            else
                temp_rooks >>= -cc, temp_queens >>= -cc;
            temp_rooks &= cardinal_masks[d];
            temp_queens &= cardinal_masks[d];
            uint64_t temp_rooks_captures = temp_rooks & other_pieces;
            uint64_t temp_queens_captures = temp_queens & other_pieces;
            temp_rooks = (temp_rooks & empty) | temp_rooks_captures;
            temp_queens = (temp_queens & empty) | temp_queens_captures;
            uint64_t temp = temp_rooks;
            while (temp) {
                uint64_t ls1b = temp & -temp;
                int i = bit_scan_forward(ls1b);
                pseudo.emplace_back(
                    Move(i - (m + 1) * cc, i,
                         bool(temp_rooks_captures & ls1b) << 2));
                temp ^= ls1b;
            }
            temp = temp_queens;
            while (temp) {
                uint64_t ls1b = temp & -temp;
                int i = bit_scan_forward(ls1b);
                pseudo.emplace_back(
                    Move(i - (m + 1) * cc, i,
                         bool(temp_queens_captures & ls1b) << 2));
                temp ^= ls1b;
            }
            temp_rooks &= empty;
            temp_queens &= empty;
        }
    }

    // king
    uint64_t king = get_piece_bb(Piece::KING, turn);
    for (int d = 0; d < 4; d++) {
        int cc = cardinal_moves_d[d], dd = diagonal_moves_d[d];
        uint64_t temp_king_cardinal = king;
        uint64_t temp_king_diagonal = king;
        if (d < 2)
            temp_king_cardinal <<= cc, temp_king_diagonal <<= dd;
        else
            temp_king_cardinal >>= -cc, temp_king_diagonal >>= -dd;
        temp_king_cardinal &= cardinal_masks[d];
        temp_king_diagonal &= diagonal_masks[d];
        uint64_t temp_king_cardinal_captures =
            temp_king_cardinal & other_pieces;
        uint64_t temp_king_diagonal_captures =
            temp_king_diagonal & other_pieces;
        temp_king_cardinal =
            (temp_king_cardinal & empty) | temp_king_cardinal_captures;
        temp_king_diagonal =
            (temp_king_diagonal & empty) | temp_king_diagonal_captures;
        if (temp_king_cardinal) {
#ifdef DEBUG
            assert(temp_king_cardinal ==
                   (temp_king_cardinal & -temp_king_cardinal));
#endif
            int i = bit_scan_forward(temp_king_cardinal);
            pseudo.emplace_back(Move(
                i - cc, i,
                bool(temp_king_cardinal & temp_king_cardinal_captures) << 2));
        }
        if (temp_king_diagonal) {
#ifdef DEBUG
            assert(temp_king_diagonal ==
                   (temp_king_diagonal & -temp_king_diagonal));
#endif
            int i = bit_scan_forward(temp_king_diagonal);
            pseudo.emplace_back(Move(
                i - dd, i,
                bool(temp_king_diagonal & temp_king_diagonal_captures) << 2));
        }
    }

    std::vector<Move> non_pseudo_moves;
    for (Move move : pseudo) {
        Colour current_turn = turn;
        make_move(move);
        if (!is_in_check(current_turn))
            non_pseudo_moves.emplace_back(move);
        undo_move();
    }

    // castling
    if (turn == Colour::WHITE && has_piece_at(4, Piece::KING, Colour::WHITE)) {
        if (can_castle[0] && !(occupied & 0x60) &&
            has_piece_at(7, Piece::ROOK, Colour::WHITE) &&
            !is_attacked(Colour::WHITE, 4u) &&
            !is_attacked(Colour::WHITE, 5u) &&
            !is_attacked(Colour::WHITE, 6u)) { // white king's side
            non_pseudo_moves.emplace_back(Move(4, 6, 0b0010));
        }
        if (can_castle[1] && !(occupied & 0x0e) &&
            has_piece_at(0, Piece::ROOK, Colour::WHITE) &&
            !is_attacked(Colour::WHITE, 4u) &&
            !is_attacked(Colour::WHITE, 3u) &&
            !is_attacked(Colour::WHITE, 2u)) { // white queen's side
            non_pseudo_moves.emplace_back(Move(4, 2, 0b0011));
        }
    } else if (turn == Colour::BLACK &&
               has_piece_at(60, Piece::KING, Colour::BLACK)) {
        if (can_castle[2] && !(occupied & (0x60ll << 56)) &&
            has_piece_at(63, Piece::ROOK, Colour::BLACK) &&
            !is_attacked(Colour::BLACK, 60u) &&
            !is_attacked(Colour::BLACK, 61u) &&
            !is_attacked(Colour::BLACK, 62u)) { // black king's side
            non_pseudo_moves.emplace_back(Move(60, 62, 0b0010));
        }
        if (can_castle[3] && !(occupied & (0x0ell << 56)) &&
            has_piece_at(56, Piece::ROOK, Colour::BLACK) &&
            !is_attacked(Colour::BLACK, 60u) &&
            !is_attacked(Colour::BLACK, 59u) &&
            !is_attacked(Colour::BLACK, 58u)) { // black queen's side
            non_pseudo_moves.emplace_back(Move(60, 58, 0b0011));
        }
    }

    return non_pseudo_moves;
}

bool Board::is_in_check(Colour by_colour) {
    uint64_t king_bb = get_piece_bb(Piece::KING, by_colour);
    return is_attacked(by_colour, king_bb);
}

bool Board::is_attacked(Colour by_colour,
                        std::variant<unsigned int, uint64_t> square) {
    uint64_t square_bb;
    if (std::holds_alternative<unsigned int>(square)) {
        square_bb = 1ull << std::get<unsigned int>(square);
    } else {
        square_bb = std::get<uint64_t>(square);
    }

    Colour other = !by_colour;
    uint64_t other_queens = get_piece_bb(Piece::QUEEN, other);
    uint64_t other_rooks = get_piece_bb(Piece::ROOK, other);
    uint64_t other_bishops = get_piece_bb(Piece::BISHOP, other);
    uint64_t other_knights = get_piece_bb(Piece::KNIGHT, other);
    uint64_t empty = get_empty_bb();

    for (int d = 0; d < 8; d++) {
        uint64_t temp = other_knights;
        if (d < 4)
            temp >>= -knight_moves_d[d];
        else
            temp <<= knight_moves_d[d];
        temp &= knight_masks[d];
        if (temp & square_bb)
            return true;
    }
    for (int d = 0; d < 4; d++) {
        int cc = cardinal_moves_d[d], dd = diagonal_moves_d[d];
        uint64_t temp_king_cardinal = square_bb;
        uint64_t temp_king_diagonal = square_bb;
        while (temp_king_cardinal || temp_king_diagonal) {
            if (d < 2)
                temp_king_cardinal <<= cc, temp_king_diagonal <<= dd;
            else
                temp_king_cardinal >>= -cc, temp_king_diagonal >>= -dd;
            temp_king_cardinal &= cardinal_masks[d];
            temp_king_diagonal &= diagonal_masks[d];
            if (temp_king_cardinal & (other_rooks | other_queens))
                return true;
            if (temp_king_diagonal & (other_bishops | other_queens))
                return true;
            temp_king_cardinal &= empty;
            temp_king_diagonal &= empty;
        }
    }
    return false;
}
