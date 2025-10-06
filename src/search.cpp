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
const int ALPHA_START = -50000;
const int BETA_START = 50000;

SearchInfo Search::iterative_deepening(int max_depth, long long time_limit) {
    SearchInfo search_info(max_depth);
    time_limit_ms = time_limit;
    start_time = std::chrono::high_resolution_clock::now();
    nodes_searched = 0;
    time_up = false;
    
    tt.new_search();
    
    int alpha = ALPHA_START;
    int beta = BETA_START;
    const int ASPIRATION_WINDOW = 50;
    
    for (int depth = 1; depth <= max_depth; depth++) {
        if (should_stop()) break;
        
        PVLine pv_line(depth);
        int score;
        
        // use aspiration windows
        if (depth > 2) {
            alpha = search_info.score - ASPIRATION_WINDOW;
            beta = search_info.score + ASPIRATION_WINDOW;
            
            // try narrow window first
            score = alpha_beta(alpha, beta, depth, &pv_line);
            
            // re-search with wider window if needed
            if (score <= alpha || score >= beta) {
                alpha = ALPHA_START;
                beta = BETA_START;
                score = alpha_beta(alpha, beta, depth, &pv_line);
            }
        } else {
            score = alpha_beta(alpha, beta, depth, &pv_line);
        }
        
        if (should_stop()) break;
        
        search_info.depth = depth;
        search_info.score = score;
        search_info.nodes = nodes_searched;
        search_info.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        search_info.pv = pv_line;

        debug(depth, score, nodes_searched, search_info.time_ms, pv_line.moves);
    }
    
    search_info.stopped = time_up;
    return search_info;
}

inline bool Search::should_stop() {
    if (time_up) return true;
    
    // check time every ~1000 nodes
    if (nodes_searched & 0x7ff) {
        auto current_time = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( current_time - start_time).count();
        
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
    TTEntry* tt_entry = nullptr;
    bool tt_hit = tt.probe(hash, tt_entry);
    
    if (tt_hit) {
        tt_move = tt_entry->best_move;
        
        // Use TT score if depth is sufficient and not a PV node
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
    
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        
        if (should_stop()) break;
        
        board->make_move(move);
        int score;
        
        if (!found_pv) {
            // full window search for first move
            score = -alpha_beta(-beta, -alpha, depth_left - 1, &line);
        } else {
            // null window search for remaining moves
            score = -alpha_beta(-alpha - 100, -alpha, depth_left - 1, &line);
            
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
    
    // Store in transposition table
    Bound bound;
    if (alpha >= beta) {
        bound = BOUND_LOWER; // Beta cutoff (failed high)
    } else if (alpha > original_alpha) {
        bound = BOUND_EXACT; // Exact score (PV node)
    } else {
        bound = BOUND_UPPER; // Failed low (all moves failed to raise alpha)
    }
    
    // Store best move (or first legal move if no improvement)
    if (best_move.m_move == 0 && !moves.empty()) {
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
