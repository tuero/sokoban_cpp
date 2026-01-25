#include "sokoban_base.h"

#include <cstdint>
#include <sstream>

#include "definitions.h"

namespace sokoban {

namespace {
// https://en.wikipedia.org/wiki/Xorshift
// Portable RNG Seed
constexpr uint64_t SPLIT64_S1 = 30;
constexpr uint64_t SPLIT64_S2 = 27;
constexpr uint64_t SPLIT64_S3 = 31;
constexpr uint64_t SPLIT64_C1 = 0x9E3779B97f4A7C15;
constexpr uint64_t SPLIT64_C2 = 0xBF58476D1CE4E5B9;
constexpr uint64_t SPLIT64_C3 = 0x94D049BB133111EB;
auto to_local_hash(int flat_size, Element el, int offset) noexcept -> uint64_t {
    auto seed = static_cast<uint64_t>((flat_size * static_cast<int>(el)) + offset);
    uint64_t result = seed + SPLIT64_C1;
    result = (result ^ (result >> SPLIT64_S1)) * SPLIT64_C2;
    result = (result ^ (result >> SPLIT64_S2)) * SPLIT64_C3;
    return result ^ (result >> SPLIT64_S3);
}
}    // namespace

SokobanGameState::SokobanGameState(const std::string& board_str) {
    std::stringstream board_ss(board_str);
    std::string segment;
    std::vector<std::string> seglist;
    // string split on |
    while (std::getline(board_ss, segment, '|')) {
        seglist.push_back(segment);
    }

    assert(seglist.size() >= 2);

    // Get general info
    rows = std::stoi(seglist[0]);
    cols = std::stoi(seglist[1]);
    if (rows < 1 || cols < 1) {
        throw std::invalid_argument("rows and/or cols < 1");
    }

    assert(static_cast<int>(seglist.size()) == rows * cols + 2);

    // Parse grid
    int agent_counter = 0;
    int box_counter = 0;
    int goal_counter = 0;
    for (std::size_t i = 2; i < seglist.size(); ++i) {
        const auto el_idx = std::stoi(seglist[i]);
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
                    agent_idx = static_cast<int>(i) - 2;
                    board_static.push_back(Element::kEmpty);
                    is_box.push_back(false);
                    ++agent_counter;
                    break;
                case 1:    // Wall
                    board_static.push_back(Element::kWall);
                    is_box.push_back(false);
                    break;
                case 2:    // Box
                    is_box.push_back(true);
                    board_static.push_back(Element::kEmpty);
                    ++box_counter;
                    break;
                case 3:    // Goal
                    ++goal_counter;
                    is_box.push_back(false);
                    board_static.push_back(Element::kGoal);
                    break;
                case 4:    // Empty
                    is_box.push_back(false);
                    board_static.push_back(Element::kEmpty);
                    break;
                case 5:    // Agent on goal
                    agent_idx = static_cast<int>(i) - 2;
                    is_box.push_back(false);
                    board_static.push_back(Element::kGoal);
                    ++goal_counter;
                    ++agent_counter;
                    break;
                case 6:    // Box on goal
                    is_box.push_back(true);
                    board_static.push_back(Element::kGoal);
                    ++box_counter;
                    ++goal_counter;
                    break;
            }
        }
    }

    if (board_static.size() != static_cast<std::size_t>(rows * cols)) {
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

    // Init hash
    int flat_size = rows * cols;
    zorb_hash = 0;
    // Static board
    {
        int i = -1;
        for (const auto& el : board_static) {
            zorb_hash ^= to_local_hash(flat_size, el, ++i);
        }
    }
    // Dynamic elements
    zorb_hash ^= to_local_hash(flat_size, Element::kAgent, agent_idx);
    {
        int i = -1;
        for (const auto b : is_box) {
            ++i;
            if (b) {
                zorb_hash ^= to_local_hash(flat_size, Element::kBox, i);
            }
        }
    }
}

SokobanGameState::SokobanGameState(InternalState&& internal_state)
    : rows(internal_state.rows),
      cols(internal_state.cols),
      agent_idx(internal_state.agent_idx),
      zorb_hash(internal_state.hash),
      reward_signal(internal_state.reward_signal),
      is_box(std::move(internal_state).is_box) {
    board_static.clear();
    board_static.reserve(internal_state.board_static.size());
    for (const auto& el : internal_state.board_static) {
        board_static.push_back(static_cast<Element>(el));
    }
}

