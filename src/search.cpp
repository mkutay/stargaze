#include "stargaze/eval.hpp"

#include "stargaze/search.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <optional>

SearchHistory::Context Search::history_context(Move move) const {
    const Piece piece = *board->get_piece(move.from());
    const Colour turn = board->get_turn();
    return SearchHistory::context(turn, piece, move.to());
}

void Search::record_cutoff(Move move, SearchHistory::Context context,
                           uint16_t ply, uint16_t depth,
                           SearchHistory::OptionalContext previous_context) {
    if (killers[ply][0] != move) {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = move;
    }

    history.record_cutoff(context, previous_context, move, 32 * depth * depth);
}

template <Search::NodeType Node>
int Search::lmr_reduction(uint16_t depth, size_t move_number, int history_score,
                          bool improving) const {
    static const auto reductions = [] {
        std::array<std::array<uint8_t, MovePicker::MAX_MOVES + 1>,
                   MAX_SEARCH_DEPTH + 1>
            table{};
        for (double d = 3; d <= MAX_SEARCH_DEPTH; d++) {
            for (double move = 4; move <= MovePicker::MAX_MOVES; move++) {
                table[d][move] = static_cast<uint8_t>(
                    1 + std::log(d) * std::log(move) / 2.5);
            }
        }
        return table;
    }();

    assert(depth <= MAX_SEARCH_DEPTH);
    assert(move_number <= MovePicker::MAX_MOVES);
    auto reduction = static_cast<int>(reductions[depth][move_number]);

    reduction -= Node == NodeType::PV;
    reduction -= improving;
    reduction += Node == NodeType::CUT;
    reduction -= history_score > SearchHistory::MAX_SCORE / 4;
    reduction += history_score < -SearchHistory::MAX_SCORE / 4;
    return std::clamp(reduction, 0, static_cast<int>(depth) - 1);
}

template <bool AllowRepetition>
std::optional<Search::TTProbe> Search::probe_tt(uint16_t ply) {
    TTEntry *entry = tt.probe(board->get_hash());
    if (entry == nullptr)
        return std::nullopt;

    const bool score_valid =
        AllowRepetition && entry->halfmove_clock == board->get_halfmove_clock();

    const auto score =
        score_valid ? std::optional(entry->score.from_tt(ply)) : std::nullopt;

    return TTProbe{score, entry->best_move, entry->depth, entry->bound};
}

template <bool AllowRepetition>
std::optional<Score> Search::draw_score(uint16_t ply, bool in_check) {
    const bool is_draw =
        AllowRepetition
            ? board->is_draw() || (ply > 0 && board->is_repetition())
            : board->get_halfmove_clock() >= 100 ||
                  board->is_insufficient_material();

    if (!is_draw)
        return std::nullopt;

    return in_check && !board->has_legal_move() ? Score::mated(ply) : Score{0};
}

std::optional<Move> Search::get_fallback_move() {
    if (search_moves.empty()) {
        return board->get_move();
    }
    return search_moves.front();
}

