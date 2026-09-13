#pragma once
#include "stargaze/bitboard.hpp"
#include "stargaze/colour.hpp"
#include "stargaze/magic.hpp"
#include "stargaze/mask.hpp"
#include "stargaze/move.hpp"
#include "stargaze/piece.hpp"
#include "stargaze/square.hpp"
#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class MovePicker;

class Board {
    friend class MovePicker;

    std::array<BitBoard, 6> piece_bbs;
    std::array<BitBoard, 2> colour_bbs;
    std::vector<Move> moves;
    Colour turn;

    // 0: white king's side, 1: white queen's side,
    // 2: black king's side, 3: black queen's side
    std::array<bool, 4> can_castle;

    /**
     * The ep_square is the square that can be captured en passant, if any;
     * i.e., the square right behind a double pawn push.
     */
    std::optional<Square> ep_square;

    /**
     * Number of half moves made since the last pawn move or capture, used for
     * the fifty-move rule.
     */
    uint8_t halfmove_clock;

    /**
     * Number of full moves made in the game. This is incremented after Black's
     * move.
     */
    uint16_t fullmove_number;

    /**
     * The hash of the current board state, used for transposition table lookup.
     * Updated incrementally on each move.
     */
    uint64_t current_hash;

    /**
     * The evaluation score of the board in the middle game and end game, for
     * each colour. The final evaluation is a linear interpolation of the two
     * scores based on the game phase.
     */
    std::array<int, 2> mg_score;
    std::array<int, 2> eg_score;
    int game_phase;

    struct UndoInfo {
        Piece moving_piece;
        std::optional<Piece> captured_piece;
        std::array<bool, 4> can_castle;
        std::optional<Square> ep_square;
        uint8_t halfmove_clock;
        uint64_t hash;
    };

    std::vector<UndoInfo> history;

    BitBoard &get_bb(Piece type);
    BitBoard get_bb(Piece type) const;
    BitBoard &get_bb(Colour colour);
    BitBoard get_bb(Colour colour) const;
    BitBoard get_bb(Piece type, Colour colour) const;

    /**
     * Move a piece of the given type and colour from one square to another,
     * updating the bitboards, hash, and evaluation scores accordingly.
     *
     * Note the piece type and colour must match the piece on the from square,
     * otherwise the behaviour is undefined.
     */
    void move_piece(Piece piece, Colour colour, Square from, Square to);

    /**
     * Add a piece of the given type and colour to the given square, updating
     * the bitboards, hash, and evaluation scores accordingly.
     */
    void add_piece(Piece piece, Colour colour, Square sq);

    /**
     * Remove a piece of the given type and colour from the given square,
     * updating the bitboards, hash, and evaluation scores accordingly.
     *
     * Note the piece type and colour must match the piece on the square,
     * otherwise the behaviour is undefined.
     */
    void clear_piece(Piece piece, Colour colour, Square sq);

    /**
     * Return true if the square (a single-bit bitboard) is attacked by the
     * opponent of `by_colour`.
     */
    bool is_attacked(Colour by_colour, BitBoard bb) const;

    uint64_t calculate_hash() const;

    void initialise_eval(std::array<int, 2> &mg_score,
                         std::array<int, 2> &eg_score, int &game_phase) const;

    /**
     * Check that the board state is consistent, i.e., that the current hash
     * matches the actual board state, evaluation data is correct, and that the
     * bitboards are consistent with each other. This is useful for debugging
     * and testing.
     */
    void check_state_consistency() const;

    /**
     * Returns a bitboard of all squares attacked by the given colour.
     */
    BitBoard attacked(Colour attacker) const;

    /**
     * Returns the pieces of the given colour attacking a square.
     */
    BitBoard attackers(Colour attacker, Square square) const;
    BitBoard attackers(Colour attacker, Square square, BitBoard occupied,
                       BitBoard excluded = {}) const;

    /**
     * Returns the squares that can capture or block a lone checker.
     */
    BitBoard check_evasion_targets(Square king, BitBoard checkers) const;

    /**
     * Returns the opposing sliders pinning the given colour to a square.
     */
    BitBoard pinners(Colour colour, Square square) const;

    /**
     * Returns the ray on which a pinned piece may move.
     */
    static BitBoard pin_ray(Square pivot, Square piece, BitBoard pinners);

    /**
     * Returns a bitboard of all squares pinned by the given colour, with the
     * given square as the pivot.
     */
    BitBoard pinned(Colour colour, Square square) const;

    bool gives_check(Move move, BitBoard occupied, Square square) const;

  public:
    Board();
    explicit Board(std::string_view fen);

    uint64_t perft(int depth);
    void make_move(Move move);
    void undo_move();
    void make_null_move();
    void undo_null_move();

