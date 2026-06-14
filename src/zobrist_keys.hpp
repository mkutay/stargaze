/**
 * Use the meta programming random generator defined in `cr-lupin/metarand` with
 * paper to generate Zobrist keys at compile time.
 *
 * https://www.researchgate.net/publication/259005783_Random_number_generator_for_C_template_metaprograms
 */

#pragma once
#include "metarnd.hpp"
#include <array>
#include <cstdint>

namespace Zobrist {

template <typename Engine, size_t N> struct GenerateSequence {
    typedef typename ::Next<Engine>::type E1;
    typedef typename ::Next<E1>::type E2;
    typedef typename ::Next<E2>::type E3;

    constexpr static uint64_t value = (static_cast<uint64_t>(E1::value) << 42) ^
                                      (static_cast<uint64_t>(E2::value) << 21) ^
                                      static_cast<uint64_t>(E3::value);

    typedef typename GenerateSequence<E3, N - 1>::next_engine next_engine;

    constexpr static auto get_array() {
        std::array<uint64_t, N> arr{};
        arr[0] = value;
        auto rest = GenerateSequence<E3, N - 1>::get_array();
        for (size_t i = 1; i < N; ++i) {
            arr[i] = rest[i - 1];
        }
        return arr;
    }
};

template <typename Engine> struct GenerateSequence<Engine, 0> {
    typedef Engine next_engine;
    constexpr static auto get_array() { return std::array<uint64_t, 0>{}; }
};

// ChunkGenerator to split large arrays and avoid exceeding constexpr recursion
// depth limits (default 512).
template <typename Engine, size_t ChunkSize, size_t NumChunks>
struct ChunkGenerator {
    typedef GenerateSequence<Engine, ChunkSize> Gen;
    typedef typename ChunkGenerator<typename Gen::next_engine, ChunkSize,
                                    NumChunks - 1>::next_engine next_engine;

    constexpr static auto get_array() {
        std::array<uint64_t, ChunkSize * NumChunks> arr{};
        auto chunk = Gen::get_array();
        for (size_t i = 0; i < ChunkSize; ++i) {
            arr[i] = chunk[i];
        }
        auto rest = ChunkGenerator<typename Gen::next_engine, ChunkSize,
                                   NumChunks - 1>::get_array();
        for (size_t j = 0; j < ChunkSize * (NumChunks - 1); ++j) {
            arr[ChunkSize + j] = rest[j];
        }
        return arr;
    }
};

template <typename Engine, size_t ChunkSize>
struct ChunkGenerator<Engine, ChunkSize, 0> {
    typedef Engine next_engine;
    constexpr static auto get_array() { return std::array<uint64_t, 0>{}; }
};

// Base engine with a deterministic seed, using uint64_t to prevent LCG
// overflow.
typedef ::linear_congruential_engine<uint64_t, 123456789ULL> BaseEngine;

// 1. Zobrist::hash: std::array<std::array<std::array<uint64_t, 64>, 6>, 2>
// Total elements: 768 = 64 * 12. Generates in 12 chunks of size 64 to avoid
// recursion depth limits.
typedef ChunkGenerator<BaseEngine, 64, 12> HashGen;
constexpr auto flat_hash = HashGen::get_array();

constexpr auto create_hash_3d(const std::array<uint64_t, 768> &flat) {
    std::array<std::array<std::array<uint64_t, 64>, 6>, 2> h{};
    size_t idx = 0;
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++) {
            for (int sq = 0; sq < 64; sq++) {
                h[c][p][sq] = flat[idx++];
            }
        }
    }
    return h;
}

typedef typename HashGen::next_engine EngineAfterHash;

// 2. Zobrist::castling: std::array<uint64_t, 4>
typedef GenerateSequence<EngineAfterHash, 4> CastlingGen;
constexpr auto flat_castling = CastlingGen::get_array();

typedef typename CastlingGen::next_engine EngineAfterCastling;

// 3. Zobrist::en_passant_file: std::array<uint64_t, 8>
typedef GenerateSequence<EngineAfterCastling, 8> EpGen;
constexpr auto flat_ep = EpGen::get_array();

typedef typename EpGen::next_engine EngineAfterEp;

// 4. Zobrist::black_move: uint64_t
typedef GenerateSequence<EngineAfterEp, 1> BlackMoveGen;
constexpr auto flat_black = BlackMoveGen::get_array();

// Define inline constexpr variables to be used throughout the engine.
inline constexpr auto hash = create_hash_3d(flat_hash);
inline constexpr auto castling = flat_castling;
inline constexpr auto en_passant_file = flat_ep;
inline constexpr uint64_t black_move = flat_black[0];

} // namespace Zobrist
