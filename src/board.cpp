#include "board.hpp"

#include "bitboard.hpp"
#include "colour.hpp"
#include "eval.hpp"
#include "mask.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "square.hpp"
#include "zobrist.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <vector>

uint64_t Board::perft(int depth) {
    if (depth == 0)
        return 1;

    uint64_t nodes = 0;
    auto legal_moves = get_moves();

    for (Move move : legal_moves) {
        make_move(move);
        nodes += perft(depth - 1);
        undo_move();
    }

    return nodes;
}

template <bool Undo> void Board::apply_move(Move move) {
    auto from = move.from();
    auto to = move.to();
    auto flags = move.flags();

    UndoInfo undo_info;
    if constexpr (Undo) {
        undo_info = history.back();
        history.pop_back();
        moves.pop_back();
        hash_history.pop_back();

        turn = !turn;
        current_hash ^= Zobrist::black_move();
    }

    Piece moving_piece;
    std::optional<Piece> captured_piece = std::nullopt;
    if constexpr (Undo) {
        moving_piece = undo_info.moving_piece;
        captured_piece = undo_info.captured_piece;
    } else {
        moving_piece = *get_piece(from);
        if (move.is_capture()) {
            captured_piece = move.is_en_passant() ? PP::PAWN : *get_piece(to);
        }
        history.emplace_back(moving_piece, captured_piece, can_castle,
                             ep_square, halfmove_clock);
    }

    std::optional<Square> new_ep = std::nullopt;
    if constexpr (Undo) {
        new_ep = undo_info.ep_square;
    } else {
        int mul = turn.weight();
        if (move.is_double_pawn_push())
            new_ep = std::optional<Square>{to - 8 * mul};
    }

    if constexpr (Undo) {
        halfmove_clock = undo_info.halfmove_clock;

        // Decrement if white
        fullmove_number -= !turn;
    } else {
        if (moving_piece == PP::PAWN || move.is_capture())
            halfmove_clock = 0;
        else
            halfmove_clock++;

        // Increment if black.
        fullmove_number += turn;
    }

    auto ep_key = [](std::optional<Square> sq) -> uint64_t {
        return sq
            .transform(
                [](Square s) -> uint64_t { return Zobrist::en_passant(s); })
            .value_or(0ULL);
    };

    // XOR out the old EP file key, update ep_square, then XOR in the new key.
    current_hash ^= ep_key(ep_square) ^ ep_key(new_ep);
    ep_square = new_ep;

    std::array<bool, 4> old_castle = can_castle;

    if constexpr (Undo) {
        can_castle = undo_info.can_castle;
    } else {
        if (moving_piece == PP::KING) {
            can_castle[turn * 2] = can_castle[turn * 2 + 1] = false;
        }

        if (from == SQ::A1 || to == SQ::A1)
            can_castle[1] = false; // white queen-side
        if (from == SQ::H1 || to == SQ::H1)
            can_castle[0] = false; // white king-side
        if (from == SQ::A8 || to == SQ::A8)
            can_castle[3] = false; // black queen-side
        if (from == SQ::H8 || to == SQ::H8)
            can_castle[2] = false; // black king-side
    }

    for (size_t i = 0; i < 4; ++i) {
        if (old_castle[i] != can_castle[i]) {
            current_hash ^= Zobrist::castling(i);
        }
    }

    Colour opponent = !turn;
    Square src = Undo ? to : from;
    Square dst = Undo ? from : to;

    switch (flags) {
    case Move::QUIET:
    case Move::DOUBLE_PAWN_PUSH:
        move_piece(moving_piece, turn, src, dst);
        break;
    case Move::KING_SIDE_CASTLE:
        move_piece(PP::KING, turn, src, dst);
        move_piece(PP::ROOK, turn, Undo ? to - 1 : to + 1,
                   Undo ? to + 1 : to - 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        move_piece(PP::KING, turn, src, dst);
        move_piece(PP::ROOK, turn, Undo ? to + 1 : to - 2,
                   Undo ? to - 2 : to + 1);
        break;
    case Move::CAPTURE:
        if constexpr (!Undo) {
            clear_piece(*captured_piece, opponent, to);
        }
        move_piece(moving_piece, turn, src, dst);
        if constexpr (Undo) {
            add_piece(*captured_piece, opponent, to);
        }
        break;
    case Move::EN_PASSANT: {
        auto addition = turn * 16 - 8;
        Square captured_sq = to + addition;
        if constexpr (!Undo) {
            clear_piece(PP::PAWN, opponent, captured_sq);
        }
        move_piece(PP::PAWN, turn, src, dst);
        if constexpr (Undo) {
            add_piece(PP::PAWN, opponent, captured_sq);
        }
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE:
        if constexpr (!Undo) {
            clear_piece(*captured_piece, opponent, to);
            clear_piece(PP::PAWN, turn, from);
            add_piece(move.promotion_piece(), turn, to);
        } else {
            clear_piece(move.promotion_piece(), turn, to);
            add_piece(*captured_piece, opponent, to);
            add_piece(PP::PAWN, turn, from);
        }
        break;
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION:
        if constexpr (!Undo) {
            clear_piece(PP::PAWN, turn, from);
            add_piece(move.promotion_piece(), turn, to);
        } else {
            clear_piece(move.promotion_piece(), turn, to);
            add_piece(PP::PAWN, turn, from);
        }
        break;
    }

    if constexpr (!Undo) {
        moves.emplace_back(move);
        current_hash ^= Zobrist::black_move();
        turn = opponent;
        hash_history.emplace_back(current_hash);
    }

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::make_move(Move move) { apply_move<false>(move); }

void Board::undo_move() { apply_move<true>(moves.back()); }

void Board::make_null_move() {
    history.emplace_back(PP::PAWN, std::nullopt, can_castle, ep_square,
                         halfmove_clock);

    if (ep_square.has_value()) {
        current_hash ^= Zobrist::en_passant(ep_square.value());
    }

    current_hash ^= Zobrist::black_move();
    turn = !turn;
    ep_square = std::nullopt;
    halfmove_clock++;

    if (turn == CC::WHITE) {
        fullmove_number++;
    }

    hash_history.emplace_back(current_hash);

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::undo_null_move() {
    UndoInfo undo_info = history.back();
    history.pop_back();
    hash_history.pop_back();

    turn = !turn;
    current_hash ^= Zobrist::black_move();

    if (turn == CC::BLACK) {
        fullmove_number--;
    }

    can_castle = undo_info.can_castle;
    ep_square = undo_info.ep_square;
    halfmove_clock = undo_info.halfmove_clock;
    current_hash = hash_history.back();

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

bool Board::has_non_pawn_material(Colour colour) const {
    return (get_bb(colour) & ~get_bb(PP::PAWN) & ~get_bb(PP::KING)) !=
           Mask::EMPTY;
}

void Board::check_state_consistency() const {
    assert(current_hash == calculate_hash());

    std::array<int, 2> _mg_score, _eg_score;
    int _game_phase;
    initialise_eval(_mg_score, _eg_score, _game_phase);

    assert(mg_score == _mg_score);
    assert(eg_score == _eg_score);
    assert(game_phase == _game_phase);

    // pairwise disjoint
    for (int i = 0; i < 6; i++)
        for (int j = i + 1; j < 6; j++)
            assert((piece_bbs[i] & piece_bbs[j]) == Mask::EMPTY);

    assert((colour_bbs[0] & colour_bbs[1]) == Mask::EMPTY);

    BitBoard all_pieces = Mask::EMPTY;
    for (auto bb : piece_bbs)
        all_pieces |= bb;
    assert(all_pieces == colour_bbs[0] | colour_bbs[1]);
}

bool Board::is_draw() const {
    if (halfmove_clock >= 100)
        return true;

    int count = 0;
    int limit = hash_history.size() - 1;
    int start = std::max(0, limit - halfmove_clock);
    for (int i = limit - 2; i >= start; i -= 2) {
        if (hash_history[i] == current_hash) {
            if (++count >= 2)
                return true;
        }
    }
    return false;
}

Colour Board::get_turn() const { return turn; }

std::string Board::nice() const {
    std::vector<std::string> result;
    std::string temp = "";
    for (int i = 0; i < 64; i++) {
        if (i != 0 && i % 8 == 0)
            result.emplace_back(temp), temp = "";

        auto piece = get_piece(Square(i));
        auto colour = get_colour(Square(i));
        if (piece && colour) {
            temp += (*piece).nice(*colour) + " ";
        } else {
            temp += ". ";
        }
    }
    result.emplace_back(temp);
    temp = "";
    reverse(result.begin(), result.end());
    for (std::string i : result)
        temp += i + "\n";
    return temp;
}

std::optional<Piece> Board::get_piece(Square sq) const {
    BitBoard bb = sq;

    for (auto type : PIECES)
        if (get_bb(type) & bb)
            return type;

    return std::nullopt;
}

std::optional<Colour> Board::get_colour(Square sq) const {
    BitBoard bb = sq;

    for (auto colour : COLOURS)
        if (get_bb(colour) & bb)
            return colour;

    return std::nullopt;
}

bool Board::has_piece_at(BitBoard bb, Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour) & bb;
}

void Board::move_piece(Piece piece, Colour colour, Square from, Square to) {
    current_hash ^= Zobrist::piece(colour, piece, from);
    current_hash ^= Zobrist::piece(colour, piece, to);

    mg_score[colour] -= Eval::mg_value(colour, piece, from);
    eg_score[colour] -= Eval::eg_value(colour, piece, from);
    mg_score[colour] += Eval::mg_value(colour, piece, to);
    eg_score[colour] += Eval::eg_value(colour, piece, to);

    BitBoard mask = BitBoard(from) | BitBoard(to);
    get_bb(piece) ^= mask;
    get_bb(colour) ^= mask;
}

void Board::add_piece(Piece piece, Colour colour, Square sq) {
    current_hash ^= Zobrist::piece(colour, piece, sq);

    mg_score[colour] += Eval::mg_value(colour, piece, sq);
    eg_score[colour] += Eval::eg_value(colour, piece, sq);
    game_phase += Eval::gamephase_inc(piece);

    BitBoard mask = BitBoard(sq);
    get_bb(piece) |= mask;
    get_bb(colour) |= mask;
}

void Board::clear_piece(Piece piece, Colour colour, Square sq) {
    current_hash ^= Zobrist::piece(colour, piece, sq);

    mg_score[colour] -= Eval::mg_value(colour, piece, sq);
    eg_score[colour] -= Eval::eg_value(colour, piece, sq);
    game_phase -= Eval::gamephase_inc(piece);

    BitBoard mask = ~BitBoard(sq);
    get_bb(piece) &= mask;
    get_bb(colour) &= mask;
}

const std::vector<Move> Board::get_move_history() const { return moves; }
const std::array<bool, 4> Board::get_castling_rights() const {
    return can_castle;
}

BitBoard &Board::get_bb(Piece type) { return piece_bbs[type]; }
BitBoard Board::get_bb(Piece type) const { return piece_bbs[type]; }
BitBoard &Board::get_bb(Colour colour) { return colour_bbs[colour]; }
BitBoard Board::get_bb(Colour colour) const { return colour_bbs[colour]; }
BitBoard Board::get_bb(Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour);
}
