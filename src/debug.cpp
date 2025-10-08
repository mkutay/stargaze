#include "debug.hpp"

std::string to_string(const std::string& s) {
    return '"' + s + '"';
}

std::string to_string(const char& c) {
    return std::string("'") + c + "'";
}

std::string to_string(const char *c) {
    return std::string(c);
}

std::string to_string(const bool& b) {
    return (b ? "true" : "false");
}

std::string to_string(const std::vector<bool>& v) {
    std::string res = "{";
    for (int i = 0; i < (int) v.size(); ++i) {
        if (i > 0) {
            res += ", ";
        }
        res += std::to_string(v[i]);
    }
    res += "}";
    return res;
}

std::string to_string(const Bound bound) {
    if (bound == BOUND_NONE) return "NONE";
    if (bound == BOUND_EXACT) return "EXACT";
    if (bound == BOUND_LOWER) return "LOWER";
    return "UPPER";
}

std::string to_string(const Move move) {
    std::string ret = "[";
    ret += std::to_string((int) move.from()) + " ";
    ret += std::to_string((int) move.to()) + " ";
    ret += std::to_string((int) move.flags());
    ret += "]";
    return ret;
}

std::string to_string(const SearchInfo result) {
    std::string ret = "{depth: " + std::to_string(result.depth);
    ret += ", score: " + to_string(result.score);
    ret += ", nodes: " + to_string(result.nodes);
    ret += ", time_ms: " + to_string(result.time_ms);
    ret += ", stopped: " + to_string(result.stopped ? "true" : "false");
    ret += ", pv: " + to_string(result.pv.moves);
    ret += ", nps: " + to_string(result.time_ms > 0 ? (result.nodes * 1000ll) / result.time_ms : 0);
    ret += "}";
    return ret;
}

void debug_out([[maybe_unused]] int size, [[maybe_unused]] bool first, [[maybe_unused]] std::string name) {
    std::cerr << std::endl;
}