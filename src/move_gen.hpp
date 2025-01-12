#pragma once
#include <cstdint>

extern const int knight_moves_d[8];
extern const int diagonal_moves_d[4];
extern const int cardinal_moves_d[4];
extern const u_int64_t knight_masks[8];
extern const u_int64_t diagonal_masks[4];
extern const u_int64_t cardinal_masks[4];

extern const u_int8_t bit_scan_index64[64];
extern const u_int64_t debruijn64;

extern int bit_scan_forward(u_int64_t bb);