#include "sokoban_base.h"

#include <cstdint>
#include <type_traits>

#include "definitions.h"

namespace sokoban {

SokobanGameState::SokobanGameState(const GameParameters &params)
    : shared_state_ptr(std::make_shared<SharedStateInfo>(params)),
      board(util::parse_board_str(std::get<std::string>(params.at("game_board_str")))) {
    reset();
}

bool SokobanGameState::operator==(const SokobanGameState &other) const {
    return board == other.board;
}

// ---------------------------------------------------------------------------

void SokobanGameState::reset() {
    // Board, local, and shared state info
    board = util::parse_board_str(shared_state_ptr->game_board_str);
    local_state = LocalState();

    // zorbist hashing
    std::mt19937 gen(shared_state_ptr->rng_seed);
    std::uniform_int_distribution<uint64_t> dist(0);
    for (int channel = 0; channel < kNumElementTypes; ++channel) {
        for (int i = 0; i < board.cols * board.rows; ++i) {
            shared_state_ptr->zrbht[(channel * board.cols * board.rows) + i] = dist(gen);
        }
    }

    // Set initial hash
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        for (int channel = 0; channel < (int)channel_items.size(); ++channel) {
            if (channel_items[channel] > 0) {
                board.zorb_hash ^= shared_state_ptr->zrbht.at((channel * board.cols * board.rows) + i);
            } 
        }
    }
}

void SokobanGameState::apply_action(int action) {
    assert(action >= 0 && action < kNumActions);

    // If action results not in bounds, don't do anything
    int agent_idx = board.agent_idx;
    if (!InBounds(agent_idx, action)) {
        return;
    }
    int new_index = IndexFromAction(agent_idx, action);
    // Move agent if not wall and not box (i.e. empty or over an unoccupied goal)
    if (IsTraversible(agent_idx, action)) {
        MoveItem(static_cast<int>(ElementTypes::kAgent), agent_idx, action);
        board.agent_idx = new_index;
    } else if (IsPushable(agent_idx, action)) {
        Push(agent_idx, action);
        board.agent_idx = new_index;
    }
}

bool SokobanGameState::is_solution() const {
    // No boxes in map (i.e. they are on goals)
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        // Box on grid without also being ontop of goal
        if (channel_items[static_cast<int>(ElementTypes::kBox)] && !channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            return false;
        }
    }
    return true;
}

std::vector<int> SokobanGameState::legal_actions() const {
    return {Directions::kUp, Directions::kRight, Directions::kDown, Directions::kLeft};
}

std::array<int, 3> SokobanGameState::observation_shape() const {
    // Empty doesn't get a channel, empty = all channels 0
    return {kNumElementTypes, board.cols, board.rows};
}

std::vector<float> SokobanGameState::get_observation() const {
    std::vector<float> obs(kNumElementTypes* board.cols * board.rows, 0);
    int channel_length = board.cols * board.rows;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        for (int j = 0; j < (int)channel_items.size(); ++j) {
            obs[j * channel_length + i] = channel_items[j] > 0 ? 1 : 0;
        }
    }
    return obs;
}

uint64_t SokobanGameState::get_reward_signal() const {
    return local_state.reward_signal;
}

uint64_t SokobanGameState::get_hash() const {
    return board.zorb_hash;
}

std::unordered_set<int> SokobanGameState::get_unsolved_box_ids() const {
    std::unordered_set<int> ids;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kBox)] && !channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            ids.insert(channel_items[static_cast<int>(ElementTypes::kBox)]);
        }
    }
    return ids;
}

std::unordered_set<int> SokobanGameState::get_all_box_ids() const {
    std::unordered_set<int> ids;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kBox)]) {
            ids.insert(channel_items[static_cast<int>(ElementTypes::kBox)]);
        }
    }
    return ids;
}

int SokobanGameState::get_box_index(int box_id) const {
    for (int i = 0; i < board.cols * board.rows; ++i) {
        if (board.item(static_cast<int>(ElementTypes::kBox), i) == box_id) {
            return i;
        }
    }
    return -1;
}

std::unordered_set<int> SokobanGameState::get_empty_goals() const {
    std::unordered_set<int> ids;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kGoal)] && !channel_items[static_cast<int>(ElementTypes::kBox)]) {
            ids.insert(i);
        }
    }
    return ids;
}

std::unordered_set<int> SokobanGameState::get_all_goals() const {
    std::unordered_set<int> ids;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            ids.insert(i);
        }
    }
    return ids;
}

int SokobanGameState::get_agent_index() const {
    return board.agent_idx;
}

std::ostream &operator<<(std::ostream &os, const SokobanGameState &state) {
    for (int h = 0; h < state.board.rows; ++h) {
        for (int w = 0; w < state.board.cols; ++w) {
            auto channel_items = state.board.get_channel_items(h * state.board.cols + w);
            int mask = 0;
            for (int i = 0; i < (int)channel_items.size(); ++i) {
                if (channel_items[i]) {
                    mask |= 1 << i;
                }
            }
            os << kElementsToStr.at(mask);
        }
        os << std::endl;
    }
    return os;
}

// ---------------------------------------------------------------------------

int SokobanGameState::IndexFromAction(int index, int action) const {
    int col = index % board.cols;
    int row = (index - col) / board.cols;
    std::pair<int, int> offsets = kDirectionOffsets.at(action);
    col += offsets.first;
    row += offsets.second;
    return (board.cols * row) + col;
}

bool SokobanGameState::InBounds(int index, int action) const {
    int col = index % board.cols;
    int row = (index - col) / board.cols;
    std::pair<int, int> offsets = kDirectionOffsets.at(action);
    col += offsets.first;
    row += offsets.second;
    return col >= 0 && col < board.cols && row >= 0 && row < board.rows;
}

void SokobanGameState::MoveItem(int channel, int index, int action) {
    int new_index = IndexFromAction(index, action);
    board.zorb_hash ^= shared_state_ptr->zrbht.at((channel * board.cols * board.rows) + index);
    board.item(channel, new_index) = board.item(channel, index);
    board.item(channel, index) = 0;
    board.zorb_hash ^= shared_state_ptr->zrbht.at((channel * board.cols * board.rows) + new_index);
}

bool SokobanGameState::IsTraversible(int index, int action) const {
    if (!InBounds(index, action)) {
        return false;
    }
    int new_index = IndexFromAction(index, action);
    auto channel_items = board.get_channel_items(new_index);
    return !channel_items[static_cast<int>(ElementTypes::kBox)] && !channel_items[static_cast<int>(ElementTypes::kWall)];
}

bool SokobanGameState::IsPushable(int index, int action) const {
    if (!InBounds(index, action)) {
        return false;
    }
    int box_index = IndexFromAction(index, action);
    if (!board.item(static_cast<int>(ElementTypes::kBox), box_index)) {
        return false;
    }
    
    // Check 1 past box in same direction
    return IsTraversible(box_index, action);
}

void SokobanGameState::Push(int index, int action) {
    int new_index = IndexFromAction(index, action);
    MoveItem(static_cast<int>(ElementTypes::kBox), new_index, action);
    MoveItem(static_cast<int>(ElementTypes::kAgent), index, action);
}

// ---------------------------------------------------------------------------

}    // namespace sokoban
