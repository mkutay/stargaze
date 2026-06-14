#include "board.hpp"

#include "bitboard.hpp"
#include "enums.hpp"
#include "eval_tables.hpp"
#include "mask.hpp"
#include "move.hpp"
#include "square.hpp"
#include "zobrist_keys.hpp"
#include <algorithm>
#include <array>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>

Board::Board()
    : piece_bbs{
          BB::A2 | BB::B2 | BB::C2 | BB::D2 | BB::E2 | BB::F2 | BB::G2 |
              BB::H2 | BB::A7 | BB::B7 | BB::C7 | BB::D7 | BB::E7 | BB::F7 |
              BB::G7 | BB::H7,               // PAWN
          BB::B1 | BB::G1 | BB::B8 | BB::G8, // KNIGHT
          BB::C1 | BB::F1 | BB::C8 | BB::F8, // BISHOP
          BB::A1 | BB::H1 | BB::A8 | BB::H8, // ROOK
          BB::D1 | BB::D8,                   // QUEEN
          BB::E1 | BB::E8,                   // KING
      },
      colour_bbs{
          BB::A1 | BB::B1 | BB::C1 | BB::D1 | BB::E1 | BB::F1 | BB::G1 |
              BB::H1 | BB::A2 | BB::B2 | BB::C2 | BB::D2 | BB::E2 | BB::F2 |
              BB::G2 | BB::H2, // WHITE
          BB::A8 | BB::B8 | BB::C8 | BB::D8 | BB::E8 | BB::F8 | BB::G8 |
              BB::H8 | BB::A7 | BB::B7 | BB::C7 | BB::D7 | BB::E7 | BB::F7 |
              BB::G7 | BB::H7, // BLACK
      },
      can_castle{true, true, true, true}, turn{Colour::WHITE} {
    initialise_eval(mg_score, eg_score, game_phase);
    current_hash = calculate_hash();
}

/**
 * Make a move on the board, updating the board state accordingly. This includes
 * updating the pieces, castling rights, and move history.
 *
 * Note that we assume the move is valid and legal. That is, we don't check if
 * the move is actually possible, such as moving a piece that isn't there, or
 * moving to a square occupied by your own piece, or moving into check. We also
 * don't check if the move is legal in terms of the rules of chess, such as
 * castling through check or en passant when not possible.
 */
