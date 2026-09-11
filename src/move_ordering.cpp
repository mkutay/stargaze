#include "search.hpp"

#include <algorithm>

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
        const auto &k_moves = killers[*ply];
        for (size_t i = 0; i < k_moves.size(); i++) {
            if (move == k_moves[i])
                return KILLER_SCORES[i];
        }
    }

    return history_table[move.from().raw()][move.to().raw()];
}

void Search::score_moves(const std::vector<Move> &moves,
                         std::vector<int> &scores, std::optional<Move> pv_move,
                         std::optional<Move> tt_move,
                         std::optional<uint16_t> ply) const {
    scores.resize(moves.size());
    for (size_t i = 0; i < moves.size(); i++)
        scores[i] = score_move(moves[i], pv_move, tt_move, ply);
}

void Search::pick_next_move(std::vector<Move> &moves, std::vector<int> &scores,
                            size_t index) const {
    size_t best_idx = index;
    for (size_t i = index + 1; i < moves.size(); i++) {
        if (scores[i] > scores[best_idx])
            best_idx = i;
    }

    std::swap(moves[index], moves[best_idx]);
    std::swap(scores[index], scores[best_idx]);
}

void Search::record_cutoff(Move move, uint16_t ply, uint16_t depth) {
    if (ply >= killers.size())
        killers.resize(ply + 1, std::array<Move, 2>{});
    if (killers[ply][0] != move) {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = move;
    }

    int &history = history_table[move.from().raw()][move.to().raw()];
    history += depth * depth;
    if (history >= KILLER_SCORES[0])
        history = KILLER_SCORES[0] - 1;
}
