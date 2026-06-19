#include "eval.hpp"

#include "search.hpp"
#include <algorithm>
#include <chrono>
#include <optional>
#include <print>

#ifdef DEBUG
#include "debug.hpp"
#else
#define debug(...) void(38)
#endif

template <bool UCI>
SearchInfo Search::iterative_deepening(uint16_t max_depth,
                                       uint32_t _time_limit_ms) {
    SearchInfo search_info(max_depth);
    time_limit_ms = _time_limit_ms;
    start_time = std::chrono::high_resolution_clock::now();
    nodes_searched = 0;
    time_up = false;
    killers.assign(max_depth, std::array<Move, 2>{});
    last_pv = PVLine(max_depth);

    tt.new_search();

    // The previous score is used for aspiration window search, which will be
    // centered around this score.
    int prev_score = 0;

    for (uint16_t depth = 1; depth <= max_depth && !should_stop(); depth++) {
        PVLine pv_line(depth);
        int score = 0;
        int alpha = ALPHA_START;
        int beta = BETA_START;

        if (depth <= 2) {
            score = alpha_beta(alpha, beta, depth, 0, &pv_line, true);
        } else {
            auto delta = ASPIRATION_WINDOW;
            alpha = prev_score - delta;
            beta = prev_score + delta;

            while (!should_stop()) {
                // Ensure that alpha and beta are within the initial bounds to
                // avoid searching outside the valid score range.
                alpha = std::max(alpha, ALPHA_START);
                beta = std::min(beta, BETA_START);

                score = alpha_beta(alpha, beta, depth, 0, &pv_line, true);

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
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start_time)
                .count();
        search_info.pv = pv_line;
        last_pv = pv_line;

        if constexpr (UCI)
            print_uci_info(depth, score, search_info, pv_line);
        else
            debug(depth, alpha, score, beta, pv_line.moves);
    }

    search_info.stopped = time_up;
    return search_info;
}

template SearchInfo Search::iterative_deepening<true>(uint16_t max_depth,
                                                      uint32_t _time_limit_ms);
template SearchInfo Search::iterative_deepening<false>(uint16_t max_depth,
                                                       uint32_t _time_limit_ms);

void Search::print_uci_info(int depth, int score, const SearchInfo &info,
                            const PVLine &pv_line) const {
    std::print("info depth {}", depth);
    if (std::abs(score) >= CHECKMATE_COMPARE &&
        std::abs(score) <= -CHECKMATE_SCORE) {
        auto mate_plies = (score > 0) ? (-CHECKMATE_SCORE - score)
                                      : (score - CHECKMATE_SCORE);
        auto mate_moves =
            (score > 0) ? ((mate_plies + 1) / 2) : -((mate_plies + 1) / 2);
        std::print(" score mate {}", mate_moves);
    } else {
        std::print(" score cp {}", score);
    }
    std::print(" nodes {} time {} nps {} pv", nodes_searched, info.time_ms,
               info.time_ms > 0 ? (nodes_searched * 1000 / info.time_ms) : 0);
    for (auto m : pv_line.moves) {
        std::print(" {}", m.to_string());
    }
    std::print("\n");
}

bool Search::should_stop() {
    if (time_up)
        return true;

    if (stop_flag.load(std::memory_order_relaxed)) {
        return time_up = true;
    }

    if (!(nodes_searched & 0x3FF)) { // check every 1024 nodes
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           current_time - start_time)
                           .count();

        if (elapsed >= time_limit_ms)
            return time_up = true;
    }

    return false;
}

int Search::score_move(Move move, std::optional<Move> pv_move,
                       std::optional<Move> tt_move,
                       std::optional<uint16_t> ply) const {
    if (move == pv_move)
        return PV_MOVE_SCORE;

    if (move == tt_move)
        return TT_MOVE_SCORE;

    if (move.is_promotion())
        return PROMOTION_SCORE;

    if (move.is_capture()) {
        auto victim_val = move.is_en_passant()
                              ? Eval::value(Piece::PAWN)
                              : Eval::value(*board->get_piece(move.to()));
        auto aggressor_val = Eval::value(*board->get_piece(move.from()));
        return CAPTURE_SCORE_BASE + 10 * victim_val - aggressor_val;
    }

    if (move.is_castle())
        return CASTLE_SCORE;

    if (ply.has_value() && *ply < killers.size()) {
        auto &k_moves = killers[*ply];
        for (size_t i = 0; i < k_moves.size(); i++) {
            if (move == k_moves[i])
                return KILLER_SCORES[i];
        }
    }

    return 0;
}

void Search::order_moves(std::vector<Move> &moves, std::optional<Move> pv_move,
                         std::optional<Move> tt_move,
                         std::optional<uint16_t> ply) {
    std::stable_sort(moves.begin(), moves.end(),
                     [this, pv_move, tt_move, ply](Move a, Move b) {
                         return score_move(a, pv_move, tt_move, ply) >
                                score_move(b, pv_move, tt_move, ply);
                     });
}