SearchInfo Search::iterative_deepening(
    uint16_t max_depth, uint32_t time_limit_ms,
    const std::function<void(const SearchInfo &)> &on_iteration,
    const std::function<void()> &on_start) {
    max_depth = std::min(max_depth, MAX_SEARCH_DEPTH);
    SearchInfo search_info(max_depth);

    start_time = Clock::now();
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            start_time.time_since_epoch())
                            .count();
    deadline_ms.store(now_ms + time_limit_ms, std::memory_order_relaxed);

    nodes_searched = 0;
    stop_check_count = 0;
    stop_reached = false;
    killers.fill(std::array<Move, 2>{});
    for (auto &line : pv_table)
        line.clear();
    static_evals.fill(std::nullopt);
    last_pv = PVLine{max_depth};

    auto fallback_move = get_fallback_move();
    if (fallback_move) {
        search_info.pv.moves.emplace_back(*fallback_move);
        last_pv = search_info.pv;
    }

    if (on_start)
        on_start();

    tt.new_search();

    // for aspiration window search; centered around this score
    Score prev_score = 0;
    for (uint16_t depth = 1; depth <= max_depth && !should_stop(); depth++) {
        Score score = 0;
        Score alpha = ALPHA_START;
        Score beta = BETA_START;

        if (depth <= 2) {
            score = alpha_beta<NodeType::PV, true>(alpha, beta, depth, 0, true,
                                                   std::nullopt);
        } else {
            auto delta = ASPIRATION_WINDOW;
            alpha = prev_score - delta;
            beta = prev_score + delta;

            while (!should_stop()) {
                // Ensure that alpha and beta are within the initial bounds to
                // avoid searching outside the valid score range.
                alpha = std::max(alpha, ALPHA_START);
                beta = std::min(beta, BETA_START);

                score = alpha_beta<NodeType::PV, true>(alpha, beta, depth, 0,
                                                       true, std::nullopt);

                if (should_stop())
                    break;

                if (score <= alpha) { // failed low
                    if (alpha == ALPHA_START)
                        break;
                    alpha -= delta;
                    delta *= 2;
                    if (delta > DELTA_MAX)
                        alpha = ALPHA_START;
                } else if (score >= beta) { // failed high
                    if (beta == BETA_START)
                        break;
                    beta += delta;
                    delta *= 2;
                    if (delta > DELTA_MAX)
                        beta = BETA_START;
                } else {
                    break; // successful search
                }
            }
        }

        if (should_stop())
            break;

        prev_score = score;

        search_info.depth = depth;
        search_info.score = score;
        search_info.nodes = nodes_searched;
        search_info.time_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                                  start_time)
                .count();
        search_info.hashfull = tt.hashfull();
        PVLine pv_line(MAX_SEARCH_DEPTH);
        pv_line.moves = pv_table[0];
        if (!pv_line.moves.empty()) {
            search_info.pv = pv_line;
            last_pv = pv_line;
        }

        if (on_iteration)
            on_iteration(search_info);
    }

    search_info.nodes = nodes_searched;
    search_info.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              Clock::now() - start_time)
                              .count();
    search_info.hashfull = tt.hashfull();
    search_info.stopped = stop_reached;
    return search_info;
}

bool Search::should_stop() {
    if (stop_reached)
        return true;

    if (nodes_searched >= node_limit)
        return stop_reached = true;

    if (stop_flag.load(std::memory_order_relaxed))
        return stop_reached = true;

    if (!(stop_check_count++ & 0x3FF)) {
        const auto current_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                Clock::now().time_since_epoch())
                .count();

        if (current_ms >= deadline_ms.load(std::memory_order_relaxed))
            return stop_reached = true;
    }

    return false;
}

