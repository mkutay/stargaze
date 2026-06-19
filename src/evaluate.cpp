#include "board.hpp"
#include "colour.hpp"
#include "eval.hpp"
#include "piece.hpp"
#include <array>

void Board::initialise_eval(std::array<int, 2> &_mg_score,
                            std::array<int, 2> &_eg_score,
                            int &_game_phase) const {
    _mg_score = {0, 0};
    _eg_score = {0, 0};
    _game_phase = 0;

    for (Colour c : COLOURS) {
        for (Piece p : PIECES) {
            auto bb = piece_bbs[p] & colour_bbs[c];
            while (bb) {
                auto sq = bb.get_square_pop();
                _mg_score[c] += Eval::mg_value(c, p, sq);
                _eg_score[c] += Eval::eg_value(c, p, sq);
                _game_phase += Eval::gamephase_inc(p);
            }
        }
    }
}

int Board::evaluate() const {
    int mg_diff = mg_score[turn] - mg_score[!turn];
    int eg_diff = eg_score[turn] - eg_score[!turn];
    int mg_phase = game_phase;
    if (mg_phase > Eval::gamephase_sum())
        mg_phase = Eval::gamephase_sum(); // in case of early promotion
    int eg_phase = Eval::gamephase_sum() - mg_phase;

    return (mg_diff * mg_phase + eg_diff * eg_phase) / Eval::gamephase_sum();
}
