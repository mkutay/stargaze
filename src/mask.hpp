#pragma once
#include "bitboard.hpp"
#include "square.hpp"
#include <array>
#include <bit>
#include <cstdlib>

namespace Mask {

constexpr const std::array<int8_t, 8> KNIGHT_MOVES = {-10, -6, -17, -15,
                                                      6,   10, 15,  17};
constexpr const std::array<int8_t, 4> DIAGONAL_MOVES = {-9, -7, 7, 9};
constexpr const std::array<int8_t, 4> CARDINAL_MOVES = {-8, -1, 1, 8};

template <typename T, size_t N, size_t M>
constexpr std::array<T, N + M> operator+(const std::array<T, N> &a,
                                         const std::array<T, M> &b) {
    std::array<T, N + M> result{};
    for (size_t i = 0; i < N; ++i) {
        result[i] = a[i];
    }
    for (size_t i = 0; i < M; ++i) {
        result[N + i] = b[i];
    }
    return result;
}

template <bool once, size_t N>
constexpr BitBoard mask_for(Square sq, const std::array<int8_t, N> &moves) {
    BitBoard mask = 0;
    for (auto move : moves) {
        if (once) {
            auto next = sq.move(move);
            if (next)
                mask |= BitBoard(*next);
            continue;
        }

        auto current = sq;
        while (true) {
            auto next = current.move(move);
            if (!next)
                break;
            mask |= BitBoard(*next);
            current = *next;
        }
    }
    return mask;
}

template <bool once, size_t N>
constexpr std::array<BitBoard, 64>
generate_masks(const std::array<int8_t, N> &moves) {
    std::array<uint64_t, 64> masks = {};
    for (uint8_t sq = 0; sq < 64; sq++)
        masks[sq] = mask_for<once>(Square(sq), moves);
    return std::bit_cast<std::array<BitBoard, 64>>(masks);
}

constexpr const static std::array<BitBoard, 64> KNIGHT_MASKS =
    generate_masks<true>(KNIGHT_MOVES);
constexpr const static std::array<BitBoard, 64> BISHOP_MASKS =
    generate_masks<false>(DIAGONAL_MOVES);
constexpr const static std::array<BitBoard, 64> ROOK_MASKS =
    generate_masks<false>(CARDINAL_MOVES);
constexpr const static std::array<BitBoard, 64> QUEEN_MASKS =
    generate_masks<false>(CARDINAL_MOVES + DIAGONAL_MOVES);
constexpr const static std::array<BitBoard, 64> KING_MASKS =
    generate_masks<true>(CARDINAL_MOVES + DIAGONAL_MOVES);

constexpr static const auto FILE_A = BitBoard(0x0101010101010101);
constexpr static const auto FILE_B = BitBoard(0x0202020202020202);
constexpr static const auto FILE_C = BitBoard(0x0404040404040404);
constexpr static const auto FILE_D = BitBoard(0x0808080808080808);
constexpr static const auto FILE_E = BitBoard(0x1010101010101010);
constexpr static const auto FILE_F = BitBoard(0x2020202020202020);
constexpr static const auto FILE_G = BitBoard(0x4040404040404040);
constexpr static const auto FILE_H = BitBoard(0x8080808080808080);

constexpr static const auto RANK_1 = BitBoard(0x00000000000000ff);
constexpr static const auto RANK_2 = BitBoard(0x000000000000ff00);
constexpr static const auto RANK_3 = BitBoard(0x0000000000ff0000);
constexpr static const auto RANK_4 = BitBoard(0x00000000ff000000);
constexpr static const auto RANK_5 = BitBoard(0x000000ff00000000);
constexpr static const auto RANK_6 = BitBoard(0x0000ff0000000000);
constexpr static const auto RANK_7 = BitBoard(0x00ff000000000000);
constexpr static const auto RANK_8 = BitBoard(0xff00000000000000);

constexpr static const std::array<BitBoard, 8> FILES = {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};

constexpr static const std::array<BitBoard, 8> RANKS = {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

constexpr static const auto LIGHT_SQUARES = BitBoard(0x55aa55aa55aa55aa);
constexpr static const auto DARK_SQUARES = BitBoard(0xaa55aa55aa55aa55);
constexpr static const auto EMPTY = BitBoard(0x0000000000000000);
constexpr static const auto ALL_SQUARES = BitBoard(0xffffffffffffffff);
constexpr static const auto EDGE_SQUARES = BitBoard(0xff818181818181ff);
}; // namespace Mask

static_assert(Mask::EMPTY.count() == 0);
static_assert(Mask::ALL_SQUARES.count() == 64);
static_assert(Mask::EMPTY.adjacent() == Mask::EMPTY);
static_assert(Mask::LIGHT_SQUARES.count() == 32);
static_assert(Mask::DARK_SQUARES.count() == 32);
static_assert(Mask::ALL_SQUARES.adjacent() == Mask::ALL_SQUARES);
static_assert((~Mask::EMPTY) == Mask::ALL_SQUARES);
static_assert(~Mask::ALL_SQUARES == Mask::EMPTY);
static_assert((Mask::LIGHT_SQUARES & Mask::DARK_SQUARES) == Mask::EMPTY);
static_assert((Mask::LIGHT_SQUARES | Mask::DARK_SQUARES) == Mask::ALL_SQUARES);
static_assert(~Mask::LIGHT_SQUARES == Mask::DARK_SQUARES);
static_assert(~Mask::DARK_SQUARES == Mask::LIGHT_SQUARES);
static_assert(Mask::EDGE_SQUARES ==
              (Mask::FILE_A | Mask::FILE_H | Mask::RANK_1 | Mask::RANK_8));
static_assert(Mask::RANK_1.north() == Mask::RANK_2);
static_assert(Mask::RANK_2.north() == Mask::RANK_3);
static_assert(Mask::RANK_3.north() == Mask::RANK_4);
static_assert(Mask::RANK_4.north() == Mask::RANK_5);
static_assert(Mask::RANK_5.north() == Mask::RANK_6);
static_assert(Mask::RANK_6.north() == Mask::RANK_7);
static_assert(Mask::RANK_7.north() == Mask::RANK_8);
static_assert(Mask::RANK_8.north() == Mask::EMPTY);
static_assert(Mask::RANK_1.south() == Mask::EMPTY);
static_assert(Mask::RANK_2.south() == Mask::RANK_1);
static_assert(Mask::RANK_3.south() == Mask::RANK_2);
static_assert(Mask::RANK_4.south() == Mask::RANK_3);
static_assert(Mask::RANK_5.south() == Mask::RANK_4);
static_assert(Mask::RANK_6.south() == Mask::RANK_5);
static_assert(Mask::RANK_7.south() == Mask::RANK_6);
static_assert(Mask::RANK_8.south() == Mask::RANK_7);
static_assert(Mask::FILE_A.east() == Mask::FILE_B);
static_assert(Mask::FILE_B.east() == Mask::FILE_C);
static_assert(Mask::FILE_C.east() == Mask::FILE_D);
static_assert(Mask::FILE_D.east() == Mask::FILE_E);
static_assert(Mask::FILE_E.east() == Mask::FILE_F);
static_assert(Mask::FILE_F.east() == Mask::FILE_G);
static_assert(Mask::FILE_G.east() == Mask::FILE_H);
static_assert(Mask::FILE_H.east() == Mask::EMPTY);
static_assert(Mask::FILE_A.west() == Mask::EMPTY);
static_assert(Mask::FILE_B.west() == Mask::FILE_A);
static_assert(Mask::FILE_C.west() == Mask::FILE_B);
static_assert(Mask::FILE_D.west() == Mask::FILE_C);
static_assert(Mask::FILE_E.west() == Mask::FILE_D);
static_assert(Mask::FILE_F.west() == Mask::FILE_E);
static_assert(Mask::FILE_G.west() == Mask::FILE_F);
static_assert(Mask::FILE_H.west() == Mask::FILE_G);
static_assert(Mask::FILE_A.adjacent() == (Mask::FILE_A | Mask::FILE_B));
static_assert(Mask::FILE_B.adjacent() ==
              (Mask::FILE_A | Mask::FILE_B | Mask::FILE_C));
static_assert(Mask::FILE_C.adjacent() ==
              (Mask::FILE_B | Mask::FILE_C | Mask::FILE_D));
static_assert(Mask::FILE_D.adjacent() ==
              (Mask::FILE_C | Mask::FILE_D | Mask::FILE_E));
static_assert(Mask::FILE_E.adjacent() ==
              (Mask::FILE_D | Mask::FILE_E | Mask::FILE_F));
static_assert(Mask::FILE_F.adjacent() ==
              (Mask::FILE_E | Mask::FILE_F | Mask::FILE_G));
static_assert(Mask::FILE_G.adjacent() ==
              (Mask::FILE_F | Mask::FILE_G | Mask::FILE_H));
static_assert(Mask::FILE_H.adjacent() == (Mask::FILE_G | Mask::FILE_H));
static_assert(Mask::RANK_1.adjacent() == (Mask::RANK_1 | Mask::RANK_2));
static_assert(Mask::RANK_2.adjacent() ==
              (Mask::RANK_1 | Mask::RANK_2 | Mask::RANK_3));
static_assert(Mask::RANK_3.adjacent() ==
              (Mask::RANK_2 | Mask::RANK_3 | Mask::RANK_4));
static_assert(Mask::RANK_4.adjacent() ==
              (Mask::RANK_3 | Mask::RANK_4 | Mask::RANK_5));
static_assert(Mask::RANK_5.adjacent() ==
              (Mask::RANK_4 | Mask::RANK_5 | Mask::RANK_6));
static_assert(Mask::RANK_6.adjacent() ==
              (Mask::RANK_5 | Mask::RANK_6 | Mask::RANK_7));
static_assert(Mask::RANK_7.adjacent() ==
              (Mask::RANK_6 | Mask::RANK_7 | Mask::RANK_8));
static_assert(Mask::RANK_8.adjacent() == (Mask::RANK_7 | Mask::RANK_8));
static_assert(Mask::ROOK_MASKS.at(SQ::D5) ==
              ((Mask::RANK_5 | Mask::FILE_D) & (~BB::D5)));
static_assert(Mask::BISHOP_MASKS.at(SQ::A1) ==
              (BB::B2 | BB::C3 | BB::D4 | BB::E5 | BB::F6 | BB::G7 | BB::H8));
static_assert(Mask::KNIGHT_MASKS.at(SQ::H8) == (BB::G6 | BB::F7));
