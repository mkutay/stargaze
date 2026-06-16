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

#include "square.hpp"
#include <utility>

struct ZobristKeys {
    std::array<std::array<std::array<uint64_t, 64>, 6>, 2> hash;
    std::array<uint64_t, 4> castling;
    std::array<uint64_t, 8> en_passant_file;
    uint64_t black_move;

    constexpr ZobristKeys() {
        crand::xoshiro256_starstar rng{0x9e3779b97f4a7c15ULL};

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

class Zobrist {
  private:
    static constexpr ZobristKeys keys{};

  public:
    Zobrist() = delete;

    static constexpr uint64_t piece(Colour colour, Piece piece, Square sq) {
        return keys
            .hash[std::to_underlying(colour)][std::to_underlying(piece)][sq];
    }

    static constexpr uint64_t castling(size_t index) {
        return keys.castling[index];
    }

    static constexpr uint64_t en_passant(Square sq) {
        return keys.en_passant_file[sq.file()];
    }

    static constexpr uint64_t black_move() { return keys.black_move; }
};