auto SokobanGameState::operator==(const SokobanGameState& other) const noexcept -> bool {
    return rows == other.rows && cols == other.cols && agent_idx == other.agent_idx &&
           board_static == other.board_static && is_box == other.is_box;
}

auto SokobanGameState::operator!=(const SokobanGameState& other) const noexcept -> bool {
    return !(*this == other);
}

// ---------------------------------------------------------------------------

void SokobanGameState::apply_action(Action action) {
    assert(is_valid_action(action));

    reward_signal = 0;
    // If action results not in bounds, don't do anything
    if (!InBounds(agent_idx, action)) {
        return;
    }
    const auto new_index = IndexFromAction(agent_idx, action);
    // Move agent if not wall and not box (i.e. empty or over an unoccupied goal)
    if (IsTraversible(agent_idx, action)) {
        MoveAgent(action);
        agent_idx = new_index;
    } else if (IsPushable(agent_idx, action)) {
        Push(agent_idx, action);
        agent_idx = new_index;
    }
}

auto SokobanGameState::is_solution() const noexcept -> bool {
    // Every box lies on a goal tile
    for (std::size_t i = 0; i < static_cast<std::size_t>(rows * cols); ++i) {
        if (!is_box[i] && board_static[i] == Element::kGoal) {
            return false;
        }
    }
    return true;
}

auto SokobanGameState::observation_shape(bool compact) const noexcept -> std::array<int, 3> {
    // Empty doesn't get a channel, empty = all channels 0
    return {compact ? kNumChannelsCompact : kNumChannels, cols, rows};
}

void SokobanGameState::_get_observation_non_compact(std::vector<float>& obs) const noexcept {
    const auto channel_size = static_cast<std::size_t>(rows * cols);

    // Set empty channel
    for (std::size_t i = 0; i < channel_size; ++i) {
        obs[(static_cast<std::size_t>(Element::kEmpty) * channel_size) + i] = 1;
    }

    // Set wall and goal (remove empty from these cells)
    std::size_t i = 0;
    for (const auto& el : board_static) {
        if (el == Element::kWall || el == Element::kGoal) {
            obs[(static_cast<std::size_t>(el) * channel_size) + i] = 1;
            obs[(static_cast<std::size_t>(Element::kEmpty) * channel_size) + i] = 0;
        }
        ++i;
    }

    // Set agent
    bool agent_on_goal =
        obs[(static_cast<std::size_t>(Element::kGoal) * channel_size) + static_cast<std::size_t>(agent_idx)] == 1;
    std::size_t agent_channel = agent_on_goal ? ChannelAgentOnGoal : static_cast<std::size_t>(Element::kAgent);
    obs[(agent_channel * channel_size) + static_cast<std::size_t>(agent_idx)] = 1;
    obs[(static_cast<std::size_t>(Element::kEmpty) * channel_size) + static_cast<std::size_t>(agent_idx)] = 0;

    // Set boxes
    for (std::size_t box_idx = 0; box_idx < channel_size; ++box_idx) {
        if (!is_box[static_cast<std::size_t>(box_idx)]) {
            continue;
        }
        bool box_on_goal = obs[(static_cast<std::size_t>(Element::kGoal) * channel_size) + box_idx] == 1;
        // Box channel is either box + goal or just box
        std::size_t box_channel = box_on_goal ? ChannelBoxOnGoal : static_cast<std::size_t>(Element::kBox);
        obs[(box_channel * channel_size) + box_idx] = 1;
        // In any case, we clear empty channel
        obs[(static_cast<std::size_t>(Element::kEmpty) * channel_size) + box_idx] = 0;
        // Goal needs to be cleared if box on goal
        // If box not on goal, we just clear empty which we already did, to avoid branching
        std::size_t goal_channel =
            box_on_goal ? static_cast<std::size_t>(Element::kGoal) : static_cast<std::size_t>(Element::kEmpty);
        obs[(goal_channel * channel_size) + box_idx] = 0;
    }
}

