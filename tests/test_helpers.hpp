#pragma once
#include <string_view>
#include <vector>

namespace test {
constexpr std::string_view START_POSITION =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr std::string_view KIWIPETE =
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

constexpr std::string_view CPW_POSITION_3 =
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr std::string_view CPW_POSITION_4 =
    "r3k2r/pbp1qbp1/1p1pn1p1/1P1PN3/1p2P3/2N2Q1p/2PB1PPP/R3K2R b KQkq - 0 1";
constexpr std::string_view CPW_POSITION_5 =
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr std::string_view CPW_POSITION_6 =
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/2NP1N2/PPP1QPPP/R3K2R w KQ - 0 10";

constexpr std::string_view CASTLING_EP_POSITION =
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b Kq e3 0 1";
constexpr std::string_view ASYMMETRIC_POSITION =
    "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d3 0 2";
constexpr std::string_view SCHOLARS_MATE =
    "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 4 4";
constexpr std::string_view CRASHING_POSITION =
    "4R3/8/P1N4P/8/8/3pk1K1/ppr2R2/6Q1 w - - 0 1";

// Endgames / Minimal positions.
constexpr std::string_view EMPTY_BOARD = "8/8/8/8/8/8/8/8 w - - 0 1";
constexpr std::string_view KINGS_ONLY = "k7/8/8/8/8/8/8/7K b - - 15 40";

// Group for round-trip serialization tests.
inline const std::vector<std::string_view> SERIALIZATION_FENS = {
    START_POSITION,       KIWIPETE,       CPW_POSITION_3,
    CPW_POSITION_4,       CPW_POSITION_5, CPW_POSITION_6,
    CASTLING_EP_POSITION, EMPTY_BOARD,    KINGS_ONLY};

// Group for evaluation symmetry tests.
inline const std::vector<std::string_view> EVAL_SYMMETRY_FENS = {
    START_POSITION,
    KIWIPETE,
    CPW_POSITION_3,
    CPW_POSITION_5,
    CASTLING_EP_POSITION,
    ASYMMETRIC_POSITION,
    KINGS_ONLY};
} // namespace test
