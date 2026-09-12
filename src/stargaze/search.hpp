#pragma once
#include "stargaze/board.hpp"
#include "stargaze/eval.hpp"
#include "stargaze/move.hpp"
#include "stargaze/score.hpp"
#include "stargaze/tt.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <limits>
#include <vector>

struct PVLine {
    std::vector<Move> moves;
    PVLine(int max_depth) { moves.reserve(max_depth); }
    PVLine() {}
};

struct SearchInfo {
    bool stopped;
    uint16_t depth;
    uint64_t nodes;
    uint32_t time_ms;
    Score score;
    PVLine pv;
    SearchInfo(int max_depth)
        : stopped(false), depth(0), nodes(0), time_ms(0), score(0),
          pv(max_depth) {}
};

class Search {
  public:
    constexpr static const uint16_t MAX_SEARCH_DEPTH = 64;

  private:
    constexpr static const int PAWN_VALUE = Eval::value(Piece::PAWN);

    constexpr static const Score ALPHA_START = -Score::INFINITY_SCORE;
    constexpr static const Score BETA_START = Score::INFINITY_SCORE;
    constexpr static const int ASPIRATION_WINDOW = PAWN_VALUE / 4;

    // If the delta becomes too large in iterative deepening, reset alpha/beta
    // to the initial starting value to avoid searching too far below the
    // expected score.
    constexpr static const int DELTA_MAX = PAWN_VALUE * 5;

    constexpr static const int DELTA_PRUNING = PAWN_VALUE * 2;

    constexpr static const int PV_MOVE_SCORE = 200000;
    constexpr static const int TT_MOVE_SCORE = 100000;
    constexpr static const int PROMOTION_SCORE = 90000;
    constexpr static const int CAPTURE_SCORE_BASE = 70000;
    constexpr static const int CASTLE_SCORE = 60000;
    constexpr static const std::array<int, 2> KILLER_SCORES = {50000, 40000};

    using Clock = std::chrono::steady_clock;

    bool stop_reached = false;
    std::atomic<int64_t> deadline_ms{std::numeric_limits<int64_t>::max()};
    uint64_t node_limit = std::numeric_limits<uint64_t>::max();
    uint64_t nodes_searched;
    Board *board;
    std::chrono::time_point<Clock> start_time;
    TT tt;

    PVLine last_pv;

    std::vector<std::array<Move, 2>> killers;
    // searchmoves as defined by UCI protocol
    std::vector<Move> search_moves;
    std::array<std::array<int, 64>, 64> history_table{};

    template <bool AllowRepetition, bool CountNodes = true>
    Score quiescence(Score alpha, Score beta, uint16_t ply);
    bool should_stop();
    int score_move(Move move, std::optional<Move> pv_move = std::nullopt,
                   std::optional<Move> tt_move = std::nullopt,
                   std::optional<uint16_t> ply = std::nullopt) const;
    void score_moves(const std::vector<Move> &moves, std::vector<int> &scores,
                     std::optional<Move> pv_move = std::nullopt,
                     std::optional<Move> tt_move = std::nullopt,
                     std::optional<uint16_t> ply = std::nullopt) const;
    void pick_next_move(std::vector<Move> &moves, std::vector<int> &scores,
                        size_t index) const;
    void record_cutoff(Move move, uint16_t ply, uint16_t depth);
    template <bool AllowRepetition>
    Score alpha_beta(Score alpha, Score beta, uint16_t depth_left, uint16_t ply,
                     PVLine *pline, bool follow_pv);
    void halve_history();
    std::optional<Move> get_fallback_move();

  public:
    /**
     * Variable to communicate with main thread to stop the search.
     */
    std::atomic<bool> stop_flag{false};

    Search() = delete;
    explicit Search(Board *board) : board(board) { assert(board != nullptr); }

    void clear_tt() { tt.clear(); }
    void resize_tt(size_t megabytes) { tt.resize(megabytes); }
    const PVLine &get_last_pv() const { return last_pv; }
    void set_time_limit(uint32_t limit_ms) {
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                             Clock::now().time_since_epoch())
                             .count();
        deadline_ms.store(now + limit_ms, std::memory_order_relaxed);
    }
    void set_limits(uint64_t nodes = std::numeric_limits<uint64_t>::max(),
                    std::vector<Move> moves = {}) {
        node_limit = nodes;
        search_moves = std::move(moves);
    }
    SearchInfo iterative_deepening(
        uint16_t max_depth, uint32_t time_limit_ms,
        const std::function<void(const SearchInfo &)> &on_iteration = {},
        const std::function<void()> &on_start = {});
};
