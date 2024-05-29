#include <sokoban/sokoban.h>

#include <iostream>

using namespace sokoban;

void test_serialization() {
    const std::string board_str =
        "10|10|01|01|01|01|01|01|01|01|01|01|01|03|04|04|01|01|01|01|01|01|01|04|02|02|04|01|01|01|01|01|01|04|03|03|"
        "04|01|01|01|01|01|01|04|02|03|01|01|01|01|01|01|01|04|04|04|01|01|01|01|01|01|01|04|01|01|01|01|01|01|01|01|"
        "01|02|00|01|01|01|01|01|01|01|01|04|04|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01";
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);

    SokobanGameState state(params);
    state.apply_action(Action(1));

    std::vector<uint8_t> bytes = state.serialize();

    state.apply_action(Action(1));
    state.apply_action(Action(2));

    SokobanGameState state_copy(bytes);
    state_copy.apply_action(Action(1));
    state_copy.apply_action(Action(2));

    if (state != state_copy) {
        std::cout << "serialization error." << std::endl;
    }
    std::cout << state << std::endl;
    std::cout << state.get_hash() << std::endl;

    if (state.get_hash() != state_copy.get_hash()) {
        std::cout << "serialization error." << std::endl;
    }
    std::cout << state_copy << std::endl;
    std::cout << state_copy.get_hash() << std::endl;
}

int main() {
    test_serialization();
}
