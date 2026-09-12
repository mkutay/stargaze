#include "stargaze/board.hpp"

#include "stargaze/bitboard.hpp"
#include "stargaze/colour.hpp"
#include "stargaze/eval.hpp"
#include "stargaze/mask.hpp"
#include "stargaze/move.hpp"
#include "stargaze/piece.hpp"
#include "stargaze/square.hpp"
#include "stargaze/zobrist.hpp"
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

void Board::make_move(Move move) {
    auto from = move.from();
    auto to = move.to();
    auto flags = move.flags();

    Piece moving_piece = *get_piece(from);
    std::optional<Piece> captured_piece = std::nullopt;
    if (move.is_capture()) {
        captured_piece = move.is_en_passant() ? PP::PAWN : *get_piece(to);
    }
    history.emplace_back(moving_piece, captured_piece, can_castle, ep_square,
                         halfmove_clock, current_hash);

    std::optional<Square> new_ep = std::nullopt;
    if (move.is_double_pawn_push()) {
        int mul = turn.weight();
        new_ep = std::optional<Square>{to - 8 * mul};
    }

    if (moving_piece == PP::PAWN || move.is_capture())
        halfmove_clock = 0;
    else
        halfmove_clock++;

    fullmove_number += turn.raw();

    auto ep_key = [](std::optional<Square> sq) -> uint64_t {
        return sq
            .transform(
                [](Square s) -> uint64_t { return Zobrist::en_passant(s); })
            .value_or(0ULL);
    };

    current_hash ^= ep_key(ep_square) ^ ep_key(new_ep);
    ep_square = new_ep;

    std::array<bool, 4> old_castle = can_castle;

    if (moving_piece == PP::KING) {
        can_castle[turn.raw() * 2] = can_castle[turn.raw() * 2 + 1] = false;
    }

    if (from == SQ::A1 || to == SQ::A1)
        can_castle[1] = false; // white queen-side
    if (from == SQ::H1 || to == SQ::H1)
        can_castle[0] = false; // white king-side
    if (from == SQ::A8 || to == SQ::A8)
        can_castle[3] = false; // black queen-side
    if (from == SQ::H8 || to == SQ::H8)
        can_castle[2] = false; // black king-side

    for (size_t i = 0; i < 4; ++i) {
        if (old_castle[i] != can_castle[i]) {
            current_hash ^= Zobrist::castling(i);
        }
    }

    Colour opponent = turn.opposite();

    switch (flags) {
    case Move::QUIET:
    case Move::DOUBLE_PAWN_PUSH:
        move_piece(moving_piece, turn, from, to);
        break;
    case Move::KING_SIDE_CASTLE:
        move_piece(PP::KING, turn, from, to);
        move_piece(PP::ROOK, turn, to + 1, to - 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        move_piece(PP::KING, turn, from, to);
        move_piece(PP::ROOK, turn, to - 2, to + 1);
        break;
    case Move::CAPTURE:
        clear_piece(*captured_piece, opponent, to);
        move_piece(moving_piece, turn, from, to);
        break;
    case Move::EN_PASSANT: {
        auto addition = turn.raw() * 16 - 8;
        Square captured_sq = to + addition;
        clear_piece(PP::PAWN, opponent, captured_sq);
        move_piece(PP::PAWN, turn, from, to);
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE:
        clear_piece(*captured_piece, opponent, to);
        clear_piece(PP::PAWN, turn, from);
        add_piece(move.promotion_piece(), turn, to);
        break;
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION:
        clear_piece(PP::PAWN, turn, from);
        add_piece(move.promotion_piece(), turn, to);
        break;
    }

    moves.emplace_back(move);
    current_hash ^= Zobrist::black_move();
    turn = opponent;

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::undo_move() {
    Move move = moves.back();
    moves.pop_back();

    UndoInfo undo_info = history.back();
    history.pop_back();

    auto from = move.from();
    auto to = move.to();
    auto flags = move.flags();

    turn = turn.opposite();
    Piece moving_piece = undo_info.moving_piece;
    std::optional<Piece> captured_piece = undo_info.captured_piece;
    ep_square = undo_info.ep_square;
    halfmove_clock = undo_info.halfmove_clock;
    fullmove_number -= turn.raw();
    can_castle = undo_info.can_castle;

    Colour opponent = turn.opposite();

    switch (flags) {
    case Move::QUIET:
    case Move::DOUBLE_PAWN_PUSH:
        move_piece(moving_piece, turn, to, from);
        break;
    case Move::KING_SIDE_CASTLE:
        move_piece(PP::KING, turn, to, from);
        move_piece(PP::ROOK, turn, to - 1, to + 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        move_piece(PP::KING, turn, to, from);
        move_piece(PP::ROOK, turn, to + 1, to - 2);
        break;
    case Move::CAPTURE:
        move_piece(moving_piece, turn, to, from);
        add_piece(*captured_piece, opponent, to);
        break;
    case Move::EN_PASSANT: {
        auto addition = turn.raw() * 16 - 8;
        Square captured_sq = to + addition;
        move_piece(PP::PAWN, turn, to, from);
        add_piece(PP::PAWN, opponent, captured_sq);
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE:
        clear_piece(move.promotion_piece(), turn, to);
        add_piece(*captured_piece, opponent, to);
        add_piece(PP::PAWN, turn, from);
        break;
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION:
        clear_piece(move.promotion_piece(), turn, to);
        add_piece(PP::PAWN, turn, from);
        break;
    }

    current_hash = undo_info.hash;

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::make_null_move() {
    history.emplace_back(PP::PAWN, std::nullopt, can_castle, ep_square,
                         halfmove_clock, current_hash);

    if (ep_square.has_value()) {
        current_hash ^= Zobrist::en_passant(ep_square.value());
    }

    current_hash ^= Zobrist::black_move();
    turn = turn.opposite();
    ep_square = std::nullopt;

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::undo_null_move() {
    UndoInfo undo_info = history.back();
    history.pop_back();

    turn = turn.opposite();
    can_castle = undo_info.can_castle;
    ep_square = undo_info.ep_square;
    halfmove_clock = undo_info.halfmove_clock;
    current_hash = undo_info.hash;

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

    assert(all_pieces == (colour_bbs[0] | colour_bbs[1]));
}

bool Board::is_insufficient_material() const {
    if (get_bb(PP::PAWN) == Mask::EMPTY && get_bb(PP::ROOK) == Mask::EMPTY &&
        get_bb(PP::QUEEN) == Mask::EMPTY) {
        const BitBoard bishops = get_bb(PP::BISHOP);
        const int minor_count = bishops.count() + get_bb(PP::KNIGHT).count();
        if (minor_count <= 1)
            return true;

        // With bishops alone, mate is impossible when every bishop is bound to
        // the same square colour.
        if (get_bb(PP::KNIGHT) == Mask::EMPTY &&
            ((bishops & Mask::LIGHT_SQUARES) == Mask::EMPTY) !=
                ((bishops & Mask::DARK_SQUARES) == Mask::EMPTY)) {
            return true;
        }
    }

    return false;
}

bool Board::is_draw() const {
    if (halfmove_clock >= 100 || is_insufficient_material())
        return true;

    int count = 0;
    int limit = history.size();
    int start = std::max(0, limit - halfmove_clock);
    for (int i = limit - 2; i >= start; i -= 2) {
        if (history[i].hash == current_hash) {
            if (++count >= 2)
                return true;
        }
    }
    return false;
}

bool Board::is_repetition() const {
    int limit = history.size();
    auto start = std::max(0, limit - halfmove_clock);
    for (int i = limit - 2; i >= start; i -= 2) {
        if (history[i].hash == current_hash) {
            return true;
        }
    }
    return false;
}

Colour Board::get_turn() const { return turn; }

uint8_t Board::get_halfmove_clock() const { return halfmove_clock; }

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
        if ((get_bb(type) & bb).has_square())
            return type;

    return std::nullopt;
}

std::optional<Colour> Board::get_colour(Square sq) const {
    BitBoard bb = sq;

    for (auto colour : COLOURS)
        if ((get_bb(colour) & bb).has_square())
            return colour;

    return std::nullopt;
}

void Board::move_piece(Piece piece, Colour colour, Square from, Square to) {
    add_piece(piece, colour, to);
    clear_piece(piece, colour, from);
}

void Board::add_piece(Piece piece, Colour colour, Square sq) {
    current_hash ^= Zobrist::piece(colour, piece, sq);

    mg_score[colour.raw()] += Eval::mg_value(colour, piece, sq);
    eg_score[colour.raw()] += Eval::eg_value(colour, piece, sq);
    game_phase += Eval::gamephase_inc(piece);

    BitBoard mask = BitBoard(sq);
    get_bb(piece) |= mask;
    get_bb(colour) |= mask;
}

void Board::clear_piece(Piece piece, Colour colour, Square sq) {
    current_hash ^= Zobrist::piece(colour, piece, sq);

    mg_score[colour.raw()] -= Eval::mg_value(colour, piece, sq);
    eg_score[colour.raw()] -= Eval::eg_value(colour, piece, sq);
    game_phase -= Eval::gamephase_inc(piece);

    BitBoard mask = ~BitBoard(sq);
    get_bb(piece) &= mask;
    get_bb(colour) &= mask;
}

const std::vector<Move> Board::get_move_history() const { return moves; }
const std::array<bool, 4> Board::get_castling_rights() const {
    return can_castle;
}

BitBoard &Board::get_bb(Piece type) { return piece_bbs[type.raw()]; }
BitBoard Board::get_bb(Piece type) const { return piece_bbs[type.raw()]; }
BitBoard &Board::get_bb(Colour colour) { return colour_bbs[colour.raw()]; }
BitBoard Board::get_bb(Colour colour) const { return colour_bbs[colour.raw()]; }
BitBoard Board::get_bb(Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour);
}

Board Board::mirrored() const {
    Board copy = *this;
    auto flip = CC::BLACK;

    copy.turn = turn.opposite();

    for (Piece p : PIECES) {
        copy.piece_bbs[p.raw()] = piece_bbs[p.raw()].flip(flip);
    }

    for (Colour c : COLOURS) {
        copy.colour_bbs[c.raw()] = colour_bbs[c.opposite().raw()].flip(flip);
    }

    copy.can_castle[0] = can_castle[2];
    copy.can_castle[1] = can_castle[3];
    copy.can_castle[2] = can_castle[0];
    copy.can_castle[3] = can_castle[1];

    if (ep_square) {
        copy.ep_square = ep_square->flip(flip);
    } else {
        copy.ep_square = std::nullopt;
    }

    copy.initialise_eval(copy.mg_score, copy.eg_score, copy.game_phase);
    copy.current_hash = copy.calculate_hash();

    return copy;
}
