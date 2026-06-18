#include "board.hpp"

#include "bitboard.hpp"
#include "enums.hpp"
#include "eval.hpp"
#include "mask.hpp"
#include "move.hpp"
#include "square.hpp"
#include "zobrist.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
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
      can_castle{true, true, true, true}, turn{Colour::WHITE},
      ep_square{std::nullopt}, halfmove_clock{0}, fullmove_number{1} {
    initialise_eval(mg_score, eg_score, game_phase);
    current_hash = calculate_hash();
    hash_history.push_back(current_hash);
}

Board::Board(std::string_view fen)
    : piece_bbs{BitBoard(0), BitBoard(0), BitBoard(0),
                BitBoard(0), BitBoard(0), BitBoard(0)},
      colour_bbs{BitBoard(0), BitBoard(0)} {
    can_castle.fill(false);
    moves.clear();
    history.clear();
    hash_history.clear();

    int r = 7, f = 0;
    size_t i = 0;
    for (; i < fen.size() && fen[i] != ' '; ++i) {
        char c = fen[i];
        if (c == '/') {
            r--;
            f = 0;
        } else if (std::isdigit(c)) {
            f += c - '0';
        } else {
            Colour colour = std::islower(c) ? Colour::BLACK : Colour::WHITE;
            Piece piece;
            switch (std::tolower(c)) {
            case 'p':
                piece = Piece::PAWN;
                break;
            case 'n':
                piece = Piece::KNIGHT;
                break;
            case 'b':
                piece = Piece::BISHOP;
                break;
            case 'r':
                piece = Piece::ROOK;
                break;
            case 'q':
                piece = Piece::QUEEN;
                break;
            case 'k':
                piece = Piece::KING;
                break;
            default:
                assert(false);
                piece = Piece::PAWN;
            }
            Square sq(r, f);
            get_bb(piece) |= BitBoard(sq);
            get_bb(colour) |= BitBoard(sq);
            f++;
        }
    }

    ++i;
    turn = fen[i] == 'w' ? Colour::WHITE : Colour::BLACK;

    i += 2;
    if (fen[i] != '-') {
        while (fen[i] != ' ') {
            switch (fen[i]) {
            case 'K':
                can_castle[0] = true;
                break;
            case 'Q':
                can_castle[1] = true;
                break;
            case 'k':
                can_castle[2] = true;
                break;
            case 'q':
                can_castle[3] = true;
                break;
            }
            i++;
        }
    } else {
        i++;
    }

    i++;
    if (fen[i] != '-') {
        ep_square = Square(std::string(fen.substr(i, 2)));
        i += 2;
    } else {
        ep_square = std::nullopt;
        i++;
    }

    if (i < fen.size()) {
        i++;
        size_t next_space = fen.find(' ', i);
        if (next_space != std::string_view::npos) {
            std::from_chars(fen.data() + i, fen.data() + next_space,
                            halfmove_clock);
            i = next_space + 1;
            std::from_chars(fen.data() + i, fen.data() + fen.size(),
                            fullmove_number);
        } else {
            std::from_chars(fen.data() + i, fen.data() + fen.size(),
                            halfmove_clock);
            fullmove_number = 1;
        }
    } else {
        halfmove_clock = 0;
        fullmove_number = 1;
    }

    initialise_eval(mg_score, eg_score, game_phase);
    current_hash = calculate_hash();
    hash_history.emplace_back(current_hash);
}

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
            captured_piece =
                move.is_en_passant() ? Piece::PAWN : *get_piece(to);
        }
        history.emplace_back(moving_piece, captured_piece, can_castle,
                             ep_square, halfmove_clock);
    }

    std::optional<Square> new_ep = std::nullopt;
    if constexpr (Undo) {
        new_ep = undo_info.ep_square;
    } else {
        int mul = weight(turn);
        if (move.is_double_pawn_push())
            new_ep = std::optional<Square>{to - 8 * mul};
    }

    if constexpr (Undo) {
        halfmove_clock = undo_info.halfmove_clock;

        // Decrement if white
        fullmove_number -= std::to_underlying(!turn);
    } else {
        if (moving_piece == Piece::PAWN || move.is_capture())
            halfmove_clock = 0;
        else
            halfmove_clock++;

        // Increment if black.
        fullmove_number += std::to_underlying(turn);
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
        move_piece(Piece::KING, turn, src, dst);
        move_piece(Piece::ROOK, turn, Undo ? to - 1 : to + 1,
                   Undo ? to + 1 : to - 1);
        break;
    case Move::QUEEN_SIDE_CASTLE:
        move_piece(Piece::KING, turn, src, dst);
        move_piece(Piece::ROOK, turn, Undo ? to + 1 : to - 2,
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
        auto turn_underlying = std::to_underlying(turn);
        auto addition = turn_underlying * 16 - 8;
        Square captured_sq = to + addition;
        if constexpr (!Undo) {
            clear_piece(Piece::PAWN, opponent, captured_sq);
        }
        move_piece(Piece::PAWN, turn, src, dst);
        if constexpr (Undo) {
            add_piece(Piece::PAWN, opponent, captured_sq);
        }
        break;
    }
    case Move::KNIGHT_PROMOTION_CAPTURE:
    case Move::BISHOP_PROMOTION_CAPTURE:
    case Move::ROOK_PROMOTION_CAPTURE:
    case Move::QUEEN_PROMOTION_CAPTURE:
        if constexpr (!Undo) {
            clear_piece(*captured_piece, opponent, to);
            clear_piece(Piece::PAWN, turn, from);
            add_piece(move.promotion_piece(), turn, to);
        } else {
            clear_piece(move.promotion_piece(), turn, to);
            add_piece(*captured_piece, opponent, to);
            add_piece(Piece::PAWN, turn, from);
        }
        break;
    case Move::KNIGHT_PROMOTION:
    case Move::BISHOP_PROMOTION:
    case Move::ROOK_PROMOTION:
    case Move::QUEEN_PROMOTION:
        if constexpr (!Undo) {
            clear_piece(Piece::PAWN, turn, from);
            add_piece(move.promotion_piece(), turn, to);
        } else {
            clear_piece(move.promotion_piece(), turn, to);
            add_piece(Piece::PAWN, turn, from);
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
    history.emplace_back(Piece::PAWN, std::nullopt, can_castle, ep_square,
                         halfmove_clock);

    if (ep_square.has_value()) {
        current_hash ^= Zobrist::en_passant(ep_square.value());
    }

    current_hash ^= Zobrist::black_move();
    turn = !turn;
    ep_square = std::nullopt;
    halfmove_clock++;

    if (turn == Colour::WHITE) {
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

    if (turn == Colour::BLACK) {
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
    return (get_bb(colour) & ~get_bb(Piece::PAWN) & ~get_bb(Piece::KING)) !=
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

    current_hash ^= Zobrist::piece(colour, piece, from);
    current_hash ^= Zobrist::piece(colour, piece, to);

    mg_score[ci] -= Eval::mg_value(colour, piece, from);
    eg_score[ci] -= Eval::eg_value(colour, piece, from);
    mg_score[ci] += Eval::mg_value(colour, piece, to);
    eg_score[ci] += Eval::eg_value(colour, piece, to);

    BitBoard mask = BitBoard(from) | BitBoard(to);
    get_bb(piece) ^= mask;
    get_bb(colour) ^= mask;
}

void Board::add_piece(Piece piece, Colour colour, Square sq) {
    auto ci = std::to_underlying(colour);

    current_hash ^= Zobrist::piece(colour, piece, sq);

    mg_score[ci] += Eval::mg_value(colour, piece, sq);
    eg_score[ci] += Eval::eg_value(colour, piece, sq);
    game_phase += Eval::gamephase_inc(piece);

    BitBoard mask = BitBoard(sq);
    get_bb(piece) |= mask;
    get_bb(colour) |= mask;
}

void Board::clear_piece(Piece piece, Colour colour, Square sq) {
    auto ci = std::to_underlying(colour);

    current_hash ^= Zobrist::piece(colour, piece, sq);

    mg_score[ci] -= Eval::mg_value(colour, piece, sq);
    eg_score[ci] -= Eval::eg_value(colour, piece, sq);
    game_phase -= Eval::gamephase_inc(piece);

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
