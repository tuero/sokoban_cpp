#ifndef SOKOBAN_BASE_H_
#define SOKOBAN_BASE_H_

#include <array>
#include <cstdint>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "definitions.h"

namespace sokoban {

constexpr int SPRITE_WIDTH = 32;
constexpr int SPRITE_HEIGHT = 32;
constexpr int SPRITE_CHANNELS = 3;
constexpr int SPRITE_DATA_LEN_PER_ROW = SPRITE_WIDTH * SPRITE_CHANNELS;
constexpr int SPRITE_DATA_LEN = SPRITE_WIDTH * SPRITE_HEIGHT * SPRITE_CHANNELS;

// Game state
class SokobanGameState {
public:
    // Internal use for packing/unpacking with pybind11 pickle
    struct InternalState {
        int rows;
        int cols;
        int agent_idx;
        uint64_t hash;
        uint64_t reward_signal;
        std::vector<int> board_static;
        std::vector<bool> is_box;
    };

    SokobanGameState() = delete;
    SokobanGameState(const std::string& board_str);
    SokobanGameState(InternalState&& internal_state);

    auto operator==(const SokobanGameState& other) const noexcept -> bool;
    auto operator!=(const SokobanGameState& other) const noexcept -> bool;

    static inline std::string name = "sokoban";

    /**
     * Check if the given action is valid.
     * @param action Action to check
     * @return True if action is valid, false otherwise
     */
    [[nodiscard]] constexpr static auto is_valid_action(Action action) -> bool {
        return static_cast<int>(action) >= 0 && static_cast<int>(action) < static_cast<int>(kNumActions);
    }

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(Action action);

    /**
     * Get the number of possible actions
     * @return Count of possible actions
     */
    [[nodiscard]] constexpr static auto action_space_size() noexcept -> int {
        return kNumActions;
    }

    /**
     * Check if the state is in the solution state (agent inside exit).
     * @return True if terminal, false otherwise
     */
    [[nodiscard]] auto is_solution() const noexcept -> bool;

    /**
     * Get the shape the observations should be viewed as.
     * @param compact True to use compact representation. 4 channels used for agent, wall, box, and goal. If agent/box
     * on goal, two channels have true values. If false, then 6 channels are used (agent, wall, box, goal, agent on
     * goal, box on goal)
     * @return vector indicating observation CHW
     */
    [[nodiscard]] auto observation_shape(bool compact = true) const noexcept -> std::array<int, 3>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @param bool True to use compact representation
     * @return vector where 1 represents element at position
     */
    [[nodiscard]] auto get_observation(bool compact = true) const noexcept -> std::vector<float>;

    /**
     * Get the shape the image should be viewed as.
     * @return array indicating observation HWC
     */
    [[nodiscard]] auto image_shape() const noexcept -> std::array<int, 3>;

    /**
     * Get the flat (HWC) image representation of the current state
     * @return flattened byte vector represending RGB values (HWC)
     */
    [[nodiscard]] auto to_image() const noexcept -> std::vector<uint8_t>;

    /**
     * Get the current reward signal as a result of the previous action taken.
     * @return bit field representing events that occured
     */
    [[nodiscard]] auto get_reward_signal() const noexcept -> uint64_t;

    /**
     * Get the hash representation for the current state.
     * @return hash value
     */
    [[nodiscard]] auto get_hash() const noexcept -> uint64_t;

    /**
     * Get all indices of boxes
     * @return vector of indicies
     */
    [[nodiscard]] auto get_box_indices() const noexcept -> std::vector<int>;

    /**
     * Get all indices of empty goals
     * @return vector of indicies
     */
    [[nodiscard]] auto get_empty_goal_indices() const noexcept -> std::vector<int>;

    /**
     * Get all indices of solved goals
     * @return vector of indicies
     */
    [[nodiscard]] auto get_solved_goal_indices() const noexcept -> std::vector<int>;

    /**
     * Get all indices of all goals
     * @return vector of indicies
     */
    [[nodiscard]] auto get_all_goal_indices() const noexcept -> std::vector<int>;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    [[nodiscard]] auto get_agent_index() const noexcept -> int;

    friend std::ostream& operator<<(std::ostream& os, const SokobanGameState& state);

    [[nodiscard]] auto pack() const -> InternalState {
        std::vector<int> _board_static;
        _board_static.reserve(board_static.size());
        for (const auto& el : board_static) {
            _board_static.push_back(static_cast<int>(el));
        }
        return {.rows = rows,
                .cols = cols,
                .agent_idx = agent_idx,
                .hash = zorb_hash,
                .reward_signal = reward_signal,
                .board_static = _board_static,
                .is_box = is_box};
    }

private:
    void _get_observation_non_compact(std::vector<float>& obs) const noexcept;
    void _get_observation_compact(std::vector<float>& obs) const noexcept;
    [[nodiscard]] int IndexFromAction(int index, Action action) const noexcept;
    [[nodiscard]] bool InBounds(int index, Action action) const noexcept;
    [[nodiscard]] bool IsTraversible(int index, Action action) const noexcept;
    [[nodiscard]] bool IsPushable(int index, Action action) const noexcept;
    void MoveAgent(Action action) noexcept;
    void MoveBox(int box_index, Action action) noexcept;
    void Push(int index, Action action) noexcept;

    int rows = -1;
    int cols = -1;
    int agent_idx = -1;
    uint64_t zorb_hash = 0;
    uint64_t reward_signal = 0;
    std::vector<Element> board_static;
    std::vector<bool> is_box;
};

}    // namespace sokoban

template <>
struct std::formatter<sokoban::SokobanGameState> : std::formatter<std::string> {
    auto format(sokoban::SokobanGameState s, format_context& ctx) const {
        std::ostringstream oss;
        oss << s;
        return formatter<string>::format(std::format("{}", oss.str()), ctx);
    }
};

#endif    // SOKOBAN_BASE_H_
