#pragma once
#include "stargaze/board.hpp"
#include "stargaze/eval.hpp"
#include "stargaze/move.hpp"
#include "stargaze/move_picker.hpp"
#include "stargaze/score.hpp"
#include "stargaze/tt.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <limits>
#include <optional>
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
    friend class MovePicker;

  public:
    constexpr static const uint16_t MAX_SEARCH_DEPTH = 64;

  private:
    constexpr static const int PAWN_VALUE = Eval::value(Piece::PAWN);
    constexpr static const int HISTORY_MAX = 16384;

    constexpr static const Score ALPHA_START = -Score::INFINITY_SCORE;
    constexpr static const Score BETA_START = Score::INFINITY_SCORE;
    constexpr static const int ASPIRATION_WINDOW = PAWN_VALUE / 4;

    // If the delta becomes too large in iterative deepening, reset alpha/beta
    // to the initial starting value to avoid searching too far below the
    // expected score.
    constexpr static const int DELTA_MAX = PAWN_VALUE * 5;

    constexpr static const int DELTA_PRUNING = PAWN_VALUE * 2;

    using Clock = std::chrono::steady_clock;

    bool stop_reached = false;
    std::atomic<int64_t> deadline_ms{std::numeric_limits<int64_t>::max()};
    uint64_t node_limit = std::numeric_limits<uint64_t>::max();
    uint64_t nodes_searched;
    uint32_t stop_check_count = 0;
    Board *board;
    std::chrono::time_point<Clock> start_time;
    TT tt;

    PVLine last_pv;

    std::vector<std::array<Move, 2>> killers;
    // searchmoves as defined by UCI protocol
    std::vector<Move> search_moves;
    std::array<std::array<std::array<int, 64>, 6>, 2> history_table{};

    template <bool AllowRepetition, bool CountNodes = true>
    Score quiescence(Score alpha, Score beta, uint16_t ply);
    bool should_stop();
    int history_score(Move move) const;
    void update_history(Move move, int bonus);
    void record_cutoff(Move move, uint16_t ply, uint16_t depth);
    int lmr_reduction(uint16_t depth, size_t move_number, int history,
                      bool pv_node) const;
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