int Search::alpha_beta(int alpha, int beta, uint16_t depth_left, uint16_t ply,
                       PVLine *pline, bool follow_pv) {
    nodes_searched++;
    if (should_stop())
        return alpha;

    if (board->is_draw())
        return 0;

    bool in_check = board->is_in_check(board->get_turn());
    if (in_check)
        depth_left++;

    bool is_pv_node = (beta - alpha) > 1;
    Move tt_move;

    auto hash = board->get_hash();
    TTEntry *tt_entry = tt.probe(hash);

    if (tt_entry != nullptr) {
        tt_move = Move(tt_entry->best_move);

        // Use TT score if depth is sufficient and not a PV node, as PV nodes
        // require a full search to find the best move.
        if (tt_entry->depth >= depth_left && !is_pv_node) {
            int tt_score = tt_entry->score;

            if (tt_score > CHECKMATE_COMPARE && tt_score <= -CHECKMATE_SCORE) {
                tt_score -= ply;
            } else if (tt_score < -CHECKMATE_COMPARE &&
                       tt_score >= CHECKMATE_SCORE) {
                tt_score += ply;
            }

            if ((tt_entry->bound == Bound::EXACT) ||
                (tt_entry->bound == Bound::LOWER && tt_score >= beta) ||
                (tt_entry->bound == Bound::UPPER && tt_score <= alpha)) {
                return tt_score;
            }
        }
    }

    if (depth_left == 0)
        return quiescence(alpha, beta);

    // Null Move Pruning
    if (depth_left >= 3 && !is_pv_node && !in_check &&
        board->has_non_pawn_material(board->get_turn())) {
        int reduction = depth_left / 6;
        int next_depth =
            std::max(0, static_cast<int>(depth_left) - 3 - reduction);

        board->make_null_move();
        PVLine null_line(next_depth);
        auto nmp_score = -alpha_beta(-beta, -beta + 1, next_depth, ply + 1,
                                     &null_line, false);
        board->undo_null_move();

        // If the null move search fails high, we can prune this branch.
        if (nmp_score >= beta && nmp_score < BETA_START)
            return beta;
    }

    std::optional<Move> pv_move = std::nullopt;
    if (follow_pv && ply < last_pv.moves.size()) {
        pv_move = last_pv.moves[ply];
    }

    std::vector<Move> moves = board->get_moves<false>();

    order_moves(moves, pv_move, tt_move, ply);

    bool found_pv = false;
    Bound bound = Bound::UPPER;

    for (size_t i = 0; i < moves.size() && !should_stop(); i++) {
        Move move = moves[i];
        board->make_move(move);
        bool is_quiet = move.is_quiet();
        bool gives_check = board->is_in_check(board->get_turn());
        int move_score;

        bool next_follow_pv = follow_pv && move == pv_move;

        PVLine line(depth_left - 1);

        if (!found_pv) {
            // Full window search for first move.
            move_score = -alpha_beta(-beta, -alpha, depth_left - 1, ply + 1,
                                     &line, next_follow_pv);
        } else {
            // Late Move Reductions (LMR)
            if (depth_left >= 3 && i >= 3 && is_quiet && !in_check &&
                !gives_check) {
                int reduction = 2 + (i > 6) + (depth_left > 6);
                int reduced_depth = std::max(0, depth_left - reduction);

                move_score = -alpha_beta(-alpha - 1, -alpha, reduced_depth,
                                         ply + 1, &line, next_follow_pv);

                if (move_score > alpha) {
                    // Re-search at full depth with narrow window when the
                    // reduced search fails high.
                    line.moves.clear();
                    move_score = -alpha_beta(-alpha - 1, -alpha, depth_left - 1,
                                             ply + 1, &line, next_follow_pv);
                }
            } else {
                // Regular null window search when the move is not reduced.
                move_score = -alpha_beta(-alpha - 1, -alpha, depth_left - 1,
                                         ply + 1, &line, next_follow_pv);
            }

            // Re-search with full window if the move is actually better; i.e.,
            // PV node.
            if (move_score > alpha && move_score < beta) {
                line.moves.clear();
                move_score = -alpha_beta(-beta, -alpha, depth_left - 1, ply + 1,
                                         &line, next_follow_pv);
            }
        }

        board->undo_move();

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
            if (is_quiet) {
                if (ply >= killers.size())
                    killers.resize(ply + 1, std::array<Move, 2>{});
                if (killers[ply][0] != move) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = move;
                }
            }
            break;
        }
    }

    // Store best move (or first legal move if no improvement).
    if (pline->moves.empty() && !moves.empty()) {
        pline->moves.emplace_back(moves.front());
    }

    if (!pline->moves.empty()) {
        if (!should_stop()) {
            int tt_score = alpha;
            if (tt_score > CHECKMATE_COMPARE && tt_score <= -CHECKMATE_SCORE) {
                tt_score += ply;
            } else if (tt_score < -CHECKMATE_COMPARE &&
                       tt_score >= CHECKMATE_SCORE) {
                tt_score -= ply;
            }
            tt.store(hash, pline->moves.front(), tt_score, depth_left, bound);
        }
        return alpha;
    }

    // No legal moves now, so: checkmate or stalemate.

    if (in_check) {
        alpha = CHECKMATE_SCORE + ply;
    } else {
        alpha = 0;
    }

    return alpha;
}

int Search::quiescence(int alpha, int beta) {
    nodes_searched++;
    if (should_stop())
        return alpha;

    const bool in_check = board->is_in_check(board->get_turn());

    int stand_pat = board->evaluate();
    if (!in_check) {
        if (stand_pat >= beta)
            return stand_pat;
        alpha = std::max(alpha, stand_pat);
    }

    std::vector<Move> moves = board->get_moves<true>();
    order_moves(moves);

    for (auto move : moves) {
        // Delta Pruning
        if (!in_check && !move.is_promotion()) {
            int gain = move.is_en_passant()
                           ? Eval::value(Piece::PAWN)
                           : Eval::value(*board->get_piece(move.to()));
            if (stand_pat + gain + DELTA_PRUNING < alpha)
                continue;
        }

        board->make_move(move);
        int score = -quiescence(-beta, -alpha);
        board->undo_move();

        if (score >= beta)
            return score;

        alpha = std::max(alpha, score);
    }

    return alpha;
}
