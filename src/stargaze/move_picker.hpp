#pragma once

#include "stargaze/bitboard.hpp"
#include "stargaze/colour.hpp"
#include "stargaze/move.hpp"
#include "stargaze/piece.hpp"
#include "stargaze/search_history.hpp"
#include <array>
#include <optional>
#include <span>

class Board;
class Search;

class MovePicker {
    constexpr static const int PV_MOVE_SCORE = 900000;
    constexpr static const int TT_MOVE_SCORE = 800000;
    constexpr static const int GOOD_TACTICAL_SCORE = 700000;
    constexpr static const std::array<int, 2> KILLER_SCORES = {600000, 590000};
    constexpr static const int COUNTER_MOVE_SCORE = 580000;
    constexpr static const int QUIET_SCORE = 300000;
    constexpr static const int BAD_TACTICAL_SCORE = 100000;

    static BitBoard least_valuable_attacker(const Board &board,
                                            BitBoard attackers, Colour side,
                                            Square target, BitBoard occupied,
                                            Piece &piece);
    static int score_move(const Search &search, Move move, int see,
                          std::optional<Move> pv_move,
                          std::optional<Move> tt_move,
                          std::optional<uint16_t> ply,
                          SearchHistory::OptionalContext previous_context);

  public:
    constexpr static const size_t MAX_MOVES = 256;

    struct ScoredMove {
        int score;
        Move move;
        bool loses_exchange;
    };

  private:
    static_assert(sizeof(ScoredMove) == 8);

    std::array<ScoredMove, MAX_MOVES> moves{};
    size_t count = 0;
    size_t index = 0;

  public:
    MovePicker(
        Search &search, bool tactical_only,
        std::optional<Move> pv_move = std::nullopt,
        std::optional<Move> tt_move = std::nullopt,
        std::optional<uint16_t> ply = std::nullopt,
        std::optional<std::span<const Move>> allowed_moves = std::nullopt,
        SearchHistory::OptionalContext previous_context = std::nullopt);

    std::optional<ScoredMove> next();
    bool empty() const { return count == 0; }

    static int see(const Board &board, Move move);
};