void Board::make_move(Move move) {
    auto from = move.from();
    auto to = move.to();
    auto flags = move.flags();

    Piece moving_piece = *get_piece(from);
    std::optional<Piece> captured_piece = std::nullopt;
    if (move.is_capture()) {
        captured_piece = move.is_en_passant() ? Piece::PAWN : *get_piece(to);
    }
    history.emplace_back(moving_piece, captured_piece, can_castle);

    if (!moves.empty() && moves.back().is_double_pawn_push()) {
        current_hash ^= Zobrist::en_passant_file[moves.back().to() & 7];
    }

    std::array<bool, 4> old_castle = can_castle;

    if (moving_piece == Piece::KING) {
        auto turn_index = std::to_underlying(turn);
        can_castle[turn_index * 2] = can_castle[turn_index * 2 + 1] = false;
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
            current_hash ^= Zobrist::castling[i];
        }
    }

    Colour opponent = !turn;
    switch (flags) {
    case Move::QUIET:
    case Move::DOUBLE_PAWN_PUSH:
        move_piece(moving_piece, turn, from, to);
        break;
    case Move::KING_SIDE_CASTLE:
        move_piece(Piece::KING, turn, from, to);
        move_piece(Piece::ROOK, turn, to + 1, to - 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        move_piece(Piece::KING, turn, from, to);
        move_piece(Piece::ROOK, turn, to - 2, to + 1);
        break;
    case Move::CAPTURE:
        clear_piece(*captured_piece, opponent, to);
        move_piece(moving_piece, turn, from, to);
        break;
    case Move::EN_PASSANT: {
        auto turn_underlying = std::to_underlying(turn);
        auto addition = turn_underlying * 16 - 8;
        Square captured_sq = to + addition;
        clear_piece(Piece::PAWN, opponent, captured_sq);
        move_piece(Piece::PAWN, turn, from, to);
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE:
        clear_piece(*captured_piece, opponent, to);
        [[fallthrough]];
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION:
        clear_piece(Piece::PAWN, turn, from);
        add_piece(move.promotion_piece(), turn, to);
        break;
    }

    moves.emplace_back(move);
    current_hash ^= Zobrist::black_move;
    turn = opponent;

    if (move.is_double_pawn_push()) {
        current_hash ^= Zobrist::en_passant_file[to & 7];
    }

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::undo_move() {
    auto undo_info = history.back();
    history.pop_back();

    Move move = moves.back();
    moves.pop_back();

    auto from = move.from();
    auto to = move.to();
    auto flags = move.flags();

    turn = !turn;
    current_hash ^= Zobrist::black_move;

    if (move.is_double_pawn_push()) {
        current_hash ^= Zobrist::en_passant_file[to & 7];
    }

    if (!moves.empty() && moves.back().is_double_pawn_push()) {
        current_hash ^= Zobrist::en_passant_file[moves.back().to() & 7];
    }

    for (size_t i = 0; i < 4; ++i) {
        if (can_castle[i] != undo_info.can_castle[i]) {
            current_hash ^= Zobrist::castling[i];
        }
    }
    can_castle = undo_info.can_castle;

    Piece moving_piece = undo_info.moving_piece;
    Colour opponent = !turn;

    switch (flags) {
    case Move::QUIET:
    case Move::DOUBLE_PAWN_PUSH:
        move_piece(moving_piece, turn, to, from);
        break;
    case Move::KING_SIDE_CASTLE:
        move_piece(Piece::KING, turn, to, from);
        move_piece(Piece::ROOK, turn, to - 1, to + 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        move_piece(Piece::KING, turn, to, from);
        move_piece(Piece::ROOK, turn, to + 1, to - 2);
        break;
    case Move::CAPTURE:
        move_piece(moving_piece, turn, to, from);
        add_piece(*undo_info.captured_piece, opponent, to);
        break;
    case Move::EN_PASSANT: {
        move_piece(Piece::PAWN, turn, to, from);
        auto turn_underlying = std::to_underlying(turn);
        auto addition = turn_underlying * 16 - 8;
        Square captured_sq = to + addition;
        add_piece(Piece::PAWN, opponent, captured_sq);
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE:
        clear_piece(move.promotion_piece(), turn, to);
        add_piece(*undo_info.captured_piece, opponent, to);
        add_piece(Piece::PAWN, turn, from);
        break;
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION:
        clear_piece(move.promotion_piece(), turn, to);
        add_piece(Piece::PAWN, turn, from);
        break;
    }

#ifdef VERIFY_CONSISTENCY
    check_state_consistency();
#endif
}

void Board::check_state_consistency() const {
    uint64_t calculated = calculate_hash();
    assert(current_hash == calculated);

    std::array<int, 2> _mg_score, _eg_score;
    int _game_phase;
    initialise_eval(_mg_score, _eg_score, _game_phase);

    assert(mg_score == _mg_score);
    assert(eg_score == _eg_score);
    assert(game_phase == _game_phase);

    // Piece bitboards must be pairwise disjoint.
    for (int i = 0; i < 6; i++)
        for (int j = i + 1; j < 6; j++)
            assert((piece_bbs[i] & piece_bbs[j]) == Mask::EMPTY);

    // Colour bitboards must be disjoint.
    assert((colour_bbs[0] & colour_bbs[1]) == Mask::EMPTY);

    // Union of piece bitboards must equal union of colour bitboards.
    BitBoard all_pieces = Mask::EMPTY;
    for (auto bb : piece_bbs)
        all_pieces |= bb;
    BitBoard all_colours = colour_bbs[0] | colour_bbs[1];
    assert(all_pieces == all_colours);
}

Colour Board::get_turn() const { return turn; }

std::string Board::to_string() const {
    const static std::unordered_map<Piece, std::array<std::string, 2>>
        piece_string = {
            {Piece::PAWN, {"♟", "♙"}},   {Piece::KNIGHT, {"♞", "♘"}},
            {Piece::BISHOP, {"♝", "♗"}}, {Piece::ROOK, {"♜", "♖"}},
            {Piece::QUEEN, {"♛", "♕"}},  {Piece::KING, {"♚", "♔"}},
        };

    std::vector<std::string> result;
    std::string temp = "";
    for (int i = 0; i < 64; i++) {
        if (i != 0 && i % 8 == 0)
            result.emplace_back(temp), temp = "";

        auto piece = get_piece(Square(i));
        auto colour = get_colour(Square(i));
        if (piece && colour) {
            temp +=
                piece_string.at(*piece).at(std::to_underlying(*colour)) + " ";
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
    auto ci = std::to_underlying(colour);
    auto pi = std::to_underlying(piece);

    current_hash ^= Zobrist::hash[ci][pi][from];
    current_hash ^= Zobrist::hash[ci][pi][to];

    mg_score[ci] -= Eval::mg_table[ci][pi][from];
    eg_score[ci] -= Eval::eg_table[ci][pi][from];
    mg_score[ci] += Eval::mg_table[ci][pi][to];
    eg_score[ci] += Eval::eg_table[ci][pi][to];

    BitBoard mask = BitBoard(from) | BitBoard(to);
    get_bb(piece) ^= mask;
    get_bb(colour) ^= mask;
}

void Board::add_piece(Piece piece, Colour colour, Square sq) {
    auto ci = std::to_underlying(colour);
    auto pi = std::to_underlying(piece);

    current_hash ^= Zobrist::hash[ci][pi][sq];

    mg_score[ci] += Eval::mg_table[ci][pi][sq];
    eg_score[ci] += Eval::eg_table[ci][pi][sq];
    game_phase += Eval::gamephase_inc[pi];

    BitBoard mask = BitBoard(sq);
    get_bb(piece) |= mask;
    get_bb(colour) |= mask;
}

void Board::clear_piece(Piece piece, Colour colour, Square sq) {
    auto ci = std::to_underlying(colour);
    auto pi = std::to_underlying(piece);

    current_hash ^= Zobrist::hash[ci][pi][sq];

    mg_score[ci] -= Eval::mg_table[ci][pi][sq];
    eg_score[ci] -= Eval::eg_table[ci][pi][sq];
    game_phase -= Eval::gamephase_inc[pi];

    BitBoard mask = ~BitBoard(sq);
    get_bb(piece) &= mask;
    get_bb(colour) &= mask;
}

const std::vector<Move> Board::get_move_history() const { return moves; }
const std::array<bool, 4> Board::get_castling_rights() const {
    return can_castle;
}

BitBoard &Board::get_bb(Piece type) {
    return piece_bbs[std::to_underlying(type)];
}

BitBoard Board::get_bb(Piece type) const {
    return piece_bbs[std::to_underlying(type)];
}

BitBoard &Board::get_bb(Colour colour) {
    return colour_bbs[std::to_underlying(colour)];
}

BitBoard Board::get_bb(Colour colour) const {
    return colour_bbs[std::to_underlying(colour)];
}

BitBoard Board::get_bb(Piece type, Colour colour) const {
    return get_bb(type) & get_bb(colour);
}
