#pragma once

#include "stargaze/colour.hpp"
#include "stargaze/move.hpp"
#include "stargaze/piece.hpp"
#include "stargaze/square.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

class SearchHistory {
  public:
    using Context = uint16_t;
    using OptionalContext = std::optional<Context>;
    constexpr static int MAX_SCORE = 16384;

  private:
    constexpr static std::size_t CONTEXT_COUNT = 2 * 6 * 64;

    std::array<int16_t, CONTEXT_COUNT> butterfly{};
    std::array<std::array<int16_t, CONTEXT_COUNT>, CONTEXT_COUNT>
        continuation{};
    std::array<std::optional<Move>, CONTEXT_COUNT> countermoves{};

    static void gravity_update(int16_t &entry, int bonus);

  public:
    SearchHistory() {}

    static Context context(Colour side, Piece piece, Square destination);
    int score(Context current, std::optional<Context> previous) const;
    void update(Context current, int bonus, std::optional<Context> previous);
    void record_cutoff(Context current, std::optional<Context> previous,
                       Move move, int bonus);
    std::optional<Move> countermove(std::optional<Context> previous) const;
};
