#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <bitset>
#include <queue>
#include <tuple>
#include "move.hpp"
#include "search.hpp"

using std::to_string;

// Forward declarations for template functions
template<typename A, typename B> std::string to_string(const std::pair<A, B>& p);
template<typename A, typename B, typename C> std::string to_string(const std::tuple<A, B, C>& t);
template<typename A, typename B, typename C, typename D> std::string to_string(const std::tuple<A, B, C, D>& t);

// Function declarations (definitions are in debug.cpp)
std::string to_string(const std::string& s);
std::string to_string(const char& c);
std::string to_string(const char *c);
std::string to_string(const bool& b);
std::string to_string(const std::vector<bool>& v);
std::string to_string(const Move move);
std::string to_string(const SearchInfo result);
std::string to_string(const Bound bound);
void debug_out(int size, bool first, std::string name);

template<size_t T> std::string to_string(const std::bitset<T>& bs) {
    return bs.to_string();
}

template<typename T> std::string to_string(std::queue<T> q) {
    std::string res = "{";
    size_t sz = q.size();
    while (sz--) {
        T cur = q.front();
        q.pop();
        if ((int) res.size() > 1) {
            res += ", ";
        }
        res += to_string(cur);
    }
    res += "}";
    return res;
}

template<typename T, class C> std::string to_string(std::priority_queue<T, std::vector<T>, C> pq) {
    std::string res = "{";
    while (!pq.empty()) {
        T cur = pq.top();
        pq.pop();
        if ((int) res.size() > 1) {
            res += ", ";
        }
        res += to_string(cur);
    }
    res += "}";
    return res;
}

// Template functions (must stay in header)
template<typename T>
std::enable_if_t<!std::is_arithmetic<T>::value, std::string>
to_string(const T& v) {
    std::string res = "{";
    bool first = true;
    for (auto const& el : v) {
        if (!first) res += ", ";
        first = false;
        res += to_string(el);
    }
    res += "}";
    return res;
}

template<typename A, typename B> std::string to_string(const std::pair<A, B>& p) {
    return '(' + to_string(p.first) + ", " + to_string(p.second) + ')';
}
template<typename A, typename B, typename C> std::string to_string(const std::tuple<A, B, C>& t) {
    return '(' + to_string(get<0>(t)) + ", " + to_string(get<1>(t)) + ", " + to_string(get<2>(t)) + ')';
}
template<typename A, typename B, typename C, typename D> std::string to_string(const std::tuple<A, B, C, D>& t) {
    return '(' + to_string(get<0>(t)) + ", " + to_string(get<1>(t)) + ", " + to_string(get<2>(t)) + ", " + to_string(get<3>(t)) + ')';
}

constexpr int buffer_size = 255;

template<typename Head, typename... Tail> void debug_out(int size, bool first, std::string name, Head H, Tail... T) {
    std::string tmp;
    int off = 0;
    while ((!name.empty() && name[0] != ',') || off != 0) {
        tmp += name[0];
        name.erase(name.begin());
        char c = tmp.back();
        if (c == '{' || c == '(') {
            ++off;
        } else if (c == '}' || c == ')'){
            --off;
        }
    }
    if (!name.empty()) {
        name.erase(name.begin());
    }
    if (tmp[0] == ' ') {
        tmp.erase(tmp.begin());
    }

    std::string buff = to_string(H);
    if ((int) buff.size() + size + (int) tmp.size() > buffer_size - 5 && !first) {
        std::cerr << '\n';
        size = 0;
    }
    std::cerr << '[' << tmp << ": " << buff << "] ";
    debug_out(((int) buff.size() + size + (int) tmp.size() + 5) % buffer_size, false, name, T...);
}

#define debug(...) std::cerr << "-> ", debug_out(3, true, std::string(#__VA_ARGS__), __VA_ARGS__)
