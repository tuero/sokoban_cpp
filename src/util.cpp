#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "definitions.h"

namespace sokoban {
namespace util {

Board parse_board_str(const std::string &board_str) {
    std::stringstream board_ss(board_str);
    std::string segment;
    std::vector<std::string> seglist;
    // string split on |
    while (std::getline(board_ss, segment, '|')) {
        seglist.push_back(segment);
    }

    assert(seglist.size() >= 2);

    // Get general info
    int rows = std::stoi(seglist[0]);
    int cols = std::stoi(seglist[1]);
    assert((int)seglist.size() == rows * cols + 2);
    Board board(rows, cols);

    // Parse grid
    int stone_counter = 0;
    for (std::size_t i = 2; i < seglist.size(); ++i) {
        int el_idx = std::stoi(seglist[i]);
        // Skip special empty indicator
        if (el_idx == kNumElementTypes) {
            continue;
        }
        if (el_idx < 0 || el_idx >= kNumElementTypes) {
            std::cerr << "Unknown element type: " << el_idx << std::endl;
            exit(1);
        }

        if (static_cast<ElementTypes>(el_idx) == ElementTypes::kAgent) {
            board.agent_idx = i - 2;
        }
        board.item(el_idx, i - 2) = static_cast<ElementTypes>(el_idx) == ElementTypes::kBox ? ++stone_counter : 1;
    }

    return board;
}

}    // namespace util
}    // namespace sokoban
