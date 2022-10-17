#include "sokoban_base.h"

#include <cstdint>
#include <queue>
#include <type_traits>
#include <unordered_set>

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

void bfs(int s, int rows, int cols, const std::vector<bool> &pathable_board, std::vector<bool> &reachability_set) {
    std::array<int, 4> OFFSETS_4 = {-1, 1, -cols, cols};
    std::queue<int> open;    // Tile idx + idx pulling from
    std::unordered_set<int> open_set;
    std::unordered_set<int> closed;
    reachability_set.clear();
    reachability_set.insert(reachability_set.end(), pathable_board.size(), false);

    auto is_in_bounds = [&](int idx) {
        int r = idx / cols;
        int c = idx % cols;
        return !(r < 0 || c < 0 || r >= rows || c >= cols);
    };

    open.push(s);
    open_set.insert(s);
    while (!open.empty()) {
        int idx = open.front();
        open.pop();
        closed.insert(idx);
        reachability_set[idx] = true;

        // children
        for (const auto &offset : OFFSETS_4) {
            int child_idx = idx + offset;
            if (!is_in_bounds(child_idx)) {
                continue;
            }
            // blocked, or previously in open/closed
            if (!pathable_board[child_idx] || open_set.find(child_idx) != open_set.end() ||
                closed.find(child_idx) != closed.end()) {
                continue;
            }
            // This should be safe because if we are walking out of bounds (wrapping around),
            // the above would have caught it
            if (!is_in_bounds(child_idx + offset)) {
                continue;
            }
            if (pathable_board[child_idx + offset]) {
                open.push(child_idx);
            }
        }
    }
}

void SokobanGameState::init_deadlock_detection() {
    shared_state_ptr->nondead_squares.clear();
    std::vector<bool> pathable_board(board.rows * board.cols, false);
    for (int i = 0; i < board.rows * board.cols; ++i) {
        pathable_board[i] = board.item(static_cast<int>(ElementTypes::kWall), i) == 0;
    }
    for (const auto &goal_idx : get_all_goals()) {
        std::vector<bool> reachability_set;
        bfs(goal_idx, board.rows, board.cols, pathable_board, reachability_set);
        shared_state_ptr->nondead_squares[goal_idx] = reachability_set;
    }
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
    init_deadlock_detection();

    // for (auto const &[k, v] : shared_state_ptr->nondead_squares) {
    //     std::cout << "goal " << k << ", idx: ";
    //     for (int i = 0; i < (int)v.size(); ++i) {
    //         if (v[i] == 0) {
    //             std::cout << i << " ";
    //         }
    //     }
    //     std::cout << std::endl;
    // }
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
        if (channel_items[static_cast<int>(ElementTypes::kBox)] &&
            !channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            return false;
        }
    }
    return true;
}

std::vector<int> SokobanGameState::legal_actions() const {
    return {Directions::kUp, Directions::kRight, Directions::kDown, Directions::kLeft};
}

bool is_wall(const std::array<int, 4> &channel_items) {
    return channel_items[static_cast<int>(ElementTypes::kWall)];
}

bool is_empty_goal(const std::array<int, 4> &channel_items) {
    return channel_items[static_cast<int>(ElementTypes::kGoal)] && !channel_items[static_cast<int>(ElementTypes::kBox)];
}

bool is_complete_goal(const std::array<int, 4> &channel_items) {
    return channel_items[static_cast<int>(ElementTypes::kGoal)] && channel_items[static_cast<int>(ElementTypes::kBox)];
}

bool non_traversable(const std::array<int, 4> &channel_items) {
    // return is_wall(channel_items);
    // return is_wall(channel_items) || is_complete_goal(channel_items);
    return channel_items[static_cast<int>(ElementTypes::kBox)] || channel_items[static_cast<int>(ElementTypes::kWall)];
}

