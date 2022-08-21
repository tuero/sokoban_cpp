#include <sokoban/sokoban.h>

#include <iostream>

using namespace sokoban;
using namespace sokoban::util;


void test_play() {
    const std::string board_str = "10|10|01|01|01|01|01|01|01|01|01|01|01|03|04|04|01|01|01|01|01|01|01|04|02|02|04|01|01|01|01|01|01|04|03|03|04|01|01|01|01|01|01|04|02|03|01|01|01|01|01|01|01|04|04|04|01|01|01|01|01|01|01|04|01|01|01|01|01|01|01|01|01|02|00|01|01|01|01|01|01|01|01|04|04|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01";
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);
    SokobanGameState state(params);

    std::cout << state;
    std::cout << state.get_hash() << std::endl;
    std::vector<int> box_ids = state.get_unsolved_box_ids();
    for (auto const & bi : box_ids) {
        std::cout << "(" << bi << ", " << state.get_box_index(bi) << "), ";
    }
    std::cout << std::endl;
    
    int action;
    while (!state.is_solution()) {
        std::cin >> action;
        switch (action) {
            case 8:
                state.apply_action(0);
                break;
            case 6:
                state.apply_action(1);
                break;
            case 2:
                state.apply_action(2);
                break;
            case 4:
                state.apply_action(3);
                break;
            default:
                break;
        }
        std::cout << state;
        std::cout << state.get_hash() << std::endl;
        std::vector<int> box_ids = state.get_unsolved_box_ids();
        for (auto const & bi : box_ids) {
            std::cout << "(" << bi << ", " << state.get_box_index(bi) << "), ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }
}


int main() {
    test_play();
}