#pragma once
#include "bitboard.hpp"
#include "square.hpp"
#include <array>
#include <bit>
#include <cstdlib>

namespace Mask {

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
constexpr std::array<BitBoard, 64>
generate_masks(const std::array<int8_t, N> &moves) {
    auto masks =
        std::bit_cast<std::array<BitBoard, 64>>(std::array<uint64_t, 64>{});
    for (Square sq = 0; sq < 64; sq++) {
        for (auto dir : moves) {
            if constexpr (once) {
                if (auto next = sq.move(dir))
                    masks[sq] |= BitBoard(*next);
            } else {
                for (auto cur = sq.move(dir); cur; cur = cur->move(dir))
                    masks[sq] |= BitBoard(*cur);
            }
        }
    }
    return masks;
}

constexpr size_t moving_masks_index(int8_t move) { return move + 63; }

/**
 * For each "move" (from -63 to +63, inclusive), have a bitboard to mask
 * before applying the move, to remove moves that can't be made.
 *
 * In other words, the mask here represents where you could "start" your move
 * from.
 */
constexpr std::array<BitBoard, 127> generate_moving_masks() {
    // Zero initialise the bitboards.
    auto masks =
        std::bit_cast<std::array<BitBoard, 127>>(std::array<uint64_t, 127>{});

    for (int8_t move = -63; move <= 63; move++) {
        for (Square sq = 0; sq < 64; sq++) {
            if (sq.move(move))
                masks[moving_masks_index(move)].set_bit(sq);
        }
    }

    return masks;
}

constexpr const static std::array<int8_t, 8> KNIGHT_MOVES = {-10, -6, -17, -15,
                                                             6,   10, 15,  17};
constexpr const static std::array<int8_t, 4> DIAGONAL_MOVES = {-9, -7, 7, 9};
constexpr const static std::array<int8_t, 4> CARDINAL_MOVES = {-8, -1, 1, 8};

constexpr const static auto KNIGHT_MASKS = generate_masks<true>(KNIGHT_MOVES);
constexpr const static auto BISHOP_MASKS =
    generate_masks<false>(DIAGONAL_MOVES);
constexpr const static auto ROOK_MASKS = generate_masks<false>(CARDINAL_MOVES);
constexpr const static auto QUEEN_MASKS =
    generate_masks<false>(CARDINAL_MOVES + DIAGONAL_MOVES);
constexpr const static auto KING_MASKS =
    generate_masks<true>(CARDINAL_MOVES + DIAGONAL_MOVES);

constexpr const static auto MOVING_MASKS = generate_moving_masks();

constexpr BitBoard moving_mask(int8_t move) {
    return MOVING_MASKS[moving_masks_index(move)];
}

template <size_t N>
constexpr std::array<BitBoard, N>
moving_masks_for(const std::array<int8_t, N> &moves) {
    std::array<BitBoard, N> result =
        std::bit_cast<std::array<BitBoard, N>>(std::array<uint64_t, N>{});
    for (size_t i = 0; i < N; i++)
        result[i] = moving_mask(moves[i]);
    return result;
}

constexpr const static auto KNIGHT_MOVING_MASKS =
    moving_masks_for(KNIGHT_MOVES);
constexpr const static auto DIAGONAL_MOVING_MASKS =
    moving_masks_for(DIAGONAL_MOVES);
constexpr const static auto CARDINAL_MOVING_MASKS =
    moving_masks_for(CARDINAL_MOVES);

constexpr static const BitBoard FILE_A = BitBoard::FILE_A;
constexpr static const BitBoard FILE_B = BitBoard::FILE_B;
constexpr static const BitBoard FILE_C = BitBoard::FILE_C;
constexpr static const BitBoard FILE_D = BitBoard::FILE_D;
constexpr static const BitBoard FILE_E = BitBoard::FILE_E;
constexpr static const BitBoard FILE_F = BitBoard::FILE_F;
constexpr static const BitBoard FILE_G = BitBoard::FILE_G;
constexpr static const BitBoard FILE_H = BitBoard::FILE_H;

constexpr static const BitBoard RANK_1 = BitBoard::RANK_1;
constexpr static const BitBoard RANK_2 = BitBoard::RANK_2;
constexpr static const BitBoard RANK_3 = BitBoard::RANK_3;
constexpr static const BitBoard RANK_4 = BitBoard::RANK_4;
constexpr static const BitBoard RANK_5 = BitBoard::RANK_5;
constexpr static const BitBoard RANK_6 = BitBoard::RANK_6;
constexpr static const BitBoard RANK_7 = BitBoard::RANK_7;
constexpr static const BitBoard RANK_8 = BitBoard::RANK_8;

constexpr static const std::array<BitBoard, 8> FILES = {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};

constexpr static const std::array<BitBoard, 8> RANKS = {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

constexpr static const BitBoard LIGHT_SQUARES = BitBoard::LIGHT_SQUARES;
constexpr static const BitBoard DARK_SQUARES = BitBoard::DARK_SQUARES;
constexpr static const BitBoard EMPTY = BitBoard::EMPTY;
constexpr static const BitBoard ALL_SQUARES = BitBoard::ALL_SQUARES;
constexpr static const BitBoard EDGE_SQUARES = BitBoard::EDGE_SQUARES;
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
static_assert(Mask::MOVING_MASKS[63] == Mask::ALL_SQUARES);
static_assert(Mask::MOVING_MASKS[53] == BitBoard(0xfcfcfcfcfcfcfc00ull));
static_assert(Mask::MOVING_MASKS[56] == BitBoard(0x7f7f7f7f7f7f7f00ull));
static_assert(sizeof(Mask::MOVING_MASKS) == sizeof(BitBoard) * 127);
static_assert(Mask::KNIGHT_MOVING_MASKS[1] == Mask::MOVING_MASKS[57]);
