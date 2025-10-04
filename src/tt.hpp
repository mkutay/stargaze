#include <unordered_map>
#include <cassert>

struct Entry {
    u_int64_t hash;
    int depth;
    int score; // evaluation
};

// transposition table
class TT {
    std::unordered_map<u_int64_t, Entry> entries;
public:
    bool is_hash_present(u_int64_t hash) { return entries.find(hash) != entries.end(); }
    Entry get_entry(u_int64_t hash) { assert(is_hash_present(hash)); return entries[hash]; }
    void set_entry(u_int64_t hash, Entry entry) { entries[hash] = entry; }
};