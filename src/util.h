#ifndef SOKOBAN_UTIL_H
#define SOKOBAN_UTIL_H

#include <string>

#include "definitions.h"

namespace sokoban {
namespace util {

Board parse_board_str(const std::string &board_str);

}    // namespace util
}    // namespace sokoban

#endif    // SOKOBAN_UTIL_H