void SokobanGameState::_get_observation_compact(std::vector<float>& obs) const noexcept {
    const auto channel_size = static_cast<std::size_t>(rows * cols);

    // Set wall and goal
    std::size_t i = 0;
    for (const auto& el : board_static) {
        if (el == Element::kWall || el == Element::kGoal) {
            obs[(static_cast<std::size_t>(el) * channel_size) + i] = 1;
        }
        ++i;
    }
    // Set agent and boxes
    obs[(static_cast<std::size_t>(Element::kAgent) * channel_size) + static_cast<std::size_t>(agent_idx)] = 1;
    for (std::size_t box_idx = 0; box_idx < channel_size; ++box_idx) {
        if (!is_box[static_cast<std::size_t>(box_idx)]) {
            continue;
        }
        obs[(static_cast<std::size_t>(Element::kBox) * channel_size) + box_idx] = 1;
    }
}

auto SokobanGameState::get_observation(bool compact) const noexcept -> std::vector<float> {
    const auto channel_size = static_cast<std::size_t>(rows * cols);
    std::vector<float> obs(static_cast<std::size_t>(compact ? kNumChannelsCompact : kNumChannels) * channel_size, 0);
    if (compact) {
        _get_observation_compact(obs);
    } else {
        _get_observation_non_compact(obs);
    }
    return obs;
}

// Binary image data
#include "assets_all.inc"

auto SokobanGameState::image_shape() const noexcept -> std::array<int, 3> {
    return {rows * SPRITE_HEIGHT, cols * SPRITE_WIDTH, SPRITE_CHANNELS};
}

