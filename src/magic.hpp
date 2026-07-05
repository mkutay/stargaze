#pragma once
#include "bitboard.hpp"
#include "mask.hpp"
#include "square.hpp"
#include <array>
#include <cstdint>

namespace Magic {
namespace detail {

template <size_t N>
constexpr BitBoard slider_mask(Square sq, const std::array<int8_t, N> &moves) {
    BitBoard mask = 0;
    for (auto dir : moves) {
        for (auto cur = sq.move(dir); cur && cur->move(dir);
             cur = cur->move(dir)) {
            mask.set_bit(*cur);
        }
    }
    return mask;
}

template <size_t N>
constexpr BitBoard attacks_slow(Square sq, BitBoard blockers,
                                const std::array<int8_t, N> &moves) {
    BitBoard attacks = 0;
    for (auto dir : moves) {
        for (auto cur = sq.move(dir); cur; cur = cur->move(dir)) {
            attacks.set_bit(*cur);
            if (blockers.has_square(*cur))
                break;
        }
    }
    return attacks;
}

constexpr int compute_table_size(bool is_rook) {
    int total = 0;
    for (int sq = 0; sq < 64; sq++) {
        BitBoard mask = is_rook ? slider_mask(sq, Mask::CARDINAL_MOVES)
                                : slider_mask(sq, Mask::DIAGONAL_MOVES);
        total += 1 << mask.count();
    }
    return total;
}

constexpr int ROOK_TABLE_SIZE = compute_table_size(true);    // 102400
constexpr int BISHOP_TABLE_SIZE = compute_table_size(false); // 5248

struct Entry {
    BitBoard mask = 0;
    BitBoard magic = 0;
    int shift = 0;
    int offset = 0;
};

constexpr std::array<BitBoard, 64> BISHOP_MAGICS = {
    0xa0043002232020ULL,   0x8620a10a4a014000ULL, 0xc1000822f400080ULL,
    0x80404008a900004ULL,  0x6021180000020ULL,    0x1042004400001ULL,
    0x411010803408204ULL,  0x4000104e08201810ULL, 0x641080230443102ULL,
    0x10002106a020148ULL,  0x5000844403821000ULL, 0x160051040088000aULL,
    0x50040420000004ULL,   0x400008821081440ULL,  0x21008948c104020ULL,
    0xc0c044400880800ULL,  0x20401004018800ULL,   0x408001242483209ULL,
    0x1510002a04009322ULL, 0xc10c210802002002ULL, 0x4104020280e00080ULL,
    0x40002002100c2002ULL, 0x2010600c02080400ULL, 0x8000208042080400ULL,
    0x1404220010200120ULL, 0x511900004105a00ULL,  0x10a80800040a2220ULL,
    0x8080000820002ULL,    0x20840204802002ULL,   0x18020000490404ULL,
    0x7230024240b02ULL,    0x2108508085040100ULL, 0x202082000042000ULL,
    0x1084640440121001ULL, 0x1082010644140800ULL, 0x12008020420201ULL,
    0x40010200010880ULL,   0x8000850100021000ULL, 0x4041220422008408ULL,
    0x4e08211820004205ULL, 0x401303010840508ULL,  0xa282108000422ULL,
    0x2409004822007000ULL, 0xac0004200800802ULL,  0xc81340094000200ULL,
    0x2040500050c02180ULL, 0x2408100900400218ULL, 0x2020405020420ULL,
    0xa110622210400080ULL, 0x20904110100000ULL,   0x300011041103000ULL,
    0x2252110210440010ULL, 0x8209640620820028ULL, 0xc0052004010210ULL,
    0x8004080288120000ULL, 0x22928401060193ULL,   0x5892410040400ULL,
    0x204422080250ULL,     0x104901404c1022ULL,   0x6010262001421604ULL,
    0x104a01310820202ULL,  0x1000402044010204ULL, 0x40001020028c0440ULL,
    0x20e0040508002980ULL};

constexpr std::array<BitBoard, 64> ROOK_MAGICS = {
    0x80008020400010ULL,   0x40200040001000ULL,   0x1280082001100080ULL,
    0x60008200c704200ULL,  0x2500041048010002ULL, 0x500040008210012ULL,
    0x100010045a40a00ULL,  0x1200020021104084ULL, 0x800080204000ULL,
    0x210140002000d000ULL, 0x420801000802004ULL,  0x4c08800800100481ULL,
    0x810808004008800ULL,  0x812001082000408ULL,  0x874000488010210ULL,
    0x20028001c2800100ULL, 0x30410020800104ULL,   0x404000201003ULL,
    0x2890002008002400ULL, 0x40c1010008100020ULL, 0x3000808004000802ULL,
    0x202008080040002ULL,  0x8543010100040200ULL, 0x40004200210290ccULL,
    0x920800280204008ULL,  0x4000200080400080ULL, 0x490040020080020ULL,
    0x400401200082201ULL,  0x8004040080800800ULL, 0x83020080040080ULL,
    0x1000080400020110ULL, 0x141000100285082ULL,  0x420324001800080ULL,
    0x2000200080804000ULL, 0x802008801000ULL,     0x4180805002802801ULL,
    0x4a18010045001048ULL, 0x404020080800400ULL,  0x840100204000108ULL,
    0x10088842000401ULL,   0x80800040028020ULL,   0x200500020004001ULL,
    0x1400200702410010ULL, 0xd800100300090020ULL, 0x48008004008008ULL,
    0x101100440080120ULL,  0x2080020001008080ULL, 0x1100005081020004ULL,
    0x20800020400080ULL,   0x8000400020009080ULL, 0x2400102001044500ULL,
    0x2000080180100180ULL, 0x4c004800816580ULL,   0x8102820080840080ULL,
    0x1381000200241100ULL, 0x80004c08812200ULL,   0x2000800102c06315ULL,
    0x400085110260c3ULL,   0x2021008214082ULL,    0x2042101100009ULL,
    0x202013060082402ULL,  0x4002000810010402ULL, 0x2308500a8204ULL,
    0x251000040802201ULL};

constexpr std::array<std::array<BitBoard, 64>, 64> generate_ray_between() {
    std::array<std::array<BitBoard, 64>, 64> table{};
    for (Square sq1 = 0; sq1 < 64; sq1++) {
        for (Square sq2 = 0; sq2 < 64; sq2++) {
            if (sq1 == sq2)
                continue;
            for (auto dir : Mask::ALL_MOVES) {
                bool found = false;
                BitBoard path = 0;
                for (auto cur = sq1.move(dir); cur; cur = cur->move(dir)) {
                    if (*cur == sq2) {
                        found = true;
                        break;
                    }
                    path.set_bit(*cur);
                }
                if (found) {
                    table[sq1][sq2] = path;
                    break;
                }
            }
        }
    }
    return table;
}

} // namespace detail

struct MagicKeys {
    std::array<detail::Entry, 64> rook;
    std::array<detail::Entry, 64> bishop;
    std::array<BitBoard, detail::ROOK_TABLE_SIZE> rook_table;
    std::array<BitBoard, detail::BISHOP_TABLE_SIZE> bishop_table;