template <Search::NodeType Node, bool AllowRepetition>
Score Search::alpha_beta(Score alpha, Score beta, uint16_t depth_left,
                         uint16_t ply, bool follow_pv,
                         SearchHistory::OptionalContext previous_context) {
    if (should_stop())
        return alpha;
    constexpr NodeType full_child_node =
        Node == NodeType::PV ? NodeType::PV : NodeType::ALL;
    constexpr NodeType narrow_child_node =
        Node == NodeType::CUT ? NodeType::ALL : NodeType::CUT;
    assert((beta - alpha) > 1 == (Node == NodeType::PV));

    nodes_searched++;
    pv_table[ply].clear();

    const bool in_check = board->in_check();
    if (const auto score = draw_score<AllowRepetition>(ply, in_check))
        return *score;

    if (ply >= MAX_SEARCH_DEPTH - 1)
        return board->evaluate();

    // increase depth to avoid immediate checkmate
    if (in_check && ply < MAX_SEARCH_DEPTH - 1)
        depth_left++;

    const auto tt_result = probe_tt<AllowRepetition>(ply);
    if constexpr (Node != NodeType::PV) {
        if (tt_result && tt_result->depth >= depth_left &&
            tt_result->can_cutoff(alpha, beta))
            return *tt_result->score;
    }

    if (depth_left == 0)
        return quiescence<AllowRepetition, false>(alpha, beta, ply,
                                                  previous_context);

    // Static Evaluation for Pruning Heuristics
    Score static_eval = board->evaluate();
    if (tt_result && tt_result->score) {
        auto bound = tt_result->bound;
        auto score = *tt_result->score;
        if ((bound == Bound::EXACT) ||
            (bound == Bound::LOWER && score > static_eval) ||
            (bound == Bound::UPPER && score < static_eval)) {
            static_eval = score;
        }
    }

    const bool improving = ply >= 2 && static_evals[ply - 2] &&
                           static_eval > *static_evals[ply - 2];
    static_evals[ply] = static_eval;

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (depth_left <= 3 && Node != NodeType::PV && !in_check &&
        !static_eval.is_mate()) {
        int margin = depth_left * PAWN_VALUE;
        if (static_eval - margin >= beta) {
            return static_eval - margin;
        }
    }

    // Null Move Pruning
    if (depth_left >= 3 && Node != NodeType::PV && !in_check &&
        board->has_non_pawn_material(board->get_turn())) {
        int reduction = depth_left / 6;
        int next_depth =
            std::max(0, static_cast<int>(depth_left) - 3 - reduction);

        board->make_null_move();
        tt.prefetch(board->get_hash());
        auto nmp_score = -alpha_beta<NodeType::CUT, false>(
            -beta, -beta + 1, next_depth, ply + 1, false, std::nullopt);
        board->undo_null_move();

        if (should_stop())
            return alpha;

        if (nmp_score >= beta && nmp_score < BETA_START) // failed high, prune
            return beta;
    }

    std::optional<Move> pv_move = std::nullopt;
    if (follow_pv && ply < last_pv.moves.size()) {
        pv_move = last_pv.moves[ply];
    }

    std::optional<std::span<const Move>> allowed_moves;
    if (ply == 0 && !search_moves.empty())
        allowed_moves = search_moves;

    MovePicker move_picker(*this, false, pv_move,
                           tt_result ? tt_result->best_move : std::nullopt, ply,
                           allowed_moves, previous_context);

    Bound bound = Bound::UPPER;
    size_t moves_searched = 0;
    std::optional<Move> first_move;
    std::vector<SearchHistory::Context> searched_quiets{};
    searched_quiets.reserve(MovePicker::MAX_MOVES);

    while (!should_stop()) {
        const auto picked = move_picker.next();
        if (!picked)
            break;

        Move move = picked->move;
        if (!first_move)
            first_move = move;
        const SearchHistory::Context move_context = history_context(move);
        auto quiet_history =
            move.is_quiet()
                ? std::optional(history.score(move_context, previous_context))
                : std::nullopt;

        // Late-move pruning is deliberately limited to shallow non-PV nodes.
        if (depth_left <= 3 && Node != NodeType::PV && !in_check &&
            quiet_history.has_value() && !alpha.is_mate() &&
            *quiet_history < SearchHistory::MAX_SCORE / 2) {
            const size_t lmp_limit =
                3 + depth_left * depth_left + (improving ? 2 : 0);
            if (moves_searched >= lmp_limit && !board->gives_check(move))
                continue;
        }

        // Futility Pruning
        if (depth_left <= 2 && quiet_history.has_value() && !in_check &&
            !static_eval.is_mate()) {
            int fp_margin = depth_left * PAWN_VALUE * 2;
            if (static_eval + fp_margin < alpha && !board->gives_check(move)) {
                continue;
            }
        }

        board->make_move(move);
        tt.prefetch(board->get_hash());
        bool gives_check = board->in_check();
        Score move_score;

        bool next_follow_pv = follow_pv && move == pv_move;

        pv_table[ply + 1].clear();

        if (moves_searched == 0) {
            // Full window search for first move.
            move_score = -alpha_beta<full_child_node, AllowRepetition>(
                -beta, -alpha, depth_left - 1, ply + 1, next_follow_pv,
                move_context);
        } else {
            // Late Move Reductions (LMR)
            if (depth_left >= 3 && moves_searched >= 3 &&
                quiet_history.has_value() && !in_check && !gives_check) {
                const int reduction = lmr_reduction<Node>(
                    depth_left, moves_searched + 1, *quiet_history, improving);
                const int reduced_depth =
                    std::max(0, static_cast<int>(depth_left) - 1 - reduction);

                move_score = -alpha_beta<NodeType::CUT, AllowRepetition>(
                    -alpha - 1, -alpha, reduced_depth, ply + 1, next_follow_pv,
                    move_context);

                if (move_score > alpha) {
                    // Re-search at full depth with narrow window when the
                    // reduced search fails high.
                    pv_table[ply + 1].clear();
                    move_score =
                        -alpha_beta<narrow_child_node, AllowRepetition>(
                            -alpha - 1, -alpha, depth_left - 1, ply + 1,
                            next_follow_pv, move_context);
                }
            } else {
                // Regular null window search when the move is not reduced.
                move_score = -alpha_beta<narrow_child_node, AllowRepetition>(
                    -alpha - 1, -alpha, depth_left - 1, ply + 1, next_follow_pv,
                    move_context);
            }

            // Re-search with full window if the move is actually better; i.e.,
            // PV node.
            if (move_score > alpha && move_score < beta) {
                pv_table[ply + 1].clear();
                move_score = -alpha_beta<full_child_node, AllowRepetition>(
                    -beta, -alpha, depth_left - 1, ply + 1, next_follow_pv,
                    move_context);
            }
        }

        board->undo_move();

        if (should_stop())
            break;

        moves_searched++;

        if (move_score > alpha) {
            alpha = move_score;
            bound = Bound::EXACT;

            pv_table[ply].clear();
            pv_table[ply].emplace_back(move);
            for (Move child_move : pv_table[ply + 1])
                pv_table[ply].emplace_back(child_move);
        }

        if (move_score >= beta) {
            bound = Bound::LOWER;
            if (quiet_history.has_value()) {
                const int malus = -16 * depth_left * depth_left;
                for (auto &context : searched_quiets)
                    history.update(context, malus, previous_context);
                record_cutoff(move, move_context, ply, depth_left,
                              previous_context);
            }
            break;
        }

        if (quiet_history.has_value())
            searched_quiets.emplace_back(move_context);
    }

    // Store best move (or first legal move if no improvement).
    if (pv_table[ply].empty() && first_move)
        pv_table[ply].emplace_back(*first_move);

    if (!pv_table[ply].empty()) {
        if (!should_stop() && AllowRepetition) {
            Score tt_score = alpha.to_tt(ply);
            tt.store(board->get_hash(), pv_table[ply].front(), tt_score,
                     depth_left, board->get_halfmove_clock(), bound);
        }
        return alpha;
    }

    // No legal moves now, so: checkmate or stalemate.

    return in_check ? Score::mated(ply) : 0;
}

