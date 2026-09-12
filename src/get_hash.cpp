#include "stargaze/board.hpp"
#include "stargaze/zobrist.hpp"

uint64_t Board::get_hash() const { return current_hash; }

uint64_t Board::calculate_hash() const {
    uint64_t ret_hash = 0;

    for (Colour c : COLOURS) {
        for (Piece p : PIECES) {
            auto piece_bb = piece_bbs[p.raw()] & colour_bbs[c.raw()];
            while (piece_bb.has_square()) {
                auto sq = piece_bb.get_square_pop();
                ret_hash ^= Zobrist::piece(c, p, sq);
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        if (can_castle[i])
            ret_hash ^= Zobrist::castling(i);
    }

    if (get_turn() == Colour::BLACK)
        ret_hash ^= Zobrist::black_move();

    if (ep_square) {
        ret_hash ^= Zobrist::en_passant(*ep_square);
    }

    return ret_hash;
}
