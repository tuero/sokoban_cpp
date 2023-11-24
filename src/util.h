#ifndef SOKOBAN_UTIL_H_
#define SOKOBAN_UTIL_H_

#include <string>

#include "definitions.h"
#include "sokoban_base.h"

namespace sokoban {

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

void parse_board_str(const std::string &board_str, LocalState &local_state,
                     std::shared_ptr<SharedStateInfo> shared_state);

}    // namespace sokoban

#endif    // SOKOBAN_UTIL_H_
