#include "sokoban_base.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <type_traits>
#include <unordered_set>

#include "definitions.h"
#include "util.h"

namespace sokoban {

SharedStateInfo::SharedStateInfo(const GameParameters &params)
    : params(params),
      obs_show_ids(std::get<bool>(params.at("obs_show_ids"))),
      rng_seed(std::get<int>(params.at("rng_seed"))),
      game_board_str(std::get<std::string>(params.at("game_board_str"))),
      gen(rng_seed),
      dist(0) {}

auto SharedStateInfo::operator==(const SharedStateInfo &other) const noexcept -> bool {
    return rows == other.rows && cols == other.cols && board_static == other.board_static;
}

auto LocalState::operator==(const LocalState &other) const noexcept -> bool {
    return agent_idx == other.agent_idx && std::is_permutation(box_indices.begin(), box_indices.end(),
                                                               other.box_indices.begin(), other.box_indices.end());
}

SokobanGameState::SokobanGameState(const GameParameters &params)
    : shared_state_ptr(std::make_shared<SharedStateInfo>(params)) {
    reset();
}

auto SokobanGameState::operator==(const SokobanGameState &other) const noexcept -> bool {
    return local_state == other.local_state && *shared_state_ptr == *other.shared_state_ptr;
}

const std::vector<Action> SokobanGameState::ALL_ACTIONS{Action::kUp, Action::kRight, Action::kDown, Action::kLeft};

// ---------------------------------------------------------------------------

void SokobanGameState::reset() noexcept {
    // Board, local, and shared state info
    local_state = LocalState();
    parse_board_str(shared_state_ptr->game_board_str, local_state, shared_state_ptr);

    // zorbist hashing
    std::mt19937 gen(shared_state_ptr->rng_seed);
    std::uniform_int_distribution<uint64_t> dist(0);
    const auto channel_size = shared_state_ptr->rows * shared_state_ptr->cols;
    shared_state_ptr->zrbht.clear();
    shared_state_ptr->zrbht.reserve(channel_size * kNumElements);
    for (std::size_t channel = 0; channel < kNumElements; ++channel) {
        for (std::size_t i = 0; i < channel_size; ++i) {
            shared_state_ptr->zrbht.push_back(dist(gen));
        }
    }

    // Set initial hash
    // Wall, Goal, Empty
    std::size_t i = 0;
    local_state.zorb_hash = 0;
    for (const auto &el : shared_state_ptr->board_static) {
        local_state.zorb_hash ^= shared_state_ptr->zrbht.at((static_cast<std::size_t>(el) * channel_size) + i);
        ++i;
    }
    // Agent
    local_state.zorb_hash ^=
        shared_state_ptr->zrbht.at((static_cast<std::size_t>(Element::kAgent) * channel_size) + local_state.agent_idx);
    // Boxes
    for (const auto &box_idx : local_state.box_indices) {
        local_state.zorb_hash ^=
            shared_state_ptr->zrbht.at((static_cast<std::size_t>(Element::kBox) * channel_size) + box_idx);
    }
}

void SokobanGameState::apply_action(Action action) noexcept {
    local_state.reward_signal = 0;
    // If action results not in bounds, don't do anything
    const auto agent_idx = local_state.agent_idx;
    if (!InBounds(agent_idx, action)) {
        return;
    }
    const auto new_index = IndexFromAction(agent_idx, action);
    // Move agent if not wall and not box (i.e. empty or over an unoccupied goal)
    if (IsTraversible(agent_idx, action)) {
        MoveAgent(action);
        local_state.agent_idx = new_index;
    } else if (IsPushable(agent_idx, action)) {
        Push(agent_idx, action);
        local_state.agent_idx = new_index;
    }
}

bool SokobanGameState::is_solution() const noexcept {
    // Every box lies on a goal tile
    for (const auto &box_idx : local_state.box_indices) {
        if (shared_state_ptr->board_static.at(box_idx) != Element::kGoal) {
            return false;
        }
    }
    return true;
}

auto SokobanGameState::legal_actions() const noexcept -> std::vector<Action> {
    return ALL_ACTIONS;
}

void SokobanGameState::legal_actions(std::vector<Action> &actions) const noexcept {
    actions.clear();
    for (const auto &a : ALL_ACTIONS) {
        actions.push_back(a);
    }
}

auto SokobanGameState::observation_shape() const noexcept -> std::array<std::size_t, 3> {
    // Empty doesn't get a channel, empty = all channels 0
    return {kNumElements - 1, shared_state_ptr->cols, shared_state_ptr->rows};
}

auto SokobanGameState::get_observation() const noexcept -> std::vector<float> {
    const auto channel_size = shared_state_ptr->rows * shared_state_ptr->cols;
    std::vector<float> obs((kNumElements - 1) * channel_size, 0);
    // Set wall and goal
    std::size_t i = 0;
    for (const auto &el : shared_state_ptr->board_static) {
        if (el == Element::kWall || el == Element::kGoal) {
            obs[(static_cast<std::size_t>(el) * channel_size) + i] = 1;
        }
        ++i;
    }
    // Set agent and boxes
    obs[(static_cast<std::size_t>(Element::kAgent) * channel_size) + local_state.agent_idx] = 1;
    for (const auto &box_idx : local_state.box_indices) {
        obs[(static_cast<std::size_t>(Element::kBox) * channel_size) + box_idx] = 1;
    }
    return obs;
}
void SokobanGameState::get_observation(std::vector<float> &obs) const noexcept {
    const auto channel_size = shared_state_ptr->rows * shared_state_ptr->cols;
    const auto obs_size = (kNumElements - 1) * channel_size;
    obs.clear();
    obs.reserve(obs_size);
    std::fill_n(std::back_inserter(obs), obs_size, 0);

    // Set wall and goal
    std::size_t i = 0;
    for (const auto &el : shared_state_ptr->board_static) {
        if (el == Element::kWall || el == Element::kGoal) {
            obs[(static_cast<std::size_t>(el) * channel_size) + i] = 1;
        }
        ++i;
    }
    // Set agent and boxes
    obs[(static_cast<std::size_t>(Element::kAgent) * channel_size) + local_state.agent_idx] = 1;
    for (const auto &box_idx : local_state.box_indices) {
        obs[(static_cast<std::size_t>(Element::kBox) * channel_size) + box_idx] = 1;
    }
}

// Binary image data
#include "assets_all.inc"

auto SokobanGameState::image_shape() const noexcept -> std::array<std::size_t, 3> {
    const auto rows = shared_state_ptr->rows;
    const auto cols = shared_state_ptr->cols;
    return {rows * SPRITE_HEIGHT, cols * SPRITE_WIDTH, SPRITE_CHANNELS};
}

auto SokobanGameState::to_image() const noexcept -> std::vector<uint8_t> {
    const auto rows = shared_state_ptr->rows;
    const auto cols = shared_state_ptr->cols;
    const std::size_t flat_size = rows * cols;
    std::vector<uint8_t> img(flat_size * SPRITE_DATA_LEN, 0);
    for (std::size_t h = 0; h < rows; ++h) {
        for (std::size_t w = 0; w < cols; ++w) {
            const std::size_t img_idx_top_left = h * (SPRITE_DATA_LEN * cols) + (w * SPRITE_DATA_LEN_PER_ROW);
            const auto idx = h * cols + w;
            int flags = 0;
            flags |= local_state.agent_idx == idx ? 1 << static_cast<int>(Element::kAgent) : 0;
            flags |=
                shared_state_ptr->board_static.at(idx) == Element::kWall ? 1 << static_cast<int>(Element::kWall) : 0;
            flags |=
                shared_state_ptr->board_static.at(idx) == Element::kGoal ? 1 << static_cast<int>(Element::kGoal) : 0;
            flags |= local_state.box_indices_set.find(idx) != local_state.box_indices_set.end()
                         ? 1 << static_cast<int>(Element::kBox)
                         : 0;
            const std::vector<uint8_t> &data = img_asset_map.at(flags);
            for (std::size_t r = 0; r < SPRITE_HEIGHT; ++r) {
                for (std::size_t c = 0; c < SPRITE_WIDTH; ++c) {
                    const std::size_t data_idx = (r * SPRITE_DATA_LEN_PER_ROW) + (3 * c);
                    const std::size_t img_idx =
                        (r * SPRITE_DATA_LEN_PER_ROW * cols) + (SPRITE_CHANNELS * c) + img_idx_top_left;
                    img[img_idx + 0] = data[data_idx + 0];
                    img[img_idx + 1] = data[data_idx + 1];
                    img[img_idx + 2] = data[data_idx + 2];
                }
            }
        }
    }
    return img;
}

auto SokobanGameState::get_reward_signal() const noexcept -> uint64_t {
    return local_state.reward_signal;
}

auto SokobanGameState::get_hash() const noexcept -> uint64_t {
    return local_state.zorb_hash;
}

auto SokobanGameState::get_unsolved_box_ids() const noexcept -> std::unordered_set<int> {
    std::unordered_set<int> ids;
    int box_id = 0;
    for (const auto &box_idx : local_state.box_indices) {
        if (shared_state_ptr->board_static.at(box_idx) != Element::kGoal) {
            ids.insert(box_id);
        }
        ++box_id;
    }
    return ids;
}

auto SokobanGameState::get_solved_box_ids() const noexcept -> std::unordered_set<int> {
    std::unordered_set<int> ids;
    int box_id = 0;
    for (const auto &box_idx : local_state.box_indices) {
        if (shared_state_ptr->board_static.at(box_idx) == Element::kGoal) {
            ids.insert(box_id);
        }
        ++box_id;
    }
    return ids;
}

auto SokobanGameState::get_all_box_ids() const noexcept -> std::unordered_set<int> {
    std::unordered_set<int> ids;
    for (std::size_t i = 0; i < local_state.box_indices.size(); ++i) {
        ids.insert(static_cast<int>(i));
    }
    return ids;
}

auto SokobanGameState::get_box_index(int box_id) const -> std::size_t {
    if (box_id < 0 || box_id >= static_cast<int>(local_state.box_indices.size())) {
        throw std::invalid_argument(std::string("Unknown box id: ") + std::to_string(box_id));
    }
    return local_state.box_indices[box_id];
}

auto SokobanGameState::get_empty_goal_indices() const noexcept -> std::unordered_set<std::size_t> {
    std::unordered_set<std::size_t> ids;
    std::size_t idx = 0;
    for (const auto &el : shared_state_ptr->board_static) {
        if (el == Element::kGoal && local_state.box_indices_set.find(idx) == local_state.box_indices_set.end()) {
            ids.insert(idx);
        }
        ++idx;
    }
    return ids;
}

auto SokobanGameState::get_solved_goal_indices() const noexcept -> std::unordered_set<std::size_t> {
    std::unordered_set<std::size_t> ids;
    std::size_t idx = 0;
    for (const auto &el : shared_state_ptr->board_static) {
        if (el == Element::kGoal && local_state.box_indices_set.find(idx) != local_state.box_indices_set.end()) {
            ids.insert(idx);
        }
        ++idx;
    }
    return ids;
}

auto SokobanGameState::get_all_goal_indices() const noexcept -> std::unordered_set<std::size_t> {
    std::unordered_set<std::size_t> ids;
    std::size_t idx = 0;
    for (const auto &el : shared_state_ptr->board_static) {
        if (el == Element::kGoal) {
            ids.insert(idx);
        }
        ++idx;
    }
    return ids;
}

auto SokobanGameState::get_agent_index() const noexcept -> std::size_t {
    return local_state.agent_idx;
}

std::ostream &operator<<(std::ostream &os, const SokobanGameState &state) {
    const auto rows = state.shared_state_ptr->rows;
    const auto cols = state.shared_state_ptr->cols;
    for (std::size_t h = 0; h < rows; ++h) {
        for (std::size_t w = 0; w < cols; ++w) {
            const auto idx = h * cols + w;
            int mask = 0;
            mask |= idx == state.local_state.agent_idx ? 1 << static_cast<int>(Element::kAgent) : 0;
            mask |= state.shared_state_ptr->board_static.at(idx) == Element::kWall
                        ? 1 << static_cast<int>(Element::kWall)
                        : 0;
            mask |= state.shared_state_ptr->board_static.at(idx) == Element::kGoal
                        ? 1 << static_cast<int>(Element::kGoal)
                        : 0;
            mask |= state.local_state.box_indices_set.find(idx) != state.local_state.box_indices_set.end()
                        ? 1 << static_cast<int>(Element::kBox)
                        : 0;
            os << kElementToStr.at(mask);
        }
        os << std::endl;
    }

    return os;
}

// ---------------------------------------------------------------------------

std::size_t SokobanGameState::IndexFromAction(std::size_t index, Action action) const noexcept {
    auto col = index % shared_state_ptr->cols;
    auto row = (index - col) / shared_state_ptr->cols;
    const auto &offsets = kActionOffsets[to_underlying(action)];    // NOLINT(*-bounds-constant-array-index)
    col += offsets.first;
    row += offsets.second;
    return (shared_state_ptr->cols * row) + col;
}

bool SokobanGameState::InBounds(std::size_t index, Action action) const noexcept {
    const auto rows = shared_state_ptr->rows;
    const auto cols = shared_state_ptr->cols;
    int col = static_cast<int>(index % cols);
    int row = static_cast<int>((index - col) / cols);
    const auto &offsets = kActionOffsets[to_underlying(action)];    // NOLINT(*-bounds-constant-array-index)
    col += offsets.first;
    row += offsets.second;
    return col >= 0 && col < static_cast<int>(cols) && row >= 0 && row < static_cast<int>(rows);
}

void SokobanGameState::MoveAgent(Action action) noexcept {
    const auto flat_size = shared_state_ptr->rows * shared_state_ptr->cols;
    local_state.zorb_hash ^=
        shared_state_ptr->zrbht.at(static_cast<std::size_t>(Element::kAgent) * flat_size + local_state.agent_idx);
    local_state.agent_idx = IndexFromAction(local_state.agent_idx, action);
    local_state.zorb_hash ^=
        shared_state_ptr->zrbht.at(static_cast<std::size_t>(Element::kAgent) * flat_size + local_state.agent_idx);
}

void SokobanGameState::MoveBox(std::size_t box_index, Action action) noexcept {
    const auto box_id =
        std::distance(local_state.box_indices.begin(),
                      std::find(local_state.box_indices.begin(), local_state.box_indices.end(), box_index));
    const auto flat_size = shared_state_ptr->rows * shared_state_ptr->cols;
    local_state.zorb_hash ^=
        shared_state_ptr->zrbht.at(static_cast<std::size_t>(Element::kBox) * flat_size + box_index);
    local_state.box_indices_set.erase(box_index);

    // Move box
    const auto box_new_idx = IndexFromAction(box_index, action);
    local_state.zorb_hash ^=
        shared_state_ptr->zrbht.at(static_cast<std::size_t>(Element::kBox) * flat_size + box_new_idx);
    local_state.box_indices_set.insert(box_new_idx);
    local_state.box_indices[box_id] = box_new_idx;

    // Check if on goal
    const bool box_on_goal = shared_state_ptr->board_static.at(box_new_idx) == Element::kGoal;
    local_state.reward_signal = box_on_goal ? (box_new_idx * flat_size + box_id + 1) : 0;
}

bool SokobanGameState::IsTraversible(std::size_t index, Action action) const noexcept {
    if (!InBounds(index, action)) {
        return false;
    }
    const auto new_index = IndexFromAction(index, action);
    // new_index isn't box and isn't wall
    return local_state.box_indices_set.find(new_index) == local_state.box_indices_set.end() &&
           shared_state_ptr->board_static.at(new_index) != Element::kWall;
}

bool SokobanGameState::IsPushable(std::size_t index, Action action) const noexcept {
    if (!InBounds(index, action)) {
        return false;
    }
    const auto box_index = IndexFromAction(index, action);

    // No box at the query index
    if (local_state.box_indices_set.find(box_index) == local_state.box_indices_set.end()) {
        return false;
    }

    // Check 1 past box in same direction
    return IsTraversible(box_index, action);
}

void SokobanGameState::Push(std::size_t index, Action action) noexcept {
    const auto box_index = IndexFromAction(index, action);
    MoveBox(box_index, action);
    MoveAgent(action);
}

// ---------------------------------------------------------------------------

}    // namespace sokoban
