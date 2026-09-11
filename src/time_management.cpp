#include "time_management.hpp"
#include "board.hpp"
#include "search.hpp"
#include <algorithm>

uint32_t calculate_time_limit(const Board &board, const Search &search,
                              uint32_t wtime, uint32_t btime, uint32_t winc,
                              uint32_t binc, uint32_t movestogo) {
    (void)search;

    const uint32_t time_left =
        (board.get_turn() == Colour::WHITE) ? wtime : btime;
    const uint32_t inc = (board.get_turn() == Colour::WHITE) ? winc : binc;
    const uint32_t moves_played = board.get_move_history().size() / 2;

    // Estimate remaining moves using expected total game length of 40 moves
    // (80 plies); fall back to 5 moves once past that mark.
    const uint32_t moves_remaining =
        (movestogo > 0) ? movestogo
                        : ((moves_played >= 40) ? 5u : (40u - moves_played));

    uint32_t target_time = time_left / moves_remaining + inc * 4 / 5;
    const uint32_t max_time =
        (time_left < 1000) ? (time_left * 3 / 10) : (time_left / 2);
    const uint32_t min_time =
        std::min(std::max(10u, time_left / 100), time_left);

    target_time = std::clamp(target_time, min_time, max_time);
    return std::max(target_time, 1u);
}