    /**
     * Emit legal moves until generation completes or the emitter returns
     * false. An emitter returning void always continues generation.
     */
    template <bool Captures = true, bool KingMoves = true, bool Checks = true,
              bool Promotions = true, bool Quiets = true>
    bool generate_moves(auto &&emitter);
    template <bool Captures = true, bool KingMoves = true, bool Checks = true,
              bool Promotions = true, bool Quiets = true>
    std::vector<Move> get_moves();
    std::optional<Move> get_move();
    bool has_legal_move();
    bool gives_check(Move move) const;
    uint64_t get_hash() const;
    uint8_t get_halfmove_clock() const;
    bool in_check() const;

    /**
     * Check if the given colour has any non-pawn, non-king material.
     */
    bool has_non_pawn_material(Colour colour) const;

    /**
     * Return the evaluation score of the board from the perspective of the
     * current player.
     */
    int evaluate() const;

    /**
     * Return a vertically mirrored version of the board.
     */
    Board mirrored() const;

    std::optional<Piece> get_piece(Square sq) const;
    std::optional<Colour> get_colour(Square sq) const;
    Colour get_turn() const;
    const std::vector<Move> &get_move_history() const;
    const std::array<bool, 4> get_castling_rights() const;
    std::string nice() const;
    std::string fen() const;
    bool is_insufficient_material() const;
    bool is_draw() const;
    bool is_repetition() const;
};

template <bool Captures, bool KingMoves, bool Checks, bool Promotions,
          bool Quiets>
