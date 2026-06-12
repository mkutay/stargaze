#include "board.hpp"
#include <random>

std::mt19937 rng;
std::uniform_int_distribution<uint64_t> my_rand;

// hash[colour][piece][square]
auto create_hash() {
    std::array<std::array<std::array<uint64_t, 64>, 6>, 2> h;
    for (Colour c : COLOURS) {
        for (Piece p : PIECES) {
            for (Square sq = 0; sq < 64; sq++) {
                auto ci = std::to_underlying(c);
                auto pi = std::to_underlying(p);
                h[ci][pi][sq] = my_rand(rng);
            }
        }
    }
    return h;
}

auto create_en_passant_file() {
    std::array<uint64_t, 8> ep;
    for (int i = 0; i < 8; i++) {
        ep[i] = my_rand(rng);
    }
    return ep;
}

auto create_castling() {
    std::array<uint64_t, 4> c;
    for (int i = 0; i < 4; i++) {
        c[i] = my_rand(rng);
    }
    return c;
}

// hash[colour][piece][square]
const auto hash = create_hash();
const auto en_passant_file = create_en_passant_file();
const auto castling = create_castling();
const uint64_t black_move = my_rand(rng);

int Board::get_hash() const {
    uint64_t ret_hash = 0;

    for (Colour c : COLOURS) {
        for (Piece p : PIECES) {
            auto ci = std::to_underlying(c);
            auto pi = std::to_underlying(p);
            auto piece_bb = piece_bbs[pi] & colour_bbs[ci];
            while (piece_bb) {
                auto sq = piece_bb.get_square_pop();
                ret_hash ^= hash[ci][pi][sq];
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        if (can_castle[i])
            ret_hash ^= castling[i];
    }

    if (get_turn() == Colour::BLACK)
        ret_hash ^= black_move;

    if (!moves.empty() && moves.back().flags() == Move::DOUBLE_PAWN_PUSH) {
        ret_hash ^= en_passant_file[moves.back().to() & 7]; // mod 8
    }

    return ret_hash;
}
