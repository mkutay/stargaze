#include "eval.hpp"

#include "search.hpp"
#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>

#ifdef DEBUG
#include "debug.hpp"
#else
#define debug(...) void(38)
#endif

SearchInfo Search::iterative_deepening(uint16_t max_depth,
                                       uint32_t time_limit) {
    SearchInfo search_info(max_depth);
    time_limit_ms = time_limit;
    start_time = std::chrono::high_resolution_clock::now();
    nodes_searched = 0;
    time_up = false;
    killers = {};

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
            score = alpha_beta(alpha, beta, depth, 0, &pv_line);
        } else {
            auto delta = ASPIRATION_WINDOW;
            alpha = prev_score - delta;
            beta = prev_score + delta;

            while (!should_stop()) {
                // Ensure that alpha and beta are within the initial bounds to
                // avoid searching outside the valid score range.
                alpha = std::max(alpha, ALPHA_START);
                beta = std::min(beta, BETA_START);

                score = alpha_beta(alpha, beta, depth, 0, &pv_line);

                if (should_stop())
                    break;

                if (score <= alpha) { // failed low
                    alpha -= delta;
                    delta *= 2;
                    if (delta > DELTA_MAX)
                        alpha = ALPHA_START;
                } else if (score >= beta) { // failed high
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

        debug(depth, alpha, score, beta, nodes_searched, pv_line.moves,
              board->get_castling_rights());
    }

    search_info.stopped = time_up;
    return search_info;
}

inline bool Search::should_stop() {
    if (time_up)
        return true;

    if (!(nodes_searched & 0x3FFF)) { // check every 16384 nodes
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           current_time - start_time)
                           .count();

        if (elapsed >= time_limit_ms)
            return time_up = true;
    }

    return false;
}

int Search::score_move(Move move, std::optional<Move> tt_move,
                       std::optional<uint16_t> ply) {
    if (move == tt_move) {
        return 100000;
    }

    if (move.is_promotion()) {
        return 90000;
    }

    if (move.is_capture()) {
        auto victim_val = move.is_en_passant()
                              ? Eval::value(Piece::PAWN)
                              : Eval::value(*board->get_piece(move.to()));
        auto aggressor_val = Eval::value(*board->get_piece(move.from()));
        return 70000 + 10 * victim_val - aggressor_val;
    }

    if (move.is_castle()) {
        return 60000;
    }

    if (ply.has_value() && ply.value() < killers.size()) {
        auto p = ply.value();
        if (move == killers[p][0])
            return 50000;
        if (move == killers[p][1])
            return 40000;
    }

    return 0;
}

void Search::order_moves(std::vector<Move> &moves, std::optional<Move> tt_move,
                         std::optional<uint16_t> ply) {
    std::vector<std::pair<int, Move>> scored_moves;
    scored_moves.reserve(moves.size());
    for (const auto &move : moves)
        scored_moves.emplace_back(score_move(move, tt_move, ply), move);

    std::stable_sort(
        scored_moves.begin(), scored_moves.end(),
        [](const auto &a, const auto &b) { return a.first > b.first; });

    for (size_t i = 0; i < moves.size(); i++)
        moves[i] = scored_moves[i].second;
}

