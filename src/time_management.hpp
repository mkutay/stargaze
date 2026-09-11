#pragma once
#include <cstdint>

class Board;
class Search;

uint32_t calculate_time_limit(const Board &board, const Search &search,
                              uint32_t wtime, uint32_t btime, uint32_t winc,
                              uint32_t binc, uint32_t movestogo);
