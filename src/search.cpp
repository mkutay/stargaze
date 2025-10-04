#include "search.hpp"
#include "evaluate.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>

const int MAX_QUIESCENCE_DEPTH = 2;
const int ALPHA_START = -50000;
const int BETA_START = 50000;

SearchInfo Search::iterative_deepening(int max_depth, long long time_limit) {
    SearchInfo search_info(max_depth);
    time_limit_ms = time_limit;
    start_time = std::chrono::high_resolution_clock::now();
    nodes_searched = 0;
    time_up = false;
    
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
        search_info.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start_time).count();
        search_info.pv = pv_line;
        
        std::cout << "info depth " << depth 
                  << " score cp " << score
                  << " nodes " << nodes_searched
                  << " time " << search_info.time_ms
                  << " pv ";
        
        for (int i = 0; i < depth && i < static_cast<int>(pv_line.moves.size()); i++) {
            if (pv_line.moves[i].from() != pv_line.moves[i].to()) { // Valid move check
                std::cout << pv_line.moves[i].to_string() << " ";
            } else {
                break;
            }
        }
        std::cout << std::endl;
    }
    
    search_info.stopped = time_up;
    return search_info;
}

inline bool Search::should_stop() {
    if (time_up) return true;
    
    // check time every ~1000 nodes
    if (nodes_searched & 0x3ff) {
        auto current_time = std::chrono::high_resolution_clock::now();
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( current_time - start_time).count();
        
        if (elapsed >= time_limit_ms) {
            time_up = true;
            return true;
        }
    }
    
    return false;
}

void Search::order_moves(std::vector<Move>& moves, const PVLine* pv_line) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        // PV move gets highest priority
        if (pv_line && !pv_line->moves.empty()) {
            Move pv_move = pv_line->moves[0];
            if (a == pv_move) return true;
            if (b == pv_move) return false;
        }
        
        // captures get priority over non-captures
        if (a.is_capture() && !b.is_capture()) return true;
        if (!a.is_capture() && b.is_capture()) return false;
        
        return false; // keep original order for moves of same type
    });
}

int Search::alpha_beta(int alpha, int beta, int depth_left, PVLine *pline) {
    nodes_searched++;
    
    if (should_stop()) return alpha;
    
    // u_int64_t hash = board->get_hash();
    // if (tt.is_hash_present(hash)) {
    //   Entry entry = tt.get_entry(hash);
    //   if (entry.depth >= depth_left) {
    //     if (entry.score >= beta) return beta;
    //     if (entry.score > alpha) alpha = entry.score;
    //   }
    // }
    
    if (depth_left == 0) return quiescence(alpha, beta, MAX_QUIESCENCE_DEPTH);
    
    PVLine line(depth_left - 1);
    std::vector<Move> moves = board->get_moves();
    
    order_moves(moves, pline);
    
    bool found_pv = false;
    
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
            found_pv = true;
            
            pline->moves[0] = move;
            std::copy(line.moves.begin(), line.moves.end(), pline->moves.begin() + 1);
        }
        
        if (score >= beta) {
            break;
        }
    }
    
    // Entry entry = { hash, depth_left, alpha };
    // tt.set_entry(hash, entry);
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
