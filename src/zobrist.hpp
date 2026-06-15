/**
 * Compile-time Zobrist keys.
 *
 * Keys are generated at compile time with the xoshiro256** engine from
 * `jan-moeller/constexpr-random`. A fixed seed keeps the keys deterministic
 * across builds, which is required for stable hashing.
 */

#pragma once
#include "crand/engines/xoshiro256_starstar_engine.hpp"

#include <array>
#include <cstdint>

namespace Zobrist {

class Keys {
  private:
    crand::xoshiro256_starstar rng;

  public:
    std::array<std::array<std::array<uint64_t, 64>, 6>, 2> hash;
    std::array<uint64_t, 4> castling;
    std::array<uint64_t, 8> en_passant_file;
    uint64_t black_move;

    constexpr Keys() : rng{0x9e3779b97f4a7c15ULL} {
        for (auto &colour : hash)
            for (auto &piece : colour)
                for (auto &square : piece)
                    square = rng();

        for (auto &c : castling)
            c = rng();

        for (auto &e : en_passant_file)
            e = rng();

        black_move = rng();
    }
};

const inline constexpr Keys keys{};

const inline constexpr auto &hash = keys.hash;
const inline constexpr auto &castling = keys.castling;
const inline constexpr auto &en_passant_file = keys.en_passant_file;
const inline constexpr uint64_t black_move = keys.black_move;

} // namespace Zobrist