template <bool AllowRepetition, bool CountNodes>
Score Search::quiescence(Score alpha, Score beta, uint16_t ply,
                         SearchHistory::OptionalContext previous_context) {
    if (should_stop())
        return alpha;
    if (CountNodes)
        nodes_searched++;

    if (ply >= MAX_SEARCH_DEPTH - 1)
        return board->evaluate();

    const bool in_check = board->in_check();
    if (const auto score = draw_score<AllowRepetition>(ply, in_check))
        return *score;

    const Score original_alpha = alpha;
    const uint64_t hash = board->get_hash();
    const auto tt_result = probe_tt<AllowRepetition>(ply);
    if (tt_result && tt_result->can_cutoff(alpha, beta))
        return *tt_result->score;

    const auto store = [&](Score score, std::optional<Move> move, Bound bound) {
        if constexpr (AllowRepetition) {
            if (!should_stop()) {
                tt.store(hash, move, score.to_tt(ply), 0,
                         board->get_halfmove_clock(), bound);
            }
        }
    };

    Score stand_pat = board->evaluate();
    if (!in_check) {
        if (stand_pat >= beta) {
            if (!board->has_legal_move()) {
                store(0, std::nullopt, Bound::EXACT);
                return 0;
            }
            store(stand_pat, std::nullopt, Bound::LOWER);
            return stand_pat;
        }
        alpha = std::max(alpha, stand_pat);
    }

    MovePicker move_picker(*this, !in_check, std::nullopt,
                           tt_result ? tt_result->best_move : std::nullopt,
                           std::nullopt, std::nullopt, previous_context);
    if (move_picker.empty()) {
        if (in_check) {
            const Score mate = Score::mated(ply);
            store(mate, std::nullopt, Bound::EXACT);
        }
        if (!board->has_legal_move()) {
            store(0, std::nullopt, Bound::EXACT);
            return 0;
        }
    }

    std::optional<Move> best_move;
    while (!should_stop()) {
        const auto picked = move_picker.next();
        if (!picked)
            break;

        Move move = picked->move;
        if (!in_check && !move.is_promotion()) {
            int gain = move.is_en_passant()
                           ? Eval::value(Piece::PAWN)
                           : Eval::value(*board->get_piece(move.to()));
            const bool fails_delta = stand_pat + gain + DELTA_PRUNING < alpha;
            if ((picked->loses_exchange || fails_delta) &&
                !board->gives_check(move))
                continue;
        }

        const SearchHistory::Context move_context = history_context(move);
        board->make_move(move);
        tt.prefetch(board->get_hash());
        Score score = -quiescence<AllowRepetition, true>(-beta, -alpha, ply + 1,
                                                         move_context);
        board->undo_move();

        if (should_stop())
            break;

        if (score >= beta) {
            store(score, move, Bound::LOWER);
            return score;
        }

        if (score > alpha) {
            alpha = score;
            best_move = move;
        }
    }

    store(alpha, best_move,
          alpha > original_alpha ? Bound::EXACT : Bound::UPPER);
    return alpha;
}
