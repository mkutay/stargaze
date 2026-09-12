#include "stargaze/time_management.hpp"

#include "stargaze/board.hpp"
#include "stargaze/search.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>

uint32_t calculate_time_limit(const Board &board, const Search &search,
                              uint32_t wtime, uint32_t btime, uint32_t winc,
                              uint32_t binc, uint32_t movestogo) {
    const uint32_t time_left =
        (board.get_turn() == Colour::WHITE) ? wtime : btime;
    const uint32_t inc = (board.get_turn() == Colour::WHITE) ? winc : binc;
    if (time_left <= 1)
        return 1;

    Board position = board;
    const auto legal_moves = position.get_moves();
    if (legal_moves.empty())
        return 1;

    uint32_t piece_count = 0;
    uint32_t non_pawn_material = 0;
    for (Square sq = 0; sq < 64; sq++) {
        const auto piece = board.get_piece(sq);
        if (!piece.has_value())
            continue;
        piece_count++;
        if (*piece != Piece::PAWN && *piece != Piece::KING)
            non_pawn_material += Eval::value(*piece);
    }

    // Fewer pieces generally mean fewer remaining moves, except for pawn-heavy
    // endings where conversion can still take substantial time.
    const uint32_t moves_played = board.get_move_history().size() / 2;
    uint32_t estimated_moves = 0;
    if (piece_count <= 10)
        estimated_moves = 18;
    else if (non_pawn_material <= 2u * Eval::value(Piece::ROOK))
        estimated_moves = 24;
    else
        estimated_moves =
            std::clamp(50u - std::min(moves_played, 30u), 20u, 36u);

    const uint32_t moves_remaining =
        std::max(1u, movestogo > 0 ? movestogo : estimated_moves);

    // Preserve both clock overhead and enough time for later moves. Increment
    // is discounted because it is only earned after making this move.
    const uint64_t overhead = std::min<uint64_t>(50, time_left / 20);
    const uint64_t reserve_moves = std::min<uint32_t>(moves_remaining, 8);
    const uint64_t reserve = std::min<uint64_t>(
        time_left - 1,
        overhead + reserve_moves * std::max<uint64_t>(10, time_left / 200));
    const uint64_t spendable = time_left - reserve;
    uint64_t target = spendable / moves_remaining + uint64_t(inc) * 3 / 4;

    // Branching factor, checks, tactical captures, and a volatile evaluation
    // are useful cheap proxies for positions where extra depth matters.
    uint32_t captures = 0;
    for (Move move : legal_moves)
        captures += move.is_capture() || move.is_promotion();

    uint32_t complexity = 100;
    complexity += std::min<uint32_t>(30, legal_moves.size());
    complexity += std::min<uint32_t>(30, captures * 5);
    if (board.in_check())
        complexity += 25;
    if (std::abs(board.evaluate()) < 2 * Eval::value(Piece::PAWN))
        complexity += 10;

    // A usable previous PV indicates that the search has a stable candidate;
    // if its first move disappeared, allow more time to resolve the change.
    const auto &pv = search.get_last_pv().moves;
    if (!pv.empty()) {
        const bool pv_still_legal =
            std::find(legal_moves.begin(), legal_moves.end(), pv.front()) !=
            legal_moves.end();
        complexity += pv_still_legal ? 0 : 20;
        complexity -= pv_still_legal && pv.size() >= 6 ? 10 : 0;
    }

    target = target * complexity / 140;

    // Never risk the reserve on one move. In severe time trouble, prioritise
    // returning a move over position-dependent extensions.
    uint64_t maximum = spendable;
    if (movestogo == 0 || movestogo > 1)
        maximum = std::min(maximum, uint64_t(time_left) *
                                        (time_left < 1000 ? 25 : 45) / 100);
    const uint64_t minimum =
        std::min<uint64_t>(maximum, std::max<uint64_t>(1, time_left / 200));
    return static_cast<uint32_t>(std::clamp(target, minimum, maximum));
}
