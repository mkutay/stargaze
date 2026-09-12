#include "stargaze/eval.hpp"

#include "stargaze/search.hpp"
#include <algorithm>
#include <chrono>
#include <optional>

void Search::halve_history() {
    for (auto &from : history_table) {
        for (int &score : from)
            score /= 2;
    }
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
    stop_reached = false;
    killers.assign(MAX_SEARCH_DEPTH, std::array<Move, 2>{});
    halve_history();
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
        PVLine pv_line(depth);
        Score score = 0;
        Score alpha = ALPHA_START;
        Score beta = BETA_START;

        if (depth <= 2) {
            score = alpha_beta<true>(alpha, beta, depth, 0, &pv_line, true);
        } else {
            auto delta = ASPIRATION_WINDOW;
            alpha = prev_score - delta;
            beta = prev_score + delta;

            while (!should_stop()) {
                // Ensure that alpha and beta are within the initial bounds to
                // avoid searching outside the valid score range.
                alpha = std::max(alpha, ALPHA_START);
                beta = std::min(beta, BETA_START);

                score = alpha_beta<true>(alpha, beta, depth, 0, &pv_line, true);

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

    if (!(nodes_searched & 0x3FF)) { // check every 1024 nodes
        const auto current_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                Clock::now().time_since_epoch())
                .count();

        if (current_ms >= deadline_ms.load(std::memory_order_relaxed))
            return stop_reached = true;
    }

    return false;
}

template <bool AllowRepetition>
Score Search::alpha_beta(Score alpha, Score beta, uint16_t depth_left,
                         uint16_t ply, PVLine *pline, bool follow_pv) {
    if (should_stop())
        return alpha;
    nodes_searched++;

    bool in_check = board->in_check();
    if constexpr (AllowRepetition) {
        if (board->is_draw() || (ply > 0 && board->is_repetition())) {
            if (in_check && !board->has_legal_move())
                return Score::mated(ply);
            return 0;
        }
    } else if (board->get_halfmove_clock() >= 100 ||
               board->is_insufficient_material()) {
        if (in_check && !board->has_legal_move())
            return Score::mated(ply);
        return 0;
    }

    if (ply >= MAX_SEARCH_DEPTH - 1)
        return board->evaluate();

    // increase depth to avoid immediate checkmate
    if (in_check && ply < MAX_SEARCH_DEPTH - 1)
        depth_left++;

    bool is_pv_node = (beta - alpha) > 1;

    std::optional<Move> tt_move = std::nullopt;
    TTEntry *tt_entry = tt.probe(board->get_hash());
    const bool tt_scores_valid =
        AllowRepetition && tt_entry != nullptr &&
        tt_entry->halfmove_clock == board->get_halfmove_clock();

    if (tt_entry != nullptr) {
        tt_move = tt_entry->best_move;
        auto tt_score = tt_entry->score.from_tt(ply);

        // PV nodes require a full search to find the best move
        if (tt_scores_valid && tt_entry->depth >= depth_left && !is_pv_node &&
            ((tt_entry->bound == Bound::EXACT) ||
             (tt_entry->bound == Bound::LOWER && tt_score >= beta) ||
             (tt_entry->bound == Bound::UPPER && tt_score <= alpha))) {
            return tt_score;
        }
    }

    if (depth_left == 0)
        return quiescence<AllowRepetition, false>(alpha, beta, ply);

    // Static Evaluation for Pruning Heuristics
    Score static_eval = board->evaluate();
    if (tt_entry != nullptr && tt_scores_valid) {
        Score tt_score = tt_entry->score.from_tt(ply);
        if ((tt_entry->bound == Bound::EXACT) ||
            (tt_entry->bound == Bound::LOWER && tt_score > static_eval) ||
            (tt_entry->bound == Bound::UPPER && tt_score < static_eval)) {
            static_eval = tt_score;
        }
    }

    // Reverse Futility Pruning (Static Null Move Pruning)
    if (depth_left <= 3 && !is_pv_node && !in_check && !static_eval.is_mate()) {
        int margin = depth_left * PAWN_VALUE;
        if (static_eval - margin >= beta) {
            return static_eval - margin;
        }
    }

    // Null Move Pruning
    if (depth_left >= 3 && !is_pv_node && !in_check &&
        board->has_non_pawn_material(board->get_turn())) {
        int reduction = depth_left / 6;
        int next_depth =
            std::max(0, static_cast<int>(depth_left) - 3 - reduction);

        board->make_null_move();
        PVLine null_line(next_depth);
        auto nmp_score = -alpha_beta<false>(-beta, -beta + 1, next_depth,
                                            ply + 1, &null_line, false);
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

    std::vector<Move> moves = board->get_moves();

    if (ply == 0 && !search_moves.empty()) {
        std::erase_if(moves, [this](Move move) {
            return std::find(search_moves.begin(), search_moves.end(), move) ==
                   search_moves.end();
        });
    }

    std::vector<int> scores;
    score_moves(moves, scores, pv_move, tt_move, ply);

    bool found_pv = false;
    Bound bound = Bound::UPPER;

    for (size_t i = 0; i < moves.size() && !should_stop(); i++) {
        pick_next_move(moves, scores, i);

        Move move = moves[i];
        bool is_quiet = move.is_quiet();

        // Futility Pruning
        if (depth_left <= 2 && is_quiet && !in_check &&
            !static_eval.is_mate()) {
            int fp_margin = depth_left * PAWN_VALUE * 2;
            if (static_eval + fp_margin < alpha) {
                continue;
            }
        }

        board->make_move(move);
        bool gives_check = board->in_check();
        Score move_score;

        bool next_follow_pv = follow_pv && move == pv_move;

        PVLine line(depth_left - 1);

        if (!found_pv) {
            // Full window search for first move.
            move_score = -alpha_beta<AllowRepetition>(
                -beta, -alpha, depth_left - 1, ply + 1, &line, next_follow_pv);
        } else {
            // Late Move Reductions (LMR)
            if (depth_left >= 3 && i >= 3 && is_quiet && !in_check &&
                !gives_check) {
                int reduction = 2 + (i > 6) + (depth_left > 6);
                int reduced_depth = std::max(0, depth_left - reduction);

                move_score = -alpha_beta<AllowRepetition>(
                    -alpha - 1, -alpha, reduced_depth, ply + 1, &line,
                    next_follow_pv);

                if (move_score > alpha) {
                    // Re-search at full depth with narrow window when the
                    // reduced search fails high.
                    line.moves.clear();
                    move_score = -alpha_beta<AllowRepetition>(
                        -alpha - 1, -alpha, depth_left - 1, ply + 1, &line,
                        next_follow_pv);
                }
            } else {
                // Regular null window search when the move is not reduced.
                move_score = -alpha_beta<AllowRepetition>(
                    -alpha - 1, -alpha, depth_left - 1, ply + 1, &line,
                    next_follow_pv);
            }

            // Re-search with full window if the move is actually better; i.e.,
            // PV node.
            if (move_score > alpha && move_score < beta) {
                line.moves.clear();
                move_score = -alpha_beta<AllowRepetition>(
                    -beta, -alpha, depth_left - 1, ply + 1, &line,
                    next_follow_pv);
            }
        }

        board->undo_move();

        if (should_stop())
            break;

        if (move_score > alpha) {
            alpha = move_score;
            found_pv = true;
            bound = Bound::EXACT;

            pline->moves.clear();
            pline->moves.emplace_back(move);
            pline->moves.insert(pline->moves.end(), line.moves.begin(),
                                line.moves.end());
        }

        if (move_score >= beta) {
            bound = Bound::LOWER;
            if (is_quiet)
                record_cutoff(move, ply, depth_left);
            break;
        }
    }

    // Store best move (or first legal move if no improvement).
    if (pline->moves.empty() && !moves.empty()) {
        pline->moves.emplace_back(moves.front());
    }

    if (!pline->moves.empty()) {
        if (!should_stop() && AllowRepetition) {
            Score tt_score = alpha.to_tt(ply);
            tt.store(board->get_hash(), pline->moves.front(), tt_score,
                     depth_left, board->get_halfmove_clock(), bound);
        }
        return alpha;
    }

    // No legal moves now, so: checkmate or stalemate.

    if (in_check) {
        alpha = Score::mated(ply);
    } else {
        alpha = 0;
    }

    return alpha;
}

template <bool AllowRepetition, bool CountNodes>
Score Search::quiescence(Score alpha, Score beta, uint16_t ply) {
    if (should_stop())
        return alpha;
    if (CountNodes)
        nodes_searched++;

    if (ply >= MAX_SEARCH_DEPTH - 1)
        return board->evaluate();

    const bool in_check = board->in_check();
    const bool is_draw =
        AllowRepetition
            ? board->is_draw() || (ply > 0 && board->is_repetition())
            : board->get_halfmove_clock() >= 100 ||
                  board->is_insufficient_material();
    if (is_draw) {
        if (in_check && !board->has_legal_move())
            return Score::mated(ply);
        return 0;
    }

    Score stand_pat = board->evaluate();
    if (!in_check) {
        if (stand_pat >= beta)
            return stand_pat;
        alpha = std::max(alpha, stand_pat);
    }

    std::vector<Move> moves =
        in_check ? board->get_moves()
                 : board->get_moves<true, false, false, true, false>();
    std::vector<int> scores;
    score_moves(moves, scores);

    for (size_t i = 0; i < moves.size() && !should_stop(); i++) {
        pick_next_move(moves, scores, i);

        Move move = moves[i];
        // Delta Pruning
        if (!in_check && !move.is_promotion()) {
            int gain = move.is_en_passant()
                           ? Eval::value(Piece::PAWN)
                           : Eval::value(*board->get_piece(move.to()));
            if (stand_pat + gain + DELTA_PRUNING < alpha)
                continue;
        }

        board->make_move(move);
        Score score =
            -quiescence<AllowRepetition, CountNodes>(-beta, -alpha, ply + 1);
        board->undo_move();

        if (should_stop())
            break;

        if (score >= beta)
            return score;

        alpha = std::max(alpha, score);
    }

    if (in_check && moves.empty())
        return Score::mated(ply);

    return alpha;
}
