#include "board.hpp"
#include <charconv>
#include <format>

Board::Board()
    : Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

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
            Colour colour = std::islower(c) ? CC::BLACK : CC::WHITE;
            Piece piece{c};
            Square sq(r, f);
            get_bb(piece) |= BitBoard(sq);
            get_bb(colour) |= BitBoard(sq);
            f++;
        }
    }

    i++;
    turn = fen[i] == 'w' ? CC::WHITE : CC::BLACK;

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

std::string Board::fen() const {
    std::string fen_str;

    for (int r = 7; r >= 0; --r) {
        int empty_count = 0;
        for (int f = 0; f < 8; ++f) {
            Square sq(r, f);
            if (auto piece = get_piece(sq)) {
                if (empty_count > 0) {
                    fen_str += std::to_string(empty_count);
                    empty_count = 0;
                }
                auto colour = get_colour(sq);
                char p_char = piece->to_string()[0];
                if (colour == CC::WHITE) {
                    p_char = std::toupper(p_char);
                }
                fen_str += p_char;
            } else {
                empty_count++;
            }
        }
        if (empty_count > 0) {
            fen_str += std::to_string(empty_count);
        }
        if (r > 0) {
            fen_str += "/";
        }
    }

    fen_str += std::format(" {}", turn == CC::WHITE ? "w" : "b");

    // castling rights
    fen_str += " ";
    std::string castling;
    if (can_castle[0])
        castling += 'K';
    if (can_castle[1])
        castling += 'Q';
    if (can_castle[2])
        castling += 'k';
    if (can_castle[3])
        castling += 'q';
    if (castling.empty())
        castling = "-";
    fen_str += castling;

    // ep target square
    fen_str += std::format(" {}", ep_square ? ep_square->to_string() : "-");

    fen_str += std::format(" {}", halfmove_clock);
    fen_str += std::format(" {}", fullmove_number);

    return fen_str;
}
