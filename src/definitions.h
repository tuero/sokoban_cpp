#ifndef SOKOBAN_DEFS_H_
#define SOKOBAN_DEFS_H_

#include <cassert>

namespace sokoban {

// Types of elements in the game
enum class Element {
    kAgent = 0,
    kWall = 1,
    kBox = 2,
    kGoal = 3,
    kEmpty = 4,
};
constexpr std::size_t kNumElements = 5;

// Possible actions for the agent to take
enum class Action {
    kUp = 0,
    kRight = 1,
    kDown = 2,
    kLeft = 3,
};
constexpr std::size_t kNumActions = 4;

// Bitfields to capture events
enum RewardCodes {
    kRewardBoxInGoal = 1 << 0,
    kRewardAllBoxesInGoal = 1 << 1,
};

}    // namespace sokoban

#endif    // SOKOBAN_DEFS_H_
