#pragma once
#include "stargaze/move.hpp"
#include "stargaze/score.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

enum class Bound : uint8_t {
    NONE = 0,  // entry is empty/invalid
    EXACT = 1, // PV-node (exact score)
    LOWER = 2, // all-node (failed high, beta cutoff)
    UPPER = 3  // cut-node (failed low, no move improved alpha)
};

struct TTEntry {
    uint64_t hash;
    Score score;
    uint32_t age;
    std::optional<Move> best_move;
    uint8_t depth;
    uint8_t halfmove_clock;
    Bound bound;
    // we can get three more bytes for free!
};

static_assert(sizeof(TTEntry) == 24);

struct alignas(128) TTCluster {
    constexpr static size_t SIZE = 5;
    std::array<TTEntry, SIZE> entries;
};

static_assert(sizeof(TTCluster) == 128);

class TT {
  private:
    std::vector<TTCluster> table;
    size_t cluster_count;
    uint32_t current_age;

    size_t index(uint64_t hash) const { return hash & (cluster_count - 1); }
    int replacement_quality(const TTEntry &entry) const;

  public:
    explicit TT(size_t megabytes = 64);
    void resize(size_t megabytes);
    void clear();
    void new_search();
    TTEntry *probe(uint64_t hash);
    void prefetch(uint64_t hash) const;
    void store(uint64_t hash, std::optional<Move> best_move, Score score,
               uint8_t depth, uint8_t halfmove_clock, Bound bound);
    int hashfull() const;
};