auto SokobanGameState::to_image() const noexcept -> std::vector<uint8_t> {
    const auto flat_size = static_cast<std::size_t>(rows * cols);
    std::vector<uint8_t> img(flat_size * SPRITE_DATA_LEN, 0);
    for (int h = 0; h < rows; ++h) {
        for (int w = 0; w < cols; ++w) {
            const auto img_idx_top_left =
                static_cast<std::size_t>(h * (SPRITE_DATA_LEN * cols) + (w * SPRITE_DATA_LEN_PER_ROW));
            const auto idx = static_cast<std::size_t>(h * cols + w);
            int flags = 0;
            flags |= static_cast<std::size_t>(agent_idx) == idx ? 1 << static_cast<int>(Element::kAgent) : 0;
            flags |= board_static.at(idx) == Element::kWall ? 1 << static_cast<int>(Element::kWall) : 0;
            flags |= board_static.at(idx) == Element::kGoal ? 1 << static_cast<int>(Element::kGoal) : 0;
            flags |= is_box[static_cast<std::size_t>(idx)] ? 1 << static_cast<int>(Element::kBox) : 0;
            const std::vector<uint8_t>& data = img_asset_map.at(flags);
            for (std::size_t r = 0; r < SPRITE_HEIGHT; ++r) {
                for (std::size_t c = 0; c < SPRITE_WIDTH; ++c) {
                    const std::size_t data_idx = (r * SPRITE_DATA_LEN_PER_ROW) + (3 * c);
                    const std::size_t img_idx = (r * SPRITE_DATA_LEN_PER_ROW * static_cast<std::size_t>(cols)) +
                                                (SPRITE_CHANNELS * c) + img_idx_top_left;
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
    return reward_signal;
}

auto SokobanGameState::get_hash() const noexcept -> uint64_t {
    return zorb_hash;
}

auto SokobanGameState::get_box_indices() const noexcept -> std::vector<int> {
    std::vector<int> indices;
    for (int i = 0; i < rows * cols; ++i) {
        if (is_box[static_cast<std::size_t>(i)]) {
            indices.push_back(i);
        }
    }
    return indices;
}

auto SokobanGameState::get_empty_goal_indices() const noexcept -> std::vector<int> {
    std::vector<int> indices;
    for (int i = 0; i < rows * cols; ++i) {
        if (!is_box[static_cast<std::size_t>(i)] && board_static[static_cast<std::size_t>(i)] == Element::kGoal) {
            indices.push_back(i);
        }
    }
    return indices;
}

auto SokobanGameState::get_solved_goal_indices() const noexcept -> std::vector<int> {
    std::vector<int> indices;
    for (int i = 0; i < rows * cols; ++i) {
        if (is_box[static_cast<std::size_t>(i)] && board_static[static_cast<std::size_t>(i)] == Element::kGoal) {
            indices.push_back(i);
        }
    }
    return indices;
}

auto SokobanGameState::get_all_goal_indices() const noexcept -> std::vector<int> {
    std::vector<int> indices;
    for (int i = 0; i < rows * cols; ++i) {
        if (board_static[static_cast<std::size_t>(i)] == Element::kGoal) {
            indices.push_back(i);
        }
    }
    return indices;
}

auto SokobanGameState::get_agent_index() const noexcept -> int {
    return agent_idx;
}

auto operator<<(std::ostream& os, const SokobanGameState& state) -> std::ostream& {
    const auto print_horz_boarder = [&]() {
        for (int w = 0; w < state.cols + 2; ++w) {
            os << "-";
        }
        os << std::endl;
    };
    print_horz_boarder();
    for (int h = 0; h < state.rows; ++h) {
        os << "|";
        for (int w = 0; w < state.cols; ++w) {
            const auto idx = static_cast<std::size_t>(h * state.cols + w);
            int mask = 0;
            mask |= idx == static_cast<std::size_t>(state.agent_idx) ? 1 << static_cast<int>(Element::kAgent) : 0;
            mask |= state.board_static.at(idx) == Element::kWall ? 1 << static_cast<int>(Element::kWall) : 0;
            mask |= state.board_static.at(idx) == Element::kGoal ? 1 << static_cast<int>(Element::kGoal) : 0;
            mask |= state.is_box[static_cast<std::size_t>(idx)] ? 1 << static_cast<int>(Element::kBox) : 0;
            os << kElementToStr.at(mask);
        }
        os << "|" << std::endl;
    }
    print_horz_boarder();

    return os;
}

// ---------------------------------------------------------------------------

auto SokobanGameState::IndexFromAction(int index, Action action) const noexcept -> int {
    auto col = index % cols;
    auto row = (index - col) / cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-array-index)
    col += offsets.first;
    row += offsets.second;
    return (cols * row) + col;
}

auto SokobanGameState::InBounds(int index, Action action) const noexcept -> bool {
    int col = index % cols;
    int row = (index - col) / cols;
    const auto& offsets = kActionOffsets[static_cast<std::size_t>(action)];    // NOLINT(*-array-index)
    col += offsets.first;
    row += offsets.second;
    return col >= 0 && col < static_cast<int>(cols) && row >= 0 && row < static_cast<int>(rows);
}

void SokobanGameState::MoveAgent(Action action) noexcept {
    const auto flat_size = rows * cols;
    zorb_hash ^= to_local_hash(flat_size, Element::kAgent, agent_idx);
    agent_idx = IndexFromAction(agent_idx, action);
    zorb_hash ^= to_local_hash(flat_size, Element::kAgent, agent_idx);
}

void SokobanGameState::MoveBox(int box_index, Action action) noexcept {
    const auto flat_size = rows * cols;
    zorb_hash ^= to_local_hash(flat_size, Element::kBox, box_index);
    is_box[static_cast<std::size_t>(box_index)] = false;

    // Move box
    const auto box_new_index = IndexFromAction(box_index, action);
    zorb_hash ^= to_local_hash(flat_size, Element::kBox, box_new_index);
    is_box[static_cast<std::size_t>(box_new_index)] = true;

    // Check if on goal
    const bool box_on_goal = board_static.at(static_cast<std::size_t>(box_new_index)) == Element::kGoal;
    reward_signal = box_on_goal ? 1 : 0;
}

auto SokobanGameState::IsTraversible(int index, Action action) const noexcept -> bool {
    if (!InBounds(index, action)) {
        return false;
    }
    const auto new_index = static_cast<std::size_t>(IndexFromAction(index, action));
    // new_index isn't box and isn't wall
    return !is_box[new_index] && board_static.at(new_index) != Element::kWall;
}

auto SokobanGameState::IsPushable(int index, Action action) const noexcept -> bool {
    if (!InBounds(index, action)) {
        return false;
    }
    const auto box_index = IndexFromAction(index, action);

    // No box at the query index
    if (!is_box[static_cast<std::size_t>(box_index)]) {
        return false;
    }

    // Check 1 past box in same direction
    return IsTraversible(box_index, action);
}

void SokobanGameState::Push(int index, Action action) noexcept {
    const auto box_index = IndexFromAction(index, action);
    MoveBox(box_index, action);
    MoveAgent(action);
}

// ---------------------------------------------------------------------------

}    // namespace sokoban
