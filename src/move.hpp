#pragma once
#include <string>
#include "enums.hpp"

// unsigned short m_move; // 16 bits (6 for from and to, 4 for type)

u_int16_t create_move(int from, int to, int flags);
u_int16_t get_move_from(u_int16_t move);
u_int16_t get_move_to(u_int16_t move);
u_int16_t get_move_flags(u_int16_t move);
bool is_move_promotion(u_int16_t move);
bool is_move_capture(u_int16_t move);
bool is_move_en_passant(u_int16_t move);
Piece get_move_promotion_piece(u_int16_t move);
std::string move_to_string(u_int16_t move);