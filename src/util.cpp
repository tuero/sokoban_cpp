#include "util.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace sokoban {

void parse_board_str(const std::string &board_str, LocalState &local_state,
                     std::shared_ptr<SharedStateInfo> shared_state) {
    std::stringstream board_ss(board_str);
    std::string segment;
    std::vector<std::string> seglist;
    // string split on |
    while (std::getline(board_ss, segment, '|')) {
        seglist.push_back(segment);
    }

    assert(seglist.size() >= 2);

    // Get general info
    const int rows = std::stoi(seglist[0]);
    const int cols = std::stoi(seglist[1]);
    assert(static_cast<int>(seglist.size()) == rows * cols + 2);
    shared_state->rows = static_cast<std::size_t>(rows);
    shared_state->cols = static_cast<std::size_t>(cols);

    // Parse grid
    int agent_counter = 0;
    int box_counter = 0;
    int goal_counter = 0;
    std::vector<std::size_t> box_indices;
    for (std::size_t i = 2; i < seglist.size(); ++i) {
        const auto el_idx = static_cast<std::size_t>(std::stoi(seglist[i]));
        // 0 Agent
        // 1 Wall
        // 2 Box
        // 3 Goal
        // 4 Empty
        // 5 Agent Goal
        // 6 Box Goal
        if (el_idx >= kNumElements) {
            std::stringstream s;
            s << "Unknown element type: " << el_idx;
            throw std::invalid_argument(s.str());
        }

        {
            switch (el_idx) {
                case 0:    // Agent
                    local_state.agent_idx = i - 2;
                    shared_state->board_static.push_back(Element::kEmpty);
                    ++agent_counter;
                    break;
                case 1:    // Wall
                    shared_state->board_static.push_back(Element::kWall);
                    break;
                case 2:    // Box
                    box_indices.push_back(i - 2);
                    local_state.box_indices_set.insert(box_indices.back());
                    shared_state->board_static.push_back(Element::kEmpty);
                    ++box_counter;
                    break;
                case 3:    // Goal
                    ++goal_counter;
                    shared_state->board_static.push_back(Element::kGoal);
                    break;
                case 4:    // Empty
                    shared_state->board_static.push_back(Element::kEmpty);
                    break;
                case 5:    // Agent on goal
                    local_state.agent_idx = i - 2;
                    shared_state->board_static.push_back(Element::kGoal);
                    ++goal_counter;
                    ++agent_counter;
                    break;
                case 6:    // Box on goal
                    box_indices.push_back(i - 2);
                    local_state.box_indices_set.insert(box_indices.back());
                    shared_state->board_static.push_back(Element::kGoal);
                    ++box_counter;
                    ++goal_counter;
                    break;
            }
        }
    }

    if (shared_state->board_static.size() != shared_state->rows * shared_state->cols) {
        throw std::invalid_argument("Missmatch in board elements");
    }
    if (box_counter != goal_counter) {
        throw std::invalid_argument("Missmatch in number of boxes and goals");
    }

    if (agent_counter == 0) {
        throw std::invalid_argument("Agent element not found");
    } else if (agent_counter > 1) {
        throw std::invalid_argument("Too many agent elements, expected only one");
    }

    // Ensure space for box_indices matches size
    local_state.box_indices.reserve(box_indices.size());
    for (const auto &box_idx : box_indices) {
        local_state.box_indices.push_back(box_idx);
    }
}

}    // namespace sokoban