int Search::alpha_beta(int alpha, int beta, uint16_t depth_left, uint16_t ply,
                       PVLine *pline) {
    nodes_searched++;
    if (should_stop())
        return alpha;

    if (board->is_draw())
        return 0;

    bool in_check = board->is_in_check(board->get_turn());
    if (in_check)
        depth_left++;

    int original_alpha = alpha;
    bool is_pv_node = (beta - alpha) > 1;
    Move tt_move;

    auto hash = board->get_hash();
    TTEntry *tt_entry = tt.probe(hash);

    if (tt_entry != nullptr) {
        if (tt_entry->best_move != 0) {
            tt_move = Move(tt_entry->best_move);
        }

        // use TT score if depth is sufficient and not a PV node
        if (tt_entry->depth >= depth_left && !is_pv_node) {
            int tt_score = tt_entry->score;

            auto copy_tt_line = [&]() {
                pline->moves.clear();
                if (tt_entry->best_move != 0) {
                    pline->moves.emplace_back(tt_entry->best_move);
                }
            };

            if (tt_entry->bound == Bound::EXACT) {
                copy_tt_line();
                return tt_score;
            } else if (tt_entry->bound == Bound::LOWER && tt_score >= beta) {
                copy_tt_line();
                return tt_score;
            } else if (tt_entry->bound == Bound::UPPER && tt_score <= alpha) {
                copy_tt_line();
                return tt_score;
            }
        }
    }

    if (depth_left == 0)
        return quiescence(alpha, beta);

    // Null Move Pruning
    if (depth_left >= 3 && !is_pv_node && !in_check &&
        board->has_non_pawn_material(board->get_turn())) {
        int R = 2 + depth_left / 6;
        int next_depth = (int) depth_left - 1 - R;
        next_depth = std::max(0, next_depth);

        board->null_move<false>();
        PVLine null_line(next_depth);
        auto score =
            -alpha_beta(-beta, -beta + 1, next_depth, ply + 1, &null_line);
        board->null_move<true>();

        if (score >= beta && score < BETA_START)
            return beta;
    }

    PVLine line(depth_left - 1);
    std::vector<Move> moves = board->get_moves<false>();

    order_moves(moves, tt_move, ply);

    bool found_pv = false;

    for (size_t i = 0; i < moves.size() && !should_stop(); i++) {
        Move move = moves[i];
        bool is_quiet = !move.is_capture() && !move.is_promotion();

        board->make_move(move);
        bool gives_check = board->is_in_check(board->get_turn());
        int score;

        if (!found_pv) {
            // full window search for first move
            score = -alpha_beta(-beta, -alpha, depth_left - 1, ply + 1, &line);
        } else {
            // Late Move Reductions (LMR)
            if (depth_left >= 3 && i >= 3 && is_quiet && !in_check &&
                !gives_check) {
                int R = 1 + (i > 6) + (depth_left > 6);
                int reduced_depth = (int) depth_left - 1 - R;
                reduced_depth = std::max(0, reduced_depth);

                score = -alpha_beta(-alpha - 1, -alpha, reduced_depth, ply + 1,
                                    &line);

                if (score > alpha) {
                    // Re-search at full depth with narrow window
                    score = -alpha_beta(-alpha - 1, -alpha, depth_left - 1,
                                        ply + 1, &line);
                }
            } else {
                // Regular null window search
                score = -alpha_beta(-alpha - 1, -alpha, depth_left - 1, ply + 1,
                                    &line);
            }

            // re-search with full window if the move is genuinely better
            if (score > alpha && score < beta) {
                score =
                    -alpha_beta(-beta, -alpha, depth_left - 1, ply + 1, &line);
            }
        }

        board->undo_move();

        if (score > alpha) {
            alpha = score;
            found_pv = true;

            pline->moves.clear();
            pline->moves.emplace_back(move);
            pline->moves.insert(pline->moves.end(), line.moves.begin(),
                                line.moves.end());
        }

        if (score >= beta) {
            // Update killers
            if (is_quiet) {
                if (ply >= killers.size()) {
                    killers.resize(ply + 1);
                }
                if (killers[ply][0] != move) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = move;
                }
            }
            break;
        }
    }

    Bound bound = Bound::NONE;
    if (alpha >= beta) {
        bound = Bound::LOWER; // beta cutoff (failed high)
        // denotes that the score is at least as high as beta
    } else if (alpha > original_alpha) {
        bound = Bound::EXACT; // exact score (PV node)
        // this is an exact score, and shouldn't change if you change alpha or
        // beta
    } else {
        bound = Bound::UPPER; // failed low (all moves failed to raise alpha)
        // the score cannot be greater than alpha
    }

    // store best move (or first legal move if no improvement)
    if (pline->moves.empty() && !moves.empty()) {
        pline->moves.emplace_back(moves[0]);
    }

    if (!pline->moves.empty()) {
        tt.store(hash, pline->moves[0], alpha, depth_left, bound);
        return alpha;
    }

    // no legal moves now, so: checkmate or stalemate

    if (board->is_in_check(board->get_turn())) {
        alpha = CHECKMATE_SCORE + ply;
    } else {
        alpha = 0;
    }

    return alpha;
}

int Search::quiescence(int alpha, int beta) {
    nodes_searched++;

    int stand_pat = board->evaluate();
    if (stand_pat >= beta)
        return stand_pat;
    alpha = std::max(alpha, stand_pat);

    std::vector<Move> moves = board->get_moves<true>();
    order_moves(moves, std::nullopt, std::nullopt);

    for (Move move : moves) {
        board->make_move(move);
        int score = -quiescence(-beta, -alpha);
        board->undo_move();

        if (score >= beta)
            return score;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}
