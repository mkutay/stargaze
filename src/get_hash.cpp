#include <random>
#include "board.hpp"
#include "move_gen.hpp"

std::mt19937 rng;
std::uniform_int_distribution<u_int64_t> my_rand;

constexpr std::array<std::array<u_int64_t, 64>, 12> create_hash() {
    std::array<std::array<u_int64_t, 64>, 12> h;
    for (int p = 0; p < 12; p++) {
        for (int i = 0; i < 64; i++) {
            h[p][i] = my_rand(rng);
        }
    }
    return h;
}

constexpr std::array<u_int64_t, 8> create_en_passant_file() {
    std::array<u_int64_t, 8> ep;
    for (int i = 0; i < 8; i++) {
        ep[i] = my_rand(rng);
    }
    return ep;
}

constexpr std::array<u_int64_t, 4> create_castling() {
    std::array<u_int64_t, 4> c;
    for (int i = 0; i < 4; i++) {
        c[i] = my_rand(rng);
    }
    return c;
}

const std::array<std::array<u_int64_t, 64>, 12> hash = create_hash(); // [piece][square]
const std::array<u_int64_t, 8> en_passant_file = create_en_passant_file();
const std::array<u_int64_t, 4> castling = create_castling();
const u_int64_t black_move = my_rand(rng);

int Board::get_hash() {
    u_int64_t ret_hash = 0;

    for (int c = 0; c < 2; c++) {
        for (int p = 2; p < 8; p++) {
            u_int64_t piece_bb = pieces[p] & pieces[c];
            while (piece_bb) {
                u_int64_t ls1b = piece_bb & -piece_bb;
                int i = bit_scan_forward(ls1b);
                int pc = (p - 2) * 2 + c; // piece code
                ret_hash ^= hash[pc][i];
                piece_bb ^= ls1b;
            }
        }
    }
    
    for (int i = 0; i < 4; i++) {
        if (can_castle[i]) ret_hash ^= castling[i];
    }

    if (turn == BLACK) ret_hash ^= black_move;

    if (!moves.empty() && moves.back().flags() == 0b0001) {
        ret_hash ^= en_passant_file[moves.back().to() & 7]; // mod 8
    }

    return ret_hash;
}