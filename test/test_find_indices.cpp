#include <sokoban/sokoban.h>

#include <iostream>
#include <unordered_set>
#include <vector>

using namespace sokoban;

void test_indices() {
    // const std::string board_str = "|10|10|02|02|02|02|02|02|02|02|02|02|02|01|01|03|01|01|02|02|02|02|02|01|03|03|04|01|02|02|02|02|02|01|01|02|01|01|01|02|02|02|02|04|04|02|02|02|02|02|02|02|02|00|01|02|02|02|02|02|02|02|02|03|04|02|02|02|02|02|02|02|02|01|01|02|02|02|02|02|02|02|02|01|02|02|02|02|02|02|02|02|02|02|02|02|02|02|02|02|02|02|";
    // GameParameters params = kDefaultGameParams;
    // params["game_board_str"] = GameParameter(board_str);
    // SokobanGameState state(params);

    // // test 1
    // {
    //     std::unordered_set<HiddenCellType> element_set = {HiddenCellType::kDiamond, HiddenCellType::kExitClosed, HiddenCellType::kExitOpen};
    //     std::vector<int> indices = state.get_indices_flat(element_set);
    //     std::cout << "Expected size: 2" << std::endl;
    //     std::cout << "Result: " << indices.size() << std::endl;
    //     for (auto const & idx : indices) {
    //         std::cout << idx << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // test 1
    // {
    //     std::unordered_set<HiddenCellType> element_set = {
    //         HiddenCellType::kDiamond, HiddenCellType::kExitClosed, HiddenCellType::kExitOpen,
    //         HiddenCellType::kKeyRed, HiddenCellType::kGateRedClosed, HiddenCellType::kGateRedOpen,
    //         HiddenCellType::kKeyBlue, HiddenCellType::kGateBlueClosed, HiddenCellType::kGateBlueOpen,
    //         HiddenCellType::kKeyGreen, HiddenCellType::kGateGreenClosed, HiddenCellType::kGateGreenOpen,
    //         HiddenCellType::kKeyYellow, HiddenCellType::kGateYellowClosed, HiddenCellType::kGateYellowOpen, 
    //     };
    //     std::vector<int> indices = state.get_indices_flat(element_set);
    //     std::cout << "Expected size: 10" << std::endl;
    //     std::cout << "Result: " << indices.size() << std::endl;
    //     for (auto const & idx : indices) {
    //         std::cout << idx << " ";
    //     }
    //     std::cout << std::endl;
    // }
}


int main() {
    test_indices();
}