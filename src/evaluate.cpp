#include "board.hpp"
#include "enums.hpp"
#include "eval.hpp"
#include <array>

void Board::initialise_eval(std::array<int, 2> &_mg_score,
                            std::array<int, 2> &_eg_score,
                            int &_game_phase) const {
    _mg_score = {0, 0};
    _eg_score = {0, 0};
    _game_phase = 0;

    for (Colour c : COLOURS) {
        for (Piece p : PIECES) {
            auto ci = std::to_underlying(c);
            auto pi = std::to_underlying(p);
            auto bb = piece_bbs[pi] & colour_bbs[ci];
            while (bb) {
                auto sq = bb.get_square_pop();
                _mg_score[ci] += Eval::mg_value(c, p, sq);
                _eg_score[ci] += Eval::eg_value(c, p, sq);
                _game_phase += Eval::gamephase_inc(p);
            }
        }
    }
}

int Board::evaluate() const {
    auto ti = std::to_underlying(turn);
    auto nti = std::to_underlying(!turn);
    int mg_diff = mg_score[ti] - mg_score[nti];
    int eg_diff = eg_score[ti] - eg_score[nti];
    int mg_phase = game_phase;
    if (mg_phase > Eval::gamephase_sum())
        mg_phase = Eval::gamephase_sum(); // in case of early promotion
    int eg_phase = Eval::gamephase_sum() - mg_phase;

    return (mg_diff * mg_phase + eg_diff * eg_phase) / Eval::gamephase_sum();
}
