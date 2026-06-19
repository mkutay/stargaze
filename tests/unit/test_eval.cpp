#include "board.hpp"
#include "doctest/doctest.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string mirror_fen(const std::string &fen) {
    std::vector<std::string> tokens;
    std::istringstream iss(fen);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 6)
        return fen;

    // 1. Mirror piece placement
    std::vector<std::string> ranks;
    std::istringstream p_iss(tokens[0]);
    std::string rank;
    while (std::getline(p_iss, rank, '/')) {
        ranks.push_back(rank);
    }
    std::reverse(ranks.begin(), ranks.end());
    for (auto &r : ranks) {
        for (char &c : r) {
            if (std::isupper(c))
                c = std::tolower(c);
            else if (std::islower(c))
                c = std::toupper(c);
        }
    }
    std::string mirrored_pieces = ranks[0];
    for (size_t i = 1; i < ranks.size(); ++i) {
        mirrored_pieces += "/" + ranks[i];
    }

    // 2. Active color
    std::string mirrored_color = (tokens[1] == "w") ? "b" : "w";

    // 3. Castling rights
    std::string mirrored_castling = "";
    if (tokens[2] != "-") {
        std::string w_castling = "";
        std::string b_castling = "";
        for (char c : tokens[2]) {
            if (c == 'K')
                b_castling += 'k';
            else if (c == 'Q')
                b_castling += 'q';
            else if (c == 'k')
                w_castling += 'K';
            else if (c == 'q')
                w_castling += 'Q';
        }
        mirrored_castling = w_castling + b_castling;
        if (mirrored_castling.empty())
            mirrored_castling = "-";
        std::string sorted_castling = "";
        if (mirrored_castling.find('K') != std::string::npos)
            sorted_castling += 'K';
        if (mirrored_castling.find('Q') != std::string::npos)
            sorted_castling += 'Q';
        if (mirrored_castling.find('k') != std::string::npos)
            sorted_castling += 'k';
        if (mirrored_castling.find('q') != std::string::npos)
            sorted_castling += 'q';
        mirrored_castling = sorted_castling;
    } else {
        mirrored_castling = "-";
    }

    // 4. EP square
    std::string mirrored_ep = "-";
    if (tokens[3] != "-") {
        char file = tokens[3][0];
        char r_char = tokens[3][1];
        char mirrored_rank = '1' + ('8' - r_char);
        mirrored_ep = std::string(1, file) + mirrored_rank;
    }

    // 5. Clocks
    std::string halfmove = tokens[4];
    std::string fullmove = tokens[5];

    return mirrored_pieces + " " + mirrored_color + " " + mirrored_castling +
           " " + mirrored_ep + " " + halfmove + " " + fullmove;
}
} // namespace

TEST_SUITE("unit") {
    TEST_CASE("Evaluation symmetry on mirrored positions") {
        // clang-format off
        const std::string fens[] = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b Kq e3 0 1",
            // Asymmetric pawns and pieces
            "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d3 0 2",
            "k7/8/8/8/8/8/8/7K b - - 15 40"
        };
        // clang-format on

        for (const auto &fen : fens) {
            Board board(fen);
            std::string mirrored = mirror_fen(fen);
            Board mirrored_board(mirrored);

            // The evaluation score should be symmetric
            CHECK(board.evaluate() == mirrored_board.evaluate());
        }
    }
}
