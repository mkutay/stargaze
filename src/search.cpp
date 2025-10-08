#include "search.hpp"
#include "evaluate.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>

#ifdef DEBUG
  #include "debug.hpp"
#else
  #define debug(...) void(38)
#endif

const int MAX_QUIESCENCE_DEPTH = 2;
const int ALPHA_START = -100000;
const int BETA_START = 100000;
const int ASPIRATION_WINDOW = 23; // ~0.25 pawn

// NOTE use calculated alpha/beta values from previous "move" in iterative deepening to set alpha/beta with a narrow window
// basically, get pvline of previous move here

SearchInfo Search::iterative_deepening(int max_depth, long long time_limit, SearchInfo *last_info) {
    SearchInfo search_info(max_depth);
    time_limit_ms = time_limit;
    start_time = std::chrono::high_resolution_clock::now();
    nodes_searched = 0;
    time_up = false;
    
    tt.new_search();
    
    // since last info is from the opponent's perspective, we negate the score
    int alpha = last_info ? -last_info->score - ASPIRATION_WINDOW : ALPHA_START;
    int beta = last_info ? -last_info->score + ASPIRATION_WINDOW : BETA_START;
    const int aspiration = ASPIRATION_WINDOW;
    
    for (int depth = 1; depth <= max_depth && !should_stop(); depth++) {
        PVLine pv_line(depth);
        int score;

        if (depth == 1) {
            score = alpha_beta(alpha, beta, depth, &pv_line);
        } else {
            alpha = search_info.score - aspiration;
            beta = search_info.score + aspiration;
            
            // try narrow window first
            score = alpha_beta(alpha, beta, depth, &pv_line);
            
            if (score >= beta) { // failed high
                alpha = search_info.score - aspiration;
                beta = search_info.score + aspiration * 4;
                score = alpha_beta(alpha, beta, depth, &pv_line);
            } else if (score <= alpha) { // failed low
                alpha = search_info.score - aspiration * 4;
                beta = search_info.score + aspiration;
                score = alpha_beta(alpha, beta, depth, &pv_line);
            }
        }

        if (should_stop()) break;
        
        search_info.depth = depth;
        search_info.score = score;
        search_info.nodes = nodes_searched;
        search_info.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        search_info.pv = pv_line;

        debug(depth, alpha, score, beta, nodes_searched, pv_line.moves);
    }
    
    search_info.stopped = time_up;
    return search_info;
}

inline bool Search::should_stop() {
    if (time_up) return true;
    
    // check time every ~2000 nodes
    if (nodes_searched & 0x7ff) {
        auto current_time = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        
        if (elapsed >= time_limit_ms) {
            time_up = true;
            return true;
        }
    }
    
    return false;
}

void Search::order_moves(std::vector<Move>& moves, const PVLine* pv_line, Move tt_move) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        if (tt_move.m_move != 0) {
            if (a == tt_move) return true;
            if (b == tt_move) return false;
        }
        
        if (pv_line && !pv_line->moves.empty()) {
            Move pv_move = pv_line->moves[0];
            if (a == pv_move) return true;
            if (b == pv_move) return false;
        }

        // promotions
        if (a.is_promotion() && !b.is_promotion()) return true;
        if (!a.is_promotion() && b.is_promotion()) return false;

        // castling
        if (a.is_castle() && !b.is_castle()) return true;
        if (!a.is_castle() && b.is_castle()) return false;
        
        // captures
        if (a.is_capture() && !b.is_capture()) return true;
        if (!a.is_capture() && b.is_capture()) return false;
        
        return false; // keep original order for moves of same type
    });
}

int Search::alpha_beta(int alpha, int beta, int depth_left, PVLine *pline) {
    nodes_searched++;
    
    if (should_stop()) return alpha;
    
    int original_alpha = alpha;
    bool is_pv_node = (beta - alpha) > 1;
    Move tt_move;
    
    // Probe transposition table
    u_int64_t hash = board->get_hash();
    auto result = tt.probe(hash);
    
    if (result != std::nullopt) {
        auto tt_entry = result.value();
        tt_move = tt_entry->best_move;
        
        // use TT score if depth is sufficient and not a PV node
        if (tt_entry->depth >= depth_left && !is_pv_node) {
            int tt_score = tt_entry->score;
            
            if (tt_entry->bound == BOUND_EXACT) {
                return tt_score;
            } else if (tt_entry->bound == BOUND_LOWER) {
                if (tt_score >= beta) return tt_score;
            } else if (tt_entry->bound == BOUND_UPPER) {
                if (tt_score <= alpha) return tt_score;
            }
        }
    }
    
    if (depth_left == 0) return quiescence(alpha, beta, MAX_QUIESCENCE_DEPTH);
    
    PVLine line(depth_left - 1);
    std::vector<Move> moves = board->get_moves();
    
    order_moves(moves, pline, tt_move);
    
    bool found_pv = false;
    Move best_move;
    
    for (size_t i = 0; i < moves.size() && !should_stop(); i++) {
        Move move = moves[i];
#ifdef DEBUG
        assert(move.m_move != 0);
#endif
        
        board->make_move(move);
        int score;
        
        if (!found_pv) {
            // full window search for first move
            score = -alpha_beta(-beta, -alpha, depth_left - 1, &line);
        } else {
            // null window search for remaining moves
            score = -alpha_beta(-alpha - ASPIRATION_WINDOW, -alpha, depth_left - 1, &line);
            
            // re-search with full window if null window failed high
            if (score > alpha && score < beta) {
                score = -alpha_beta(-beta, -alpha, depth_left - 1, &line);
            }
        }
        
        board->undo_move();

        if (score > alpha) {
            alpha = score;
            best_move = move;
            found_pv = true;
            
            pline->moves[0] = move;
            std::copy(line.moves.begin(), line.moves.end(), pline->moves.begin() + 1);
        }
        
        if (score >= beta) {
            break;
        }
    }
    
    Bound bound;
    if (alpha >= beta) {
        bound = BOUND_LOWER; // beta cutoff (failed high)
        // denotes that the score is at least as high as beta
    } else if (alpha > original_alpha) {
        bound = BOUND_EXACT; // exact score (PV node)
        // this is an exact score, and shouldn't change if you change alpha or beta
    } else {
        bound = BOUND_UPPER; // failed low (all moves failed to raise alpha)
        // the score cannot be greater than alpha
    }
    
    // store best move (or first legal move if no improvement)
    if (best_move.m_move == 0) {
#ifdef DEBUG
        assert(!moves.empty());
        assert(moves[0].m_move != 0);
#endif
        best_move = moves[0];
    }
    
    tt.store(hash, best_move, alpha, depth_left, bound);
    
    return alpha;
}

int Search::quiescence(int alpha, int beta, int depth) {
    nodes_searched++;
    
    if (should_stop()) return alpha;
    
    int stand_pat = evaluate(*board);
    if (depth == 0) return stand_pat;
    if (stand_pat >= beta) return stand_pat;
    alpha = std::max(alpha, stand_pat);
    
    std::vector<Move> moves = board->get_moves();
    
    for (Move move : moves) {
        if (!move.is_capture()) continue;
        
        if (should_stop()) break;
        
        board->make_move(move);
        int score = -quiescence(-beta, -alpha, depth - 1);
        board->undo_move();
        
        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }
    
    return alpha;
}
