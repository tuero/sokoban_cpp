#include <sokoban/sokoban.h>

#include <iostream>
#include <limits>

using namespace sokoban;

namespace {
void test_play() {
    const std::string board_str =
        "10|10|01|01|01|01|01|01|01|01|01|01|01|03|04|04|01|01|01|01|01|01|01|04|02|02|04|01|01|01|01|01|01|04|03|03|"
        "04|01|01|01|01|01|01|04|02|03|01|01|01|01|01|01|01|04|04|04|01|01|01|01|01|01|01|04|01|01|01|01|01|01|01|01|"
        "01|02|00|01|01|01|01|01|01|01|01|04|04|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01";
    SokobanGameState state(board_str);

    std::cout << sizeof(state) << std::endl;
    std::cout << state;
    std::cout << state.get_hash() << std::endl;
    for (const auto &bi : state.get_box_indices()) {
        std::cout << bi << ", ";
    }
    std::cout << std::endl;

    int action = -1;
    while (!state.is_solution()) {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        action = std::cin.get();
        switch (action) {
            case 'w':
                state.apply_action(Action::kUp);
                break;
            case 'd':
                state.apply_action(Action::kRight);
                break;
            case 's':
                state.apply_action(Action::kDown);
                break;
            case 'a':
                state.apply_action(Action::kLeft);
                break;
            default:
                return;
        }
        std::cout << state;
        std::cout << state.get_hash() << std::endl;
        for (auto const &bi : state.get_box_indices()) {
            std::cout << bi << ", ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }
}
}    // namespace

int main() {
    test_play();
}
