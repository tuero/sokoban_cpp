#ifndef SOKOBAN_DEFS_H
#define SOKOBAN_DEFS_H

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sokoban {

enum class ElementTypes {
    kAgent = 0,
    kWall = 1,
    kBox = 2,
    kGoal = 3,
};
constexpr int kNumElementTypes = 4;

const std::unordered_map<int, std::string> kElementsToStr{
    {0, " "},
    {0 | (1 << static_cast<int>(ElementTypes::kAgent)), "@"},
    {0 | (1 << static_cast<int>(ElementTypes::kWall)), "#"},
    {0 | (1 << static_cast<int>(ElementTypes::kBox)), "*"},
    {0 | (1 << static_cast<int>(ElementTypes::kGoal)), "$"},
    {0 | (1 << static_cast<int>(ElementTypes::kAgent)) | (1 << static_cast<int>(ElementTypes::kGoal)), "&"},
    {0 | (1 << static_cast<int>(ElementTypes::kBox)) | (1 << static_cast<int>(ElementTypes::kGoal)), "!"},
};

// Directions the interactions take place
enum Directions {
    kNoop = -1,
    kUp = 0,
    kRight = 1,
    kDown = 2,
    kLeft = 3,
};

// Agent can only take a subset of all directions
constexpr int kNumDirections = 4;
constexpr int kNumActions = 4;

enum RewardCodes {
    kRewardBoxInGoal = 1 << 0,
    kRewardAllBoxesInGoal = 1 << 1,
};

// actions to strings
const std::unordered_map<int, std::string> kActionsToString{
    {Directions::kUp, "up"},
    {Directions::kLeft, "left"},
    {Directions::kDown, "down"},
    {Directions::kRight, "right"},
};

// directions to offsets (col, row)
const std::unordered_map<int, std::pair<int, int>> kDirectionOffsets{
    {Directions::kUp, {0, -1}},   {Directions::kLeft, {-1, 0}}, {Directions::kDown, {0, 1}},
    {Directions::kRight, {1, 0}}, {Directions::kNoop, {0, 0}},
};

struct Board {
    Board() = delete;
    Board(int rows, int cols)
        : zorb_hash(0),
          rows(rows),
          cols(cols),
          agent_idx(-1),
          grid{std::vector<int>(rows * cols, 0), std::vector<int>(rows * cols, 0), std::vector<int>(rows * cols, 0),
               std::vector<int>(rows * cols, 0)} {}

    bool operator==(const Board &other) const {
        return grid == other.grid;
    }

    int &item(int channel, int index) {
        assert(index >= 0 && index < rows * cols);
        assert(channel >= 0 && channel < 4);
        return grid[channel][index];
    }

    int item(int channel, int index) const {
        assert(index >= 0 && index < rows * cols);
        assert(channel >= 0 && channel < 4);
        return grid[channel][index];
    }

    std::array<int, 4> get_channel_items(int index) const {
        assert(index >= 0 && index < rows * cols);
        return {grid[0][index], grid[1][index], grid[2][index], grid[3][index]};
    }

    uint64_t zorb_hash;
    int rows;
    int cols;
    int agent_idx;
    std::array<std::vector<int>, 4> grid;
};

}    // namespace sokoban

#endif    // SOKOBAN_DEFS_H