bool Board::generate_moves(auto &&emitter) {
    constexpr bool GenerateAll =
        Captures && KingMoves && Checks && Promotions && Quiets;

    Colour opponent = turn.opposite();
    BitBoard us = get_bb(turn);
    BitBoard them = get_bb(opponent);
    BitBoard occupied = us | them;

    Square king_sq = get_bb(PP::KING, turn).lsb_square();
    Square enemy_king_sq = get_bb(PP::KING, opponent).lsb_square();
    BitBoard king_danger = attacked(opponent);
    BitBoard pinned_pieces = pinned(turn, king_sq);

    auto emit_move = [&](Move move) {
        if constexpr (std::is_void_v<decltype(emitter(move))>) {
            emitter(move);
            return true;
        } else {
            return static_cast<bool>(emitter(move));
        }
    };

    auto add_move = [&](Move move, bool selected) {
        return (!selected &&
                !(Checks && gives_check(move, occupied, enemy_king_sq))) ||
               emit_move(move);
    };

    auto add_moves = [&]<bool IsKing = false>(Square from, BitBoard targets) {
        while (targets.has_square()) {
            Square to = targets.get_square_pop();
            bool capture = them.has_square(to);
            Move move(from, to, Move::create_flags(capture));
            bool selected = (IsKing && KingMoves) || (capture && Captures) ||
                            (!capture && Quiets);
            if (!add_move(move, selected))
                return false;
        }
        return true;
    };

    BitBoard king_targets = Mask::kings(king_sq) & ~us & ~king_danger;
    if constexpr (!GenerateAll && !KingMoves && !Checks) {
        if constexpr (Captures && !Quiets)
            king_targets &= them;
        else if constexpr (!Captures && Quiets)
            king_targets &= ~them;
        else if constexpr (!Captures && !Quiets)
            king_targets = BitBoard();
    }

    if (!add_moves.template operator()<true>(king_sq, king_targets))
        return false;

    BitBoard checkers = attackers(opponent, king_sq);
    int num_checkers = checkers.count();

    // Double check: only king moves are legal.
    if (num_checkers >= 2)
        return true;

    BitBoard evasion_targets = check_evasion_targets(king_sq, checkers);
    BitBoard pinning_pieces = pinners(turn, king_sq);
    BitBoard active_own = num_checkers == 0 ? us : us & ~pinned_pieces;
    int mul = turn.weight();

    {
        BitBoard pawns = get_bb(PP::PAWN, turn) & active_own;
        while (pawns.has_square()) {
            Square from = pawns.get_square_pop();
            BitBoard allowed_targets =
                pinned_pieces.has_square(from)
                    ? pin_ray(king_sq, from, pinning_pieces)
                    : BitBoard::ALL_SQUARES;
            BitBoard target_mask = evasion_targets & allowed_targets;

            Square from_rel = from.flip(turn);

            // Push one
            Square push1_rel = from_rel + 8;
            Square push1_to = push1_rel.flip(turn);
            if (!occupied.has_square(push1_to) &&
                target_mask.has_square(push1_to)) {
                if (push1_rel.rank() == 7) { // Promotion rank
                    if constexpr (Promotions || Checks) {
                        for (auto promo : Move::PROMOTION_PIECES) {
                            if (!add_move(Move(from, push1_to, promo),
                                          Promotions))
                                return false;
                        }
                    }
                } else if constexpr (Quiets || Checks) {
                    if (!add_move(Move(from, push1_to, Move::QUIET), Quiets))
                        return false;
                }
            }

            // Push two
            if (from_rel.rank() == 1) { // Starting rank
                Square push2_rel = from_rel + 16;
                Square push2_to = push2_rel.flip(turn);
                if (!occupied.has_square(push1_to) &&
                    !occupied.has_square(push2_to) &&
                    target_mask.has_square(push2_to)) {
                    if constexpr (Quiets || Checks) {
                        if (!add_move(
                                Move(from, push2_to, Move::DOUBLE_PAWN_PUSH),
                                Quiets))
                            return false;
                    }
                }
            }

            BitBoard pawn = from;
            BitBoard captures = turn == CC::WHITE
                                    ? pawn.north().west() | pawn.north().east()
                                    : pawn.south().west() | pawn.south().east();
            captures &= them & target_mask;

            while (captures.has_square()) {
                Square to = captures.get_square_pop();
                if (from_rel.rank() == 6) {
                    if constexpr (Captures || Promotions || Checks) {
                        for (auto promo : Move::PROMOTION_CAPTURE_PIECES) {
                            if (!add_move(Move(from, to, promo),
                                          Captures || Promotions))
                                return false;
                        }
                    }
                } else if constexpr (Captures || Checks) {
                    if (!add_move(Move(from, to, Move::CAPTURE), Captures))
                        return false;
                }
            }

            // En passant
            if (ep_square) {
                Square ep = *ep_square;
                Square ep_rel = ep.flip(turn);
                Square captured_pawn_sq = ep - 8 * mul;

                bool can_ep = (from_rel.file() > 0 && ep_rel == from_rel + 7) ||
                              (from_rel.file() < 7 && ep_rel == from_rel + 9);
                bool allowed = evasion_targets.has_square(captured_pawn_sq) &&
                               allowed_targets.has_square(ep);

                if (can_ep && allowed) {
                    BitBoard occupied_after = occupied;
                    occupied_after.erase_square(from);
                    occupied_after.erase_square(captured_pawn_sq);
                    occupied_after.set_square(ep);

                    BitBoard discovered_attackers =
                        (Magic::rook_attacks(king_sq, occupied_after) &
                         (get_bb(PP::ROOK, opponent) |
                          get_bb(PP::QUEEN, opponent))) |
                        (Magic::bishop_attacks(king_sq, occupied_after) &
                         (get_bb(PP::BISHOP, opponent) |
                          get_bb(PP::QUEEN, opponent)));
                    if (!discovered_attackers.has_square() &&
                        (Captures || Checks) &&
                        !add_move(Move(from, ep, Move::EN_PASSANT), Captures))
                        return false;
                }
            }
        }
    }

    for (Piece piece = PP::KNIGHT; piece <= PP::QUEEN; piece++) {
        BitBoard pieces = get_bb(piece, turn) & active_own;
        while (pieces.has_square()) {
            Square from = pieces.get_square_pop();
            BitBoard targets = pinned_pieces.has_square(from)
                                   ? pin_ray(king_sq, from, pinning_pieces)
                                   : BitBoard::ALL_SQUARES;

            if (piece == PP::KNIGHT) {
                targets &= Mask::knights(from);
            } else if (piece == PP::BISHOP) {
                targets &= Magic::bishop_attacks(from, occupied);
            } else if (piece == PP::ROOK) {
                targets &= Magic::rook_attacks(from, occupied);
            } else {
                targets &= Magic::bishop_attacks(from, occupied) |
                           Magic::rook_attacks(from, occupied);
            }

            targets &= ~us & evasion_targets;
            if constexpr (!GenerateAll && !Checks) {
                if constexpr (Captures && !Quiets)
                    targets &= them;
                else if constexpr (!Captures && Quiets)
                    targets &= ~them;
                else if constexpr (!Captures && !Quiets)
                    targets = BitBoard();
            }
            if (!add_moves(from, targets))
                return false;
        }
    }

    if constexpr (KingMoves || Quiets || Checks) {
        if (num_checkers == 0) {
            auto SQE1 = SQ::E1.flip(turn), SQG1 = SQ::G1.flip(turn),
                 SQC1 = SQ::C1.flip(turn);
            auto BBF1 = BB::F1.flip(turn), BBG1 = BB::G1.flip(turn),
                 BBD1 = BB::D1.flip(turn), BBC1 = BB::C1.flip(turn),
                 BBB1 = BB::B1.flip(turn);

            // King-side castling
            if (can_castle[turn.raw() * 2] && occupied.empty(BBF1 | BBG1) &&
                king_danger.empty(BitBoard(SQE1) | BBF1 | BitBoard(SQG1)) &&
                !add_move(Move(SQE1, SQG1, Move::KING_SIDE_CASTLE),
                          KingMoves || Quiets))
                return false;

            // Queen-side castling
            if (can_castle[turn.raw() * 2 + 1] &&
                occupied.empty(BBB1 | BBC1 | BBD1) &&
                king_danger.empty(BitBoard(SQE1) | BBD1 | BitBoard(SQC1)) &&
                !add_move(Move(SQE1, SQC1, Move::QUEEN_SIDE_CASTLE),
                          KingMoves || Quiets))
                return false;
        }
    }

    return true;
}
