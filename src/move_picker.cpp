#include "stargaze/move_picker.hpp"

#include "stargaze/board.hpp"
#include "stargaze/eval.hpp"
#include "stargaze/search.hpp"
#include <algorithm>
#include <cassert>

int MovePicker::score_move(const Search &search, Move move, int see,
                           std::optional<Move> pv_move,
                           std::optional<Move> tt_move,
                           std::optional<uint16_t> ply) {
    if (move == pv_move)
        return PV_MOVE_SCORE;
    if (move == tt_move)
        return TT_MOVE_SCORE;

    if (move.is_capture() || move.is_promotion()) {
        return see >= 0 ? GOOD_TACTICAL_SCORE + std::min(see, 10000)
                        : BAD_TACTICAL_SCORE + std::max(see, -10000);
    }

    if (ply && *ply < search.killers.size()) {
        const auto &killers = search.killers[*ply];
        for (size_t i = 0; i < killers.size(); i++) {
            if (move == killers[i])
                return KILLER_SCORES[i];
        }
    }

    return QUIET_SCORE + search.history_score(move);
}

MovePicker::MovePicker(Search &search, bool tactical_only,
                       std::optional<Move> pv_move, std::optional<Move> tt_move,
                       std::optional<uint16_t> ply,
                       const std::vector<Move> *allowed_moves) {
    auto emit = [&](Move move) {
        if (allowed_moves &&
            std::find(allowed_moves->begin(), allowed_moves->end(), move) ==
                allowed_moves->end())
            return;

        assert(count < moves.size());
        const bool tactical = move.is_capture() || move.is_promotion();
        const int see_score = tactical && move != pv_move && move != tt_move
                                  ? see(*search.board, move)
                                  : 0;
        moves[count++] = {
            score_move(search, move, see_score, pv_move, tt_move, ply), move,
            see_score < 0};
    };

    if (tactical_only)
        search.board->generate_moves<true, false, false, true, false>(emit);
    else
        search.board->generate_moves(emit);
}

std::optional<MovePicker::ScoredMove> MovePicker::next() {
    if (index == count)
        return std::nullopt;

    size_t best = index;
    for (size_t i = index + 1; i < count; i++) {
        if (moves[i].score > moves[best].score)
            best = i;
    }

    std::swap(moves[index], moves[best]);
    return moves[index++];
}

BitBoard MovePicker::least_valuable_attacker(const Board &board,
                                             BitBoard candidates, Colour side,
                                             Square target, BitBoard occupied,
                                             Piece &piece) {
    candidates &= board.get_bb(side);
    const Square king = board.get_bb(PP::KING, side).lsb_square();

    for (piece = PP::PAWN; piece <= PP::KING; piece++) {
        BitBoard subset = candidates & board.get_bb(piece);
        while (subset.has_square()) {
            const BitBoard from = subset.lsb();
            const Square from_square = from.lsb_square();
            const Square king_after = piece == PP::KING ? target : king;

            if (piece != PP::KING &&
                !Mask::queens(king).has_square(from_square))
                return from;

            BitBoard occupied_after = occupied ^ from;
            occupied_after.set_square(target);
            if (!board
                     .attackers(side.opposite(), king_after, occupied_after,
                                BitBoard(target))
                     .has_square())
                return from;

            subset ^= from;
        }
    }

    return BitBoard::EMPTY;
}

int MovePicker::see(const Board &board, Move move) {
    const Square from = move.from();
    const Square target = move.to();
    const Piece moving_piece = *board.get_piece(from);
    Piece target_piece =
        move.is_promotion() ? move.promotion_piece() : moving_piece;
    BitBoard occupied = board.get_bb(CC::WHITE) | board.get_bb(CC::BLACK);
    std::array<int, 32> gain{};
    int depth = 0;

    if (move.is_capture()) {
        gain[0] = Eval::value(move.is_en_passant() ? PP::PAWN
                                                   : *board.get_piece(target));
    }
    if (move.is_promotion())
        gain[0] += Eval::value(target_piece) - Eval::value(PP::PAWN);

    occupied.erase_square(from);
    if (move.is_en_passant())
        occupied.erase_square(target - 8 * board.turn.weight());
    occupied.set_square(target);

    Colour side = board.turn.opposite();
    while (depth < 31) {
        const BitBoard current_attackers =
            board.attackers(side, target, occupied);
        Piece attacker_piece;
        const BitBoard from_set = least_valuable_attacker(
            board, current_attackers, side, target, occupied, attacker_piece);
        if (!from_set.has_square())
            break;

        const bool promotes = attacker_piece == PP::PAWN &&
                              (target.rank() == 0 || target.rank() == 7);
        const int promotion_gain =
            promotes ? Eval::value(PP::QUEEN) - Eval::value(PP::PAWN) : 0;
        depth++;
        gain[depth] =
            (target_piece == PP::KING ? 32000 : Eval::value(target_piece)) +
            promotion_gain - gain[depth - 1];

        occupied ^= from_set;
        target_piece = promotes ? PP::QUEEN : attacker_piece;
        side = side.opposite();
    }

    while (depth > 0) {
        gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
        depth--;
    }

    return gain[0];
}
