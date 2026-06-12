#include "board.hpp"
#include <random>

std::mt19937 rng;
std::uniform_int_distribution<uint64_t> my_rand;

std::array<std::array<uint64_t, 64>, 12> create_hash() {
    std::array<std::array<uint64_t, 64>, 12> h;
    for (int p = 0; p < 12; p++) {
        for (int i = 0; i < 64; i++) {
            h[p][i] = my_rand(rng);
        }
    }
    return h;
}

std::array<uint64_t, 8> create_en_passant_file() {
    std::array<uint64_t, 8> ep;
    for (int i = 0; i < 8; i++) {
        ep[i] = my_rand(rng);
    }
    return ep;
}

std::array<uint64_t, 4> create_castling() {
    std::array<uint64_t, 4> c;
    for (int i = 0; i < 4; i++) {
        c[i] = my_rand(rng);
    }
    return c;
}

const std::array<std::array<uint64_t, 64>, 12> hash =
    create_hash(); // [piece][square]
const std::array<uint64_t, 8> en_passant_file = create_en_passant_file();
const std::array<uint64_t, 4> castling = create_castling();
const uint64_t black_move = my_rand(rng);

int Board::get_hash() const {
    uint64_t ret_hash = 0;

    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++) {
            auto piece_bb = piece_bbs[p] & colour_bbs[c];
            while (piece_bb) {
                auto sq = piece_bb.get_square_pop();
                int pc = p * 2 + c; // piece code
                ret_hash ^= hash[pc][sq];
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        if (can_castle[i])
            ret_hash ^= castling[i];
    }

    if (get_turn() == Colour::BLACK)
        ret_hash ^= black_move;

    if (!moves.empty() && moves.back().flags() == 0b0001) {
        ret_hash ^= en_passant_file[moves.back().to() & 7]; // mod 8
    }

    return ret_hash;
}
