#pragma once
#include "board.hpp"

// Declare the variables
extern int mg_pawn_table[64];
extern int eg_pawn_table[64];
extern int mg_knight_table[64];
extern int eg_knight_table[64];
extern int mg_bishop_table[64];
extern int eg_bishop_table[64];
extern int mg_rook_table[64];
extern int eg_rook_table[64];
extern int mg_queen_table[64];
extern int eg_queen_table[64];
extern int mg_king_table[64];
extern int eg_king_table[64];
extern int mg_table[12][64];
extern int eg_table[12][64];
extern int gamephase_inc[12];
extern int mg_value[6];
extern int eg_value[6];
extern int* mg_pesto_table[6];
extern int* eg_pesto_table[6];

// Declare the functions
void init_eval_table();
int evaluate(Board &board);