#include "board.hpp"
#include "zobrist_keys.hpp"

uint64_t Board::get_hash() const { return current_hash; }

uint64_t Board::calculate_hash() const {
    uint64_t ret_hash = 0;

    for (Colour c : COLOURS) {
        for (Piece p : PIECES) {
            auto ci = std::to_underlying(c);
            auto pi = std::to_underlying(p);
            auto piece_bb = piece_bbs[pi] & colour_bbs[ci];
            while (piece_bb) {
                auto sq = piece_bb.get_square_pop();
                ret_hash ^= Zobrist::hash[ci][pi][sq];
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        if (can_castle[i])
            ret_hash ^= Zobrist::castling[i];
    }

    if (get_turn() == Colour::BLACK)
        ret_hash ^= Zobrist::black_move;

    if (!moves.empty() && moves.back().flags() == Move::DOUBLE_PAWN_PUSH) {
        ret_hash ^= Zobrist::en_passant_file[moves.back().to() & 7];
    }

    return ret_hash;
}
