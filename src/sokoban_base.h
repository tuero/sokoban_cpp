#ifndef SOKOBAN_BASE_H_
#define SOKOBAN_BASE_H_

#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "definitions.h"

namespace sokoban {

constexpr int SPRITE_WIDTH = 32;
constexpr int SPRITE_HEIGHT = 32;
constexpr int SPRITE_CHANNELS = 3;
constexpr int SPRITE_DATA_LEN_PER_ROW = SPRITE_WIDTH * SPRITE_CHANNELS;
constexpr int SPRITE_DATA_LEN = SPRITE_WIDTH * SPRITE_HEIGHT * SPRITE_CHANNELS;

// Game parameter can be boolean, integral or floating point
using GameParameter = std::variant<bool, int, float, std::string>;
using GameParameters = std::unordered_map<std::string, GameParameter>;

// Default game parameters
static const GameParameters kDefaultGameParams{
    {"obs_show_ids", GameParameter(false)},    // Flag to show object ids in observation instead of binary channels
    {"rng_seed", GameParameter(0)},            // Seed for anything that uses the rng
    {"game_board_str", GameParameter(std::string("3|3|00|01|01|02|01|01|03|01|01"))},    // Game board string
};

// Shared global state information relevant to all states for the given game
struct SharedStateInfo {
    SharedStateInfo(GameParameters params);
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    GameParameters params;                      // Copy of game parameters for state resetting
    bool obs_show_ids;                          // Flag to show object IDs (currently not used)
    int rng_seed;                               // Seed
    std::string game_board_str;                 // String representation of the starting state
    std::mt19937 gen;                           // Generator for RNG
    std::uniform_int_distribution<int> dist;    // Random int distribution
    std::vector<uint64_t> zrbht;                // Zobrist hashing table
    std::size_t rows = 0;                       // Rows of the common board
    std::size_t cols = 0;                       // Cols of the common board
    std::vector<Element> board_static;          // static elements (wall, goal, empty)
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    auto operator==(const SharedStateInfo &other) const noexcept -> bool;
};

// Information specific for the current game state
struct LocalState {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    uint64_t zorb_hash = 0;
    uint8_t current_reward = 0;    // Reward for the current game state
    uint64_t reward_signal = 0;    // Signal for external information about events
    std::size_t agent_idx = 0;
    std::vector<std::size_t> box_indices;               // Index each box resides, where vector index is the box id
    std::unordered_set<std::size_t> box_indices_set;    // Same as above but in set for faster querying
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    auto operator==(const LocalState &other) const noexcept -> bool;
};

// Game state
class SokobanGameState {
public:
    SokobanGameState() = delete;
    SokobanGameState(const GameParameters &params = kDefaultGameParams);

    bool operator==(const SokobanGameState &other) const noexcept;

    /**
     * Reset the environment to the state as given by the GameParameters
     */
    void reset();

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(Action action) noexcept;

    /**
     * Check if the state is in the solution state (agent inside exit).
     * @return True if terminal, false otherwise
     */
    [[nodiscard]] auto is_solution() const noexcept -> bool;

    /**
     * Get the legal actions which can be applied in the state.
     * @return vector containing each actions available
     */
    [[nodiscard]] auto legal_actions() const noexcept -> std::vector<Action>;

    /**
     * Get the legal actions which can be applied in the state, and store in the given vector.
     * @note Use when wanting to reuse a pre-allocated vector
     * @param actions The vector to store the available actions in
     */
    void legal_actions(std::vector<Action> &actions) const noexcept;

    /**
     * Get the number of possible actions
     * @return Count of possible actions
     */
    [[nodiscard]] constexpr static auto action_space_size() noexcept -> std::size_t {
        return kNumActions;
    }

    /**
     * Get the shape the observations should be viewed as.
     * @return vector indicating observation CHW
     */
    [[nodiscard]] auto observation_shape() const noexcept -> std::array<std::size_t, 3>;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents element at position
     */
    [[nodiscard]] auto get_observation() const noexcept -> std::vector<float>;

    /**
     * Get a flat representation of the current state observation, and store in the given vector.
     * @note Use when wanting to reuse a pre-allocated vector
     * The observation should be viewed as the shape given by observation_shape(), where 1 represents the element at the
     * given position.
     * @param obs Vector to store the observation in
     */
    void get_observation(std::vector<float> &obs) const noexcept;

    /**
     * Get the shape the image should be viewed as.
     * @return array indicating observation HWC
     */
    [[nodiscard]] auto image_shape() const noexcept -> std::array<std::size_t, 3>;

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
     * Get all ids for unsolved boxes
     * @return set of ids
     */
    [[nodiscard]] auto get_unsolved_box_ids() const noexcept -> std::unordered_set<int>;

    /**
     * Get all ids for solved boxes
     * @return set of ids
     */
    [[nodiscard]] auto get_solved_box_ids() const noexcept -> std::unordered_set<int>;

    /**
     * Get all ids for all boxes
     * @return set of ids
     */
    [[nodiscard]] auto get_all_box_ids() const noexcept -> std::unordered_set<int>;

    /**
     * Get all indices of empty goals
     * @return vector of indicies
     */
    [[nodiscard]] auto get_empty_goal_indices() const noexcept -> std::unordered_set<std::size_t>;

    /**
     * Get all indices of solved goals
     * @return vector of indicies
     */
    [[nodiscard]] auto get_solved_goal_indices() const noexcept -> std::unordered_set<std::size_t>;

    /**
     * Get all indices of all goals
     * @return vector of indicies
     */
    [[nodiscard]] auto get_all_goal_indices() const noexcept -> std::unordered_set<std::size_t>;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    [[nodiscard]] auto get_agent_index() const noexcept -> std::size_t;

    /**
     * Get the index of a given box id
     * @note throws an invalid argument error if box_id does not exist
     * @param index The box id to query
     * @return the index
     */
    [[nodiscard]] auto get_box_index(int box_id) const -> std::size_t;

    // All possible actions
    static const std::vector<Action> ALL_ACTIONS;

    friend std::ostream &operator<<(std::ostream &os, const SokobanGameState &state);

private:
    [[nodiscard]] std::size_t IndexFromAction(std::size_t index, Action action) const noexcept;
    [[nodiscard]] bool InBounds(std::size_t index, Action action) const noexcept;
    [[nodiscard]] bool IsTraversible(std::size_t index, Action action) const noexcept;
    [[nodiscard]] bool IsPushable(std::size_t index, Action action) const noexcept;
    void MoveAgent(Action action) noexcept;
    void MoveBox(std::size_t box_index, Action action) noexcept;
    void Push(std::size_t index, Action action) noexcept;

    std::shared_ptr<SharedStateInfo> shared_state_ptr;
    LocalState local_state;
};

}    // namespace sokoban

#endif    // SOKOBAN_BASE_H_