bool SokobanGameState::is_deadlocked() const {
    // Check each goal if no boxes in non-dead squares
    for (const auto &goal_idx : get_empty_goals()) {
        bool flag = true;
        for (const auto &box_idx : get_unsolved_box_idxs()) {
            // Found a box that lies on a non-dead square
            if (shared_state_ptr->nondead_squares[goal_idx][box_idx]) {
                flag = false;
                break;
            }
        }
        if (flag) {
            return true;
        }
    }

    // Check each box to see if its contained in no-ones non-dead squares
    for (const auto &box_idx : get_unsolved_box_idxs()) {
        bool flag = true;
        for (const auto &goal_idx : get_empty_goals()) {
            // Found a goal that this rock can path to
            if (shared_state_ptr->nondead_squares[goal_idx][box_idx]) {
                flag = false;
                break;
            }
        }
        if (flag) {
            return true;
        }
    }

    // Check each box for deadlock
    for (const auto &box_idx : get_unsolved_box_idxs()) {
        // bool non_traversable_N = non_traversable(board.get_channel_items(box_idx - board.cols));
        // bool non_traversable_S = non_traversable(board.get_channel_items(box_idx + board.cols));
        // bool non_traversable_E = non_traversable(board.get_channel_items(box_idx + 1));
        // bool non_traversable_W = non_traversable(board.get_channel_items(box_idx - 1));
        // bool is_wall_N = is_wall(board.get_channel_items(box_idx - board.cols));
        // bool is_wall_S = is_wall(board.get_channel_items(box_idx + board.cols));
        // bool is_wall_E = is_wall(board.get_channel_items(box_idx + 1));
        // bool is_wall_W = is_wall(board.get_channel_items(box_idx - 1));

        // Check corner case, i.e. non-traversable for pair of N/S and E/W
        // Non-traversible could mix up these 2 scenarios (left deadlock, right isn't)
        //    #             #*
        //   *#              *
        //  ###
        // if ((non_traversable_N && non_traversable_E) || (non_traversable_N && non_traversable_W) ||
        //     (non_traversable_S && non_traversable_E) || (non_traversable_S && non_traversable_W)) {
        //         std::cout << "bad idx " << box_idx << std::endl;
        //     return true;
        // }
        // if ((is_wall_N && is_wall_E) || (is_wall_N && is_wall_W) || (is_wall_S && is_wall_E) ||
        //     (is_wall_S && is_wall_W)) {
        //     std::cout << "bad idx " << box_idx << std::endl;
        //     return true;
        // }

        // Check 2 rock case
        //       #       #           #######       #######
        //      .#       #.              * #       # *
        //     * #       # *              .#       #.
        // #######       #######           #       #
        auto check_two_rock = [&](const std::vector<int> &offsets) {
            bool flag = true;
            for (auto const &o : offsets) {
                flag &= non_traversable(board.get_channel_items(box_idx + o));
            }
            return flag;
        };

        for (const auto &other_box_idx : get_all_box_idxs()) {
            auto offset = box_idx - other_box_idx;
            if (offset == -(board.cols - 1)) {
                // Down left
                if (check_two_rock({2 * board.cols - 1, 2 * board.cols, 2 * board.cols + 1, board.cols + 1, 1}) &&
                    !is_empty_goal(board.get_channel_items(box_idx + board.cols))) {
                    // std::cout << "bad idx a " << box_idx << std::endl;
                    return true;
                }
            } else if (offset == -(board.cols + 1)) {
                // Down right
                if (check_two_rock({2 * board.cols + 1, 2 * board.cols, 2 * board.cols - 1, board.cols - 1, -1}) &&
                    !is_empty_goal(board.get_channel_items(box_idx + board.cols))) {
                    // std::cout << "bad idx b " << box_idx << std::endl;
                    return true;
                }
            } else if (offset == (board.cols + 1)) {
                // Up left
                if (check_two_rock(
                        {-(2 * board.cols + 1), -(2 * board.cols), -(2 * board.cols - 1), -(board.cols - 1), 1}) &&
                    !is_empty_goal(board.get_channel_items(box_idx - board.cols))) {
                    // std::cout << "bad idx c " << box_idx << std::endl;
                    return true;
                }
            } else if (offset == (board.cols - 1)) {
                // Up right
                if (check_two_rock(
                        {-(2 * board.cols - 1), -(2 * board.cols), -(2 * board.cols + 1), -(board.cols + 1), -1}) &&
                    !is_empty_goal(board.get_channel_items(box_idx - board.cols))) {
                    // std::cout << "bad idx d " << box_idx << std::endl;
                    return true;
                }
            }
        }
    }

    return false;
}

std::vector<int> SokobanGameState::legal_actions_no_deadlocks() const {
    std::vector<int> actions;

    SokobanGameState temp_state(*this);
    for (auto const &action : {Directions::kUp, Directions::kRight, Directions::kDown, Directions::kLeft}) {
        temp_state.board = this->board;
        temp_state.apply_action(action);
        if (!temp_state.is_deadlocked()) {
            actions.push_back(action);
        }
    }

    return actions;
}

std::array<int, 3> SokobanGameState::observation_shape() const {
    // Empty doesn't get a channel, empty = all channels 0
    return {kNumElementTypes, board.cols, board.rows};
}

std::vector<float> SokobanGameState::get_observation() const {
    std::vector<float> obs(kNumElementTypes * board.cols * board.rows, 0);
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
        if (channel_items[static_cast<int>(ElementTypes::kBox)] &&
            !channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            ids.insert(channel_items[static_cast<int>(ElementTypes::kBox)]);
        }
    }
    return ids;
}

std::unordered_set<int> SokobanGameState::get_solved_box_ids() const {
    std::unordered_set<int> ids;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kBox)] &&
            channel_items[static_cast<int>(ElementTypes::kGoal)]) {
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

std::unordered_set<int> SokobanGameState::get_unsolved_box_idxs() const {
    std::unordered_set<int> idxs;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kBox)] &&
            !channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            idxs.insert(i);
        }
    }
    return idxs;
}

std::unordered_set<int> SokobanGameState::get_solved_box_idxs() const {
    std::unordered_set<int> idxs;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kBox)] &&
            channel_items[static_cast<int>(ElementTypes::kGoal)]) {
            idxs.insert(i);
        }
    }
    return idxs;
}

std::unordered_set<int> SokobanGameState::get_all_box_idxs() const {
    std::unordered_set<int> idxs;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kBox)]) {
            idxs.insert(i);
        }
    }
    return idxs;
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
        if (channel_items[static_cast<int>(ElementTypes::kGoal)] &&
            !channel_items[static_cast<int>(ElementTypes::kBox)]) {
            ids.insert(i);
        }
    }
    return ids;
}

std::unordered_set<int> SokobanGameState::get_solved_goals() const {
    std::unordered_set<int> ids;
    for (int i = 0; i < board.cols * board.rows; ++i) {
        auto channel_items = board.get_channel_items(i);
        if (channel_items[static_cast<int>(ElementTypes::kGoal)] &&
            channel_items[static_cast<int>(ElementTypes::kBox)]) {
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
    return !channel_items[static_cast<int>(ElementTypes::kBox)] &&
           !channel_items[static_cast<int>(ElementTypes::kWall)];
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
