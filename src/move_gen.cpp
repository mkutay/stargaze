#include "bitboard.hpp"
#include "board.hpp"
#include <array>

constexpr const static std::array<int8_t, 8> KNIGHT_MOVES = {-10, -6, -17, -15,
                                                             6,   10, 15,  17};
constexpr const static std::array<int8_t, 4> DIAGONAL_MOVES = {7, 9, -7, -9};
constexpr const static std::array<int8_t, 4> CARDINAL_MOVES = {8, 1, -8, -1};
constexpr const static std::array<BitBoard, 8> KNIGHT_MASKS = {
    0xfcfcfcfcfcfcfc00, 0x3f3f3f3f3f3f3f00, 0xfefefefefefe0000,
    0x7f7f7f7f7f7f0000, 0x00fcfcfcfcfcfcfc, 0x003f3f3f3f3f3f3f,
    0x0000fefefefefefe, 0x00007f7f7f7f7f7f,
};
constexpr const static std::array<BitBoard, 4> DIAGONAL_MASKS = {
    0x7f7f7f7f7f7f7f00,
    0xfefefefefefefe00,
    0x00fefefefefefefe,
    0x007f7f7f7f7f7f7f,
};
constexpr const static std::array<BitBoard, 4> CARDINAL_MASKS = {
    0xffffffffffffff00,
    0xfefefefefefefefe,
    0x00ffffffffffffff,
    0x7f7f7f7f7f7f7f7f,
};

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

    // These are the final "places" of pawns after these actions.
    BitBoard pawns = get_bb(Piece::PAWN, turn), pawn_push_one = 0,
             pawn_push_two = 0, pawn_capture_left = 0, pawn_capture_right = 0,
             pawn_push_one_promotion = 0, pawn_capture_left_promotion = 0,
             pawn_capture_right_promotion = 0;

    if (turn == Colour::WHITE) {
        pawn_push_one = pawns.north() & empty;
        pawn_push_two = (pawn_push_one & (0xffull << 16)).north() & empty;
        pawn_capture_left = pawns.north().west() & other_pieces;
        pawn_capture_right = pawns.north().east() & other_pieces;
        pawn_push_one_promotion = pawn_push_one & (0xffull << 56);
        pawn_capture_left_promotion = pawn_capture_left & (0xffull << 56);
        pawn_capture_right_promotion = pawn_capture_right & (0xffull << 56);
    } else {
        pawn_push_one = pawns.south() & empty;
        pawn_push_two = (pawn_push_one & (0xffull << 40)).south() & empty;
        pawn_capture_left = pawns.south().east() & other_pieces;
        pawn_capture_right = pawns.south().west() & other_pieces;
        pawn_push_one_promotion = pawn_push_one & 0xffull;
        pawn_capture_left_promotion = pawn_capture_left & 0xffull;
        pawn_capture_right_promotion = pawn_capture_right & 0xffull;
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

    // knight
    auto knights = get_bb(Piece::KNIGHT, turn);
    for (size_t d = 0; d < KNIGHT_MOVES.size(); d++) {
        auto temp = knights & KNIGHT_MASKS[d];
        if (KNIGHT_MOVES[d] < 0)
            temp >>= -KNIGHT_MOVES[d];
        else
            temp <<= KNIGHT_MOVES[d];
        auto temp_captures = temp & other_pieces;
        temp = (temp & empty) | temp_captures;
        while (temp) {
            Square sq = temp.get_square_pop();
            pseudo.emplace_back(
                sq - KNIGHT_MOVES[d], sq,
                Move::create_flags(temp_captures & BitBoard(sq)));
        }
    }

    // bishop and queen
    auto bishops = get_bb(Piece::BISHOP, turn);
    auto queens = get_bb(Piece::QUEEN, turn);
    for (size_t d = 0; d < DIAGONAL_MOVES.size(); d++) {
        auto move = DIAGONAL_MOVES[d];
        auto temp_bishops = bishops;
        auto temp_queens = queens;
        for (int m = 0; temp_bishops || temp_queens; m++) {
            if (move > 0) {
                temp_bishops <<= move;
                temp_queens <<= move;
            } else {
                temp_bishops >>= -move;
                temp_queens >>= -move;
            }
            temp_bishops &= DIAGONAL_MASKS[d];
            temp_queens &= DIAGONAL_MASKS[d];
            auto temp_bishops_captures = temp_bishops & other_pieces;
            auto temp_queens_captures = temp_queens & other_pieces;
            temp_bishops = (temp_bishops & empty) | temp_bishops_captures;
            temp_queens = (temp_queens & empty) | temp_queens_captures;
            auto temp = temp_bishops;
            while (temp) {
                auto sq = temp.get_square_pop();
                pseudo.emplace_back(
                    sq - (m + 1) * move, sq,
                    Move::create_flags(temp_bishops_captures & BitBoard(sq)));
            }
            temp = temp_queens;
            while (temp) {
                auto sq = temp.get_square_pop();
                pseudo.emplace_back(
                    sq - (m + 1) * move, sq,
                    Move::create_flags(temp_queens_captures & BitBoard(sq)));
            }
            temp_bishops &= empty;
            temp_queens &= empty;
        }
    }

    // rook and queen
    auto rooks = get_bb(Piece::ROOK, turn);
    for (size_t d = 0; d < CARDINAL_MOVES.size(); d++) {
        auto move = CARDINAL_MOVES[d];
        auto temp_rooks = rooks;
        auto temp_queens = queens;
        for (int m = 0; temp_rooks || temp_queens; m++) {
            if (move > 0) {
                temp_rooks <<= move;
                temp_queens <<= move;
            } else {
                temp_rooks >>= -move;
                temp_queens >>= -move;
            }
            temp_rooks &= CARDINAL_MASKS[d];
            temp_queens &= CARDINAL_MASKS[d];
            auto temp_rooks_captures = temp_rooks & other_pieces;
            auto temp_queens_captures = temp_queens & other_pieces;
            temp_rooks = (temp_rooks & empty) | temp_rooks_captures;
            temp_queens = (temp_queens & empty) | temp_queens_captures;
            auto temp = temp_rooks;
            while (temp) {
                auto sq = temp.get_square_pop();
                pseudo.emplace_back(
                    sq - (m + 1) * move, sq,
                    Move::create_flags(temp_rooks_captures & BitBoard(sq)));
            }
            temp = temp_queens;
            while (temp) {
                auto sq = temp.get_square_pop();
                pseudo.emplace_back(
                    sq - (m + 1) * move, sq,
                    Move::create_flags(temp_queens_captures & BitBoard(sq)));
            }
            temp_rooks &= empty;
            temp_queens &= empty;
        }
    }

    // king
    auto king = get_bb(Piece::KING, turn);
    for (size_t d = 0; d < CARDINAL_MOVES.size(); d++) {
        auto cc = CARDINAL_MOVES[d], dd = DIAGONAL_MOVES[d];
        auto temp_king_cardinal = king;
        auto temp_king_diagonal = king;
        if (cc > 0)
            temp_king_cardinal <<= cc;
        else
            temp_king_cardinal >>= -cc;
        if (dd > 0)
            temp_king_diagonal <<= dd;
        else
            temp_king_diagonal >>= -dd;

        temp_king_cardinal &= CARDINAL_MASKS[d];
        temp_king_diagonal &= DIAGONAL_MASKS[d];
        auto temp_king_cardinal_captures = temp_king_cardinal & other_pieces;
        auto temp_king_diagonal_captures = temp_king_diagonal & other_pieces;
        temp_king_cardinal =
            (temp_king_cardinal & empty) | temp_king_cardinal_captures;
        temp_king_diagonal =
            (temp_king_diagonal & empty) | temp_king_diagonal_captures;
        if (temp_king_cardinal) {
            auto sq = temp_king_cardinal.get_square_pop();
            assert(!temp_king_cardinal);
            pseudo.emplace_back(
                sq - cc, sq,
                Move::create_flags(BitBoard(sq) & temp_king_cardinal_captures));
        }
        if (temp_king_diagonal) {
            auto sq = temp_king_diagonal.get_square_pop();
            assert(!temp_king_diagonal);
            pseudo.emplace_back(
                sq - dd, sq,
                Move::create_flags(BitBoard(sq) & temp_king_diagonal_captures));
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
    if (turn == Colour::WHITE &&
        has_piece_at(BB::E1, Piece::KING, Colour::WHITE)) {
        if (can_castle[0] && occupied.empty(0x60ull) &&
            has_piece_at(BB::H1, Piece::ROOK, Colour::WHITE) &&
            !is_attacked(Colour::WHITE, SQ::E1) &&
            !is_attacked(Colour::WHITE, SQ::F1) &&
            !is_attacked(Colour::WHITE, SQ::G1)) {
            non_pseudo_moves.emplace_back(SQ::E1, SQ::G1,
                                          Move::KING_SIDE_CASTLE);
        }
        if (can_castle[1] && occupied.empty(0x0eull) &&
            has_piece_at(BB::A1, Piece::ROOK, Colour::WHITE) &&
            !is_attacked(Colour::WHITE, SQ::E1) &&
            !is_attacked(Colour::WHITE, SQ::D1) &&
            !is_attacked(Colour::WHITE, SQ::C1)) {
            non_pseudo_moves.emplace_back(SQ::E1, SQ::C1,
                                          Move::QUEEN_SIDE_CASTLE);
        }
    } else if (turn == Colour::BLACK &&
               has_piece_at(BB::E8, Piece::KING, Colour::BLACK)) {
        if (can_castle[2] && occupied.empty(0x60ull << 56) &&
            has_piece_at(BB::H8, Piece::ROOK, Colour::BLACK) &&
            !is_attacked(Colour::BLACK, SQ::E8) &&
            !is_attacked(Colour::BLACK, SQ::F8) &&
            !is_attacked(Colour::BLACK, SQ::G8)) {
            non_pseudo_moves.emplace_back(SQ::E8, SQ::G8,
                                          Move::KING_SIDE_CASTLE);
        }
        if (can_castle[3] && occupied.empty(0x0eull << 56) &&
            has_piece_at(56, Piece::ROOK, Colour::BLACK) &&
            !is_attacked(Colour::BLACK, SQ::E8) &&
            !is_attacked(Colour::BLACK, SQ::D8) &&
            !is_attacked(Colour::BLACK, SQ::C8)) {
            non_pseudo_moves.emplace_back(SQ::E8, SQ::C8,
                                          Move::QUEEN_SIDE_CASTLE);
        }
    }

    return non_pseudo_moves;
}

bool Board::is_in_check(Colour by_colour) {
    auto king_bb = get_bb(Piece::KING, by_colour);
    return is_attacked(by_colour, king_bb);
}

bool Board::is_attacked(Colour by_colour, BitBoard bb) {
    Colour other = !by_colour;
    auto other_queens = get_bb(Piece::QUEEN, other);
    auto other_rooks = get_bb(Piece::ROOK, other);
    auto other_bishops = get_bb(Piece::BISHOP, other);
    auto other_knights = get_bb(Piece::KNIGHT, other);
    auto occupied = get_bb(Colour::WHITE) | get_bb(Colour::BLACK);
    auto empty = ~occupied;

    for (size_t d = 0; d < KNIGHT_MOVES.size(); d++) {
        auto temp = other_knights;
        auto move = KNIGHT_MOVES[d];
        if (move < 0) {
            temp >>= -move;
        } else {
            temp <<= move;
        }
        temp &= KNIGHT_MASKS[d];
        if (temp & bb)
            return true;
    }

    for (size_t d = 0; d < CARDINAL_MOVES.size(); d++) {
        auto cc = CARDINAL_MOVES[d], dd = DIAGONAL_MOVES[d];
        auto temp_king_cardinal = bb;
        auto temp_king_diagonal = bb;
        while (temp_king_cardinal || temp_king_diagonal) {
            if (cc > 0)
                temp_king_cardinal <<= cc;
            else
                temp_king_cardinal >>= -cc;
            if (dd > 0)
                temp_king_diagonal <<= dd;
            else
                temp_king_diagonal >>= -dd;

            temp_king_cardinal &= CARDINAL_MASKS[d];
            temp_king_diagonal &= DIAGONAL_MASKS[d];
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