    constexpr MagicKeys() : rook{}, bishop{}, rook_table{}, bishop_table{} {
        auto init = [&](auto &entries, auto &table, const auto &moves,
                        const auto &magics) {
            int offset = 0;
            for (Square sq = 0; sq < 64; sq++) {
                BitBoard mask = detail::slider_mask(sq, moves);
                int bits = mask.count();
                int n = 1 << bits;

                BitBoard magic = magics[sq];
                entries[sq] = {mask, magic, 64 - bits, offset};

                BitBoard occ = 0;
                for (int i = 0; i < n; i++) {
                    int idx = static_cast<int>((occ * magic) >> (64 - bits));
                    table[offset + idx] = detail::attacks_slow(sq, occ, moves);
                    occ = (occ - mask) & mask;
                }

                offset += n;
            }
        };

        init(rook, rook_table, Mask::CARDINAL_MOVES, detail::ROOK_MAGICS);
        init(bishop, bishop_table, Mask::DIAGONAL_MOVES, detail::BISHOP_MAGICS);
    }
};

extern const MagicKeys keys;

inline constexpr BitBoard rook_attacks(Square sq, BitBoard occupancy) {
    const auto &e = keys.rook[sq];
    int index = ((occupancy & e.mask) * e.magic) >> e.shift;
    return BitBoard(keys.rook_table[e.offset + index]);
}

inline constexpr BitBoard bishop_attacks(Square sq, BitBoard occupancy) {
    const auto &e = keys.bishop[sq];
    int index = ((occupancy & e.mask) * e.magic) >> e.shift;
    return BitBoard(keys.bishop_table[e.offset + index]);
}

extern const std::array<std::array<BitBoard, 64>, 64> RAY_BETWEEN;

} // namespace Magic
