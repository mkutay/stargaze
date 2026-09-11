#pragma once

#include <cstdint>
#include <cstdlib>
#include <format>

class Score {
  private:
    int32_t v;

  public:
    constexpr static int32_t INFINITY_SCORE = 300000;
    constexpr static int32_t MATE_SCORE = 200000;
    constexpr static int32_t MATE_THRESHOLD = 150000;

    constexpr Score() : v(0) {}
    constexpr Score(int32_t val) : v(val) {}

    static constexpr Score draw() { return Score(0); }
    static constexpr Score mate(int plies) {
        return plies >= 0 ? Score(MATE_SCORE - plies)
                          : Score(-MATE_SCORE - plies);
    }
    static constexpr Score centipawns(int cp) { return Score(cp); }

    constexpr bool is_mate() const {
        const int32_t abs_v = v < 0 ? -v : v;
        return abs_v >= MATE_THRESHOLD && abs_v <= MATE_SCORE;
    }

    constexpr bool is_draw() const { return v == 0; }

    /**
     * Returns the number of plies until mate, positive for white mates,
     * negative for black mates.
     */
    constexpr int mate_plies() const {
        if (!is_mate())
            return 0;
        return v > 0 ? (MATE_SCORE - v) : (v + MATE_SCORE);
    }

    /**
     * Returns the number of moves until mate, similar to mate_plies().
     */
    constexpr int mate_moves() const {
        int plies = mate_plies();
        return (v > 0) ? ((plies + 1) / 2) : -((plies + 1) / 2);
    }

    constexpr int raw() const { return v; }

    /**
     * Converts the score to a representation that can be used in a
     * transposition table, adjusting for the number of plies from the root.
     */
    constexpr Score to_tt(int ply) const {
        if (v > MATE_THRESHOLD && v <= MATE_SCORE) {
            return Score(v + ply);
        } else if (v < -MATE_THRESHOLD && v >= -MATE_SCORE) {
            return Score(v - ply);
        }
        return *this;
    }

    /**
     * Converts the score from a representation that can be used in a
     * transposition table to a representation that can be used for evaluation,
     * adjusting for the number of plies from the root.
     */
    constexpr Score from_tt(int ply) const {
        if (v > MATE_THRESHOLD && v <= MATE_SCORE) {
            return Score(v - ply);
        } else if (v < -MATE_THRESHOLD && v >= -MATE_SCORE) {
            return Score(v + ply);
        }
        return *this;
    }

    constexpr Score operator-() const { return Score(-v); }

    constexpr Score operator+(const Score &o) const { return Score(v + o.v); }
    constexpr Score operator-(const Score &o) const { return Score(v - o.v); }
    constexpr Score &operator+=(const Score &o) {
        v += o.v;
        return *this;
    }
    constexpr Score &operator-=(const Score &o) {
        v -= o.v;
        return *this;
    }

    constexpr Score operator+(int32_t o) const { return Score(v + o); }
    constexpr Score operator-(int32_t o) const { return Score(v - o); }
    constexpr Score &operator+=(int32_t o) {
        v += o;
        return *this;
    }
    constexpr Score &operator-=(int32_t o) {
        v -= o;
        return *this;
    }

    friend constexpr Score operator+(int32_t lhs, const Score &rhs) {
        return Score(lhs + rhs.v);
    }
    friend constexpr Score operator-(int32_t lhs, const Score &rhs) {
        return Score(lhs - rhs.v);
    }

    constexpr bool operator==(const Score &o) const { return v == o.v; }
    constexpr bool operator!=(const Score &o) const { return v != o.v; }
    constexpr bool operator<(const Score &o) const { return v < o.v; }
    constexpr bool operator>(const Score &o) const { return v > o.v; }
    constexpr bool operator<=(const Score &o) const { return v <= o.v; }
    constexpr bool operator>=(const Score &o) const { return v >= o.v; }

    constexpr bool operator==(int32_t o) const { return v == o; }
    constexpr bool operator!=(int32_t o) const { return v != o; }
    constexpr bool operator<(int32_t o) const { return v < o; }
    constexpr bool operator>(int32_t o) const { return v > o; }
    constexpr bool operator<=(int32_t o) const { return v <= o; }
    constexpr bool operator>=(int32_t o) const { return v >= o; }

    friend constexpr bool operator==(int32_t lhs, const Score &rhs) {
        return lhs == rhs.v;
    }
    friend constexpr bool operator!=(int32_t lhs, const Score &rhs) {
        return lhs != rhs.v;
    }
    friend constexpr bool operator<(int32_t lhs, const Score &rhs) {
        return lhs < rhs.v;
    }
    friend constexpr bool operator>(int32_t lhs, const Score &rhs) {
        return lhs > rhs.v;
    }
    friend constexpr bool operator<=(int32_t lhs, const Score &rhs) {
        return lhs <= rhs.v;
    }
    friend constexpr bool operator>=(int32_t lhs, const Score &rhs) {
        return lhs >= rhs.v;
    }
};

// std::formatter specialisation for Score
template <> struct std::formatter<Score> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const Score &score, std::format_context &ctx) const {
        if (score.is_mate()) {
            return std::format_to(ctx.out(), "mate {}", score.mate_moves());
        } else {
            return std::format_to(ctx.out(), "{}", score.raw());
        }
    }
};

static_assert(Score::centipawns(150).raw() == 150);
static_assert(!Score::centipawns(150).is_mate());
static_assert(!Score::centipawns(150).is_draw());
static_assert(Score::draw().is_draw());
static_assert(Score::draw().raw() == 0);
static_assert(!Score::draw().is_mate());
static_assert(Score::mate(3).is_mate());
static_assert(!Score::mate(3).is_draw());
static_assert(Score::mate(3).mate_plies() == 3);
static_assert(Score::mate(3).mate_moves() == 2); // (3+1)/2 = 2 moves
static_assert(Score::mate(-3).is_mate());
static_assert(!Score::mate(-3).is_draw());
static_assert(Score::mate(-3).mate_plies() == 3);
static_assert(Score::mate(-3).mate_moves() == -2);
static_assert(Score(100) + Score(200) == 300);
static_assert(Score(200) - Score(100) == 100);
static_assert(-Score(100) == -100);
static_assert(Score(100) < Score(200));
static_assert(Score(200) > Score(100));
static_assert(Score(100) <= Score(200));
static_assert(Score(200) >= Score(100));
static_assert(Score(100) != Score(200));
static_assert(Score::mate(3).to_tt(2).raw() ==
              199999); // winning mate in 3 plies found at search ply 2
static_assert(Score::mate(3).to_tt(2).from_tt(2).mate_plies() == 3);
static_assert(Score::mate(-3).to_tt(2).raw() ==
              -199999); // losing mate in 3 plies found at search ply 2
static_assert(Score::mate(-3).to_tt(2).from_tt(2).mate_plies() == 3);
