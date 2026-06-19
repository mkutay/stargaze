#pragma once
#include <array>
#include <cstdint>

class Colour {
  private:
    uint8_t colour;

  public:
    constexpr static const uint8_t WHITE = 0;
    constexpr static const uint8_t BLACK = 1;

    constexpr Colour() : colour(WHITE) {}
    constexpr Colour(uint8_t _colour) : colour(_colour) {}

    constexpr operator uint8_t() const { return colour; }

    /**
     * Returns the weight of the colour, which is 1 for white and -1 for black.
     */
    constexpr int weight() const { return -colour * 2 + 1; }

    /**
     * Returns the opposite colour.
     */
    constexpr Colour operator!() const { return Colour(1 - colour); }
};

namespace CC {
constexpr auto WHITE = Colour(Colour::WHITE);
constexpr auto BLACK = Colour(Colour::BLACK);

static_assert(WHITE.weight() == 1);
static_assert(BLACK.weight() == -1);
static_assert(!WHITE == BLACK);
static_assert(WHITE == !BLACK);
} // namespace CC

constexpr const static std::array<Colour, 2> COLOURS = {CC::WHITE, CC::BLACK};

static_assert(sizeof(Colour) == sizeof(uint8_t));
