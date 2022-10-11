#include <sokoban/sokoban.h>

#include <iostream>
#include <unordered_map>

using namespace sokoban;
using namespace sokoban::util;

const std::unordered_map<std::string, int> ActionMap{
    {"w", 0},
    {"d", 1},
    {"s", 2},
    {"a", 3},  
};


void test_play() {
    std::string board_str;
    std::cout << "Enter board str: ";
    std::cin >> board_str;
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);
    SokobanGameState state(params);

    std::cout << state;
    std::cout << state.get_hash() << std::endl;
    auto box_ids = state.get_unsolved_box_ids();
    for (auto const & bi : box_ids) {
        std::cout << "(" << bi << ", " << state.get_box_index(bi) << "), ";
    }
    std::cout << std::endl;
    
    std::string action_str;
    while (!state.is_solution()) {
        std::cin >> action_str;
        state.apply_action(ActionMap.find(action_str) == ActionMap.end() ? 0 : ActionMap.at(action_str));
        std::cout << state;
        std::cout << state.get_hash() << std::endl;
        std::cout << state.is_deadlocked() << std::endl;
        std::cout << "legal non-deadlocking actions: ";
        for (auto const & a : state.legal_actions_no_deadlocks()) {
            std::cout << a << " ";
        }
        std::cout << std::endl;
        auto box_ids = state.get_unsolved_box_ids();
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