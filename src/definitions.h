#ifndef SOKOBAN_DEFS_H_
#define SOKOBAN_DEFS_H_

#include <array>
#include <cassert>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace sokoban {

// Types of elements in the game
enum class Element {
    kAgent = 0,
    kWall = 1,
    kBox = 2,
    kGoal = 3,
    kEmpty = 4,
};
constexpr int kNumElements = 5;
constexpr int kNumChannelsCompact = 4;
constexpr int kNumChannels = 7;

constexpr int ChannelAgentOnGoal = 5;
constexpr int ChannelBoxOnGoal = 6;

// Possible actions for the agent to take
enum class Action {
    kUp = 0,
    kRight = 1,
    kDown = 2,
    kLeft = 3,
};
constexpr int kNumActions = 4;

// Bitfields to capture events
enum RewardCodes {
    kRewardBoxInGoal = 1 << 0,
    kRewardAllBoxesInGoal = 1 << 1,
};

template <class E>
constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

// actions to strings
const std::unordered_map<Action, std::string> kActionToString{
    {Action::kUp, "up"},
    {Action::kLeft, "left"},
    {Action::kDown, "down"},
    {Action::kRight, "right"},
};

// Typedefs
using Offset = std::pair<int, int>;

// Direction to offsets (col, row)
// Not necessarily best practice but faster than map lookups
constexpr std::array<Offset, kNumActions> kActionOffsets{{
    {0, -1},    // Direction::kUp
    {1, 0},     // Direction::kRight
    {0, 1},     // Direction::kDown
    {-1, 0},    // Direction::kLeft
}};

const std::unordered_map<int, std::string> kElementToStr{
    {0, " "},
    {0 | (1 << to_underlying(Element::kAgent)), "@"},
    {0 | (1 << to_underlying(Element::kWall)), "#"},
    {0 | (1 << to_underlying(Element::kBox)), "*"},
    {0 | (1 << to_underlying(Element::kGoal)), "$"},
    {0 | (1 << to_underlying(Element::kAgent)) | (1 << to_underlying(Element::kGoal)), "&"},
    {0 | (1 << to_underlying(Element::kBox)) | (1 << to_underlying(Element::kGoal)), "!"},
};

}    // namespace sokoban

#endif    // SOKOBAN_DEFS_H_
