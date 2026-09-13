#include "stargaze/search_history.hpp"

#include <algorithm>
#include <cstdlib>

SearchHistory::Context SearchHistory::context(Colour side, Piece piece,
                                              Square destination) {
    return (side.raw() * 6 + piece.raw()) * 64 + destination.raw();
}

int SearchHistory::score(Context current, OptionalContext previous) const {
    return butterfly[current] +
           (previous ? continuation[*previous][current] : 0);
}

void SearchHistory::gravity_update(int16_t &entry, int bonus) {
    bonus = std::clamp(bonus, -MAX_SCORE, MAX_SCORE);
    const int value = entry;
    entry = static_cast<int16_t>(value + bonus -
                                 value * std::abs(bonus) / MAX_SCORE);
}

void SearchHistory::update(Context current, int bonus,
                           OptionalContext previous) {
    gravity_update(butterfly[current], bonus);
    if (previous)
        gravity_update(continuation[*previous][current], bonus);
}

void SearchHistory::record_cutoff(Context current, OptionalContext previous,
                                  Move move, int bonus) {
    update(current, bonus, previous);
    if (previous)
        countermoves[*previous] = move;
}

std::optional<Move> SearchHistory::countermove(OptionalContext previous) const {
    return previous ? countermoves[*previous] : std::nullopt;
}
