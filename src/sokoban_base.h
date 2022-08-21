#ifndef SOKOBAN_BASE_H
#define SOKOBAN_BASE_H

#include <array>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "definitions.h"
#include "util.h"

namespace sokoban {

// Game parameter can be boolean, integral or floating point
using GameParameter = std::variant<bool, int, float, std::string>;
using GameParameters = std::unordered_map<std::string, GameParameter>;

// Default game parameters
static const GameParameters kDefaultGameParams{
    {"obs_show_ids", GameParameter(false)},    // Flag to show object ids in observation instead of binary channels
    {"rng_seed", GameParameter(0)},            // Seed for anything that uses the rng
    {"game_board_str", GameParameter(std::string("|3|3|00|01|01|03|01|01|04|01|01|"))},    // Game board string
};

// Shared global state information relevant to all states for the given game
struct SharedStateInfo {
    SharedStateInfo(const GameParameters &params)
        : params(params),
          obs_show_ids(std::get<bool>(params.at("obs_show_ids"))),
          rng_seed(std::get<int>(params.at("rng_seed"))),
          game_board_str(std::get<std::string>(params.at("game_board_str"))),
          gen(rng_seed),
          dist(0) {}
    GameParameters params;                      // Copy of game parameters for state resetting
    bool obs_show_ids;                          // Flag to show object IDs (currently not used)
    int rng_seed;                               // Seed
    std::string game_board_str;                 // String representation of the starting state
    std::mt19937 gen;                           // Generator for RNG
    std::uniform_int_distribution<int> dist;    // Random int distribution
    std::unordered_map<int, uint64_t> zrbht;    // Zobrist hashing table
};

// Information specific for the current game state
struct LocalState {
    LocalState()
        : current_reward(0),
          reward_signal(0) {}

    uint8_t current_reward;       // Reward for the current game state
    uint64_t reward_signal;       // Signal for external information about events
};

// Game state
class SokobanGameState {
public:
    SokobanGameState(const GameParameters &params = kDefaultGameParams);

    bool operator==(const SokobanGameState &other) const;

    /**
     * Reset the environment to the state as given by the GameParameters
     */
    void reset();

    /**
     * Apply the action to the current state, and set the reward and signals.
     * @param action The action to apply, should be one of the legal actions
     */
    void apply_action(int action);

    /**
     * Check if the state is in the solution state (agent inside exit).
     * @return True if terminal, false otherwise
     */
    bool is_solution() const;

    /**
     * Get the legal actions which can be applied in the state.
     * @return vector containing each actions available
     */
    std::vector<int> legal_actions() const;

    /**
     * Get the shape the observations should be viewed as.
     * @return vector indicating observation CHW
     */
    std::array<int, 3> observation_shape() const;

    /**
     * Get a flat representation of the current state observation.
     * The observation should be viewed as the shape given by observation_shape().
     * @return vector where 1 represents object at position
     */
    std::vector<float> get_observation() const;

    /**
     * Get the current reward signal as a result of the previous action taken.
     * @return bit field representing events that occured
     */
    uint64_t get_reward_signal() const;

    /**
     * Get the hash representation for the current state.
     * @return hash value
     */
    uint64_t get_hash() const;

    /**
     * Get all ids for unsolved boxes
     * @return set of ids
     */
    std::unordered_set<int> get_unsolved_box_ids() const;

    /**
     * Get all ids for all boxes
     * @return set of ids
     */
    std::unordered_set<int> get_all_box_ids() const;

    /**
     * Get all indices of empty goals
     * @return vector of indicies
     */
    std::unordered_set<int> get_empty_goals() const;

    /**
     * Get all indices of all goals
     * @return vector of indicies
     */
    std::unordered_set<int> get_all_goals() const;

    /**
     * Get the agent index position, even if in exit
     * @return Agent index
     */
    int get_agent_index() const;

    /**
     * Get the index of a given box id
     * @param index The box id to query
     * @return the index
     */
    int get_box_index(int box_id) const;

    /**
     * Observation shape if rows and cols are given.
     * @param rows Number of rows
     * @param cols Number of cols
     * @return vector indicating observation CHW
     */
    static std::array<int, 3> observation_shape(int rows, int cols) {
        return {kNumElementTypes, rows, cols};
    }

    friend std::ostream &operator<<(std::ostream &os, const SokobanGameState &state);

private:
    int IndexFromAction(int index, int action) const;
    bool InBounds(int index, int action = Directions::kNoop) const;
    bool IsTraversible(int index, int action = Directions::kNoop) const;
    bool IsPushable(int index, int action = Directions::kNoop) const;
    void MoveItem(int channel, int index, int action);
    void Push(int index, int action);

    std::shared_ptr<SharedStateInfo> shared_state_ptr;
    Board board;
    LocalState local_state;
};

}    // namespace sokoban

#endif    // SOKOBAN_BASE_H