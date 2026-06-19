#include "board.hpp"
#include "doctest/doctest.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

TEST_SUITE("integration") {
    TEST_CASE("Perft suite validation") {
        std::ifstream file("tests/data/perft_suite.epd");
        REQUIRE_MESSAGE(file.is_open(),
                        "Could not open tests/data/perft_suite.epd");

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;

            std::istringstream iss(line);
            std::string fen;
            std::getline(iss, fen, ';');

            Board board(fen);

            std::string depth_res;
            while (std::getline(iss, depth_res, ';')) {
                if (depth_res.empty())
                    continue;
                std::istringstream dr_iss(depth_res);
                std::string depth_str;
                uint64_t expected_nodes = 0;
                dr_iss >> depth_str >> expected_nodes;

                if (depth_str.size() > 1 && depth_str[0] == 'D') {
                    int depth = std::stoi(depth_str.substr(1));
                    uint64_t actual_nodes = board.perft(depth);
                    CHECK_MESSAGE(actual_nodes == expected_nodes,
                                  "FEN: " << fen << " at depth " << depth
                                          << " expected " << expected_nodes
                                          << " but got " << actual_nodes);
                }
            }
        }
    }
}
