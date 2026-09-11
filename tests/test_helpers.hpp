#pragma once
#include <cstdint>
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
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
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

// Move-generation edge cases.
constexpr std::string_view ORTHOGONAL_PIN = "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1";
constexpr std::string_view DIAGONAL_PIN = "6k1/8/7b/8/8/8/3B4/2K5 w - - 0 1";
constexpr std::string_view PINNED_PAWN_PUSH =
    "4r1k1/8/8/8/8/8/4P3/4K3 w - - 0 1";
constexpr std::string_view PINNED_PAWN_CAPTURES_PINNER =
    "6k1/8/8/8/8/4b3/3P4/2K5 w - - 0 1";
constexpr std::string_view PINNED_KNIGHT = "4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1";
constexpr std::string_view FALSE_POSITIVE_PIN =
    "6k1/8/8/8/5b2/8/3R4/4K3 w - - 0 1";
constexpr std::string_view DOUBLE_CHECK = "4r1k1/8/8/8/1b6/8/8/4K3 w - - 0 1";
constexpr std::string_view KING_XRAY_CAPTURE =
    "4r1k1/8/8/8/8/8/4K3/4n3 w - - 0 1";
constexpr std::string_view EN_PASSANT_CHECK_EVASION =
    "4k3/8/8/3pP3/4K3/8/8/8 w - d6 0 1";
constexpr std::string_view EN_PASSANT_EXPOSES_ROOK =
    "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1";
constexpr std::string_view CASTLING_THROUGH_QUEEN_ATTACK =
    "4k3/8/q7/8/8/8/8/4K2R w K - 0 1";

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

// Perft test cases (expected node counts for depth 1, 2, ..., N).
struct PerftTestCase {
    std::string_view name;
    std::string_view fen;
    std::vector<uint64_t> expected_nodes;
};

inline const std::vector<PerftTestCase> PERFT_TEST_CASES = {
    {"start position", START_POSITION, {20, 400, 8902, 197281}},
    {"kiwipete", KIWIPETE, {48, 2039, 97862}},
    {"CPW position 3", CPW_POSITION_3, {14, 191, 2812, 43238}},
    {"CPW position 4", CPW_POSITION_4, {6, 264, 9467}},
    {"CPW position 5", CPW_POSITION_5, {44, 1486, 62379}},
    {"CPW position 6", CPW_POSITION_6, {45, 2033, 87459}},
    {"orthogonal pin", ORTHOGONAL_PIN, {10, 129, 1775, 25563}},
    {"diagonal pin", DIAGONAL_PIN, {8, 74, 866, 9232}},
    {"pinned pawn push", PINNED_PAWN_PUSH, {6, 93, 571, 9660}},
    {"pinned pawn captures pinner",
     PINNED_PAWN_CAPTURES_PINNER,
     {5, 65, 504, 6782}},
    {"pinned knight", PINNED_KNIGHT, {4, 64, 651, 11486}},
    {"unrelated slider is not a pin",
     FALSE_POSITIVE_PIN,
     {18, 254, 3969, 51633}},
    {"double check", DOUBLE_CHECK, {3, 78, 274, 6955}},
    {"king cannot capture into slider x-ray",
     KING_XRAY_CAPTURE,
     {4, 80, 263, 5539}},
    {"en passant check evasion", EN_PASSANT_CHECK_EVASION, {8, 44, 316, 1951}},
    {"en passant exposes rook", EN_PASSANT_EXPOSES_ROOK, {4, 66, 289, 5098}},
    {"castling through queen attack",
     CASTLING_THROUGH_QUEEN_ATTACK,
     {12, 284, 3921, 92735}},
};
} // namespace test
