#include "debug.hpp"
#include "move.hpp"
#include "search.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::string to_string(const std::string &s) { return std::format("'{}'", s); }
std::string to_string(const char &c) { return std::format("'{}'", c); }
std::string to_string(const char *c) { return std::string(c); }
std::string to_string(const bool &b) { return (b ? "true" : "false"); }
std::string to_string(const std::vector<bool> &v) {
    std::string res = "{";
    for (int i = 0; i < (int)v.size(); ++i) {
        if (i > 0) {
            res += ", ";
        }
        res += std::to_string(v[i]);
    }
    res += "}";
    return res;
}
std::string to_string(const Bound bound) {
    const static std::map<Bound, std::string> bound_to_string = {
        {Bound::EXACT, "EXACT"},
        {Bound::LOWER, "LOWER"},
        {Bound::UPPER, "UPPER"},
        {Bound::NONE, "NONE"}};

    return bound_to_string.at(bound);
}
std::string to_string(const Move move) {
    return std::format("[{} {} {}]", (int)move.from(), (int)move.to(),
                       (int)move.flags());
}

std::string to_string(const SearchInfo result) {
    const int nps =
        result.time_ms != 0 ? (result.nodes * 1000ll) / result.time_ms : 0;
    return "{" +
           std::format(
               "depth: {}, score: {}, nodes: {}, time_ms: {}, stopped: {}, pv: "
               "{}, "
               "nps: {}",
               result.depth, result.score, result.nodes, result.time_ms,
               to_string(result.stopped), to_string(result.pv.moves), nps) +
           "}";
}

void debug_out([[maybe_unused]] int size, [[maybe_unused]] bool first,
               [[maybe_unused]] std::string name) {
    std::cerr << '\n';
}
