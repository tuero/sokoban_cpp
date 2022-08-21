#include <sokoban/sokoban.h>

#include <chrono>
#include <iostream>

using namespace sokoban;

using std::chrono::high_resolution_clock;
using std::chrono::duration;
using std::chrono::milliseconds;

void test_throughput() {
    const std::string board_str = "10|10|01|01|01|01|01|01|01|01|01|01|01|03|04|04|01|01|01|01|01|01|01|04|02|02|04|01|01|01|01|01|01|04|03|03|04|01|01|01|01|01|01|04|02|03|01|01|01|01|01|01|01|04|04|04|01|01|01|01|01|01|01|04|01|01|01|01|01|01|01|01|01|02|00|01|01|01|01|01|01|01|01|04|04|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01";
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);
    SokobanGameState state(params);
    int NUM_STEPS = 1000000;
    std::vector<SokobanGameState> state_list;

    std::cout << "starting ..." << std::endl;

    auto t1 = high_resolution_clock::now();
    state_list.reserve(NUM_STEPS * 5);
    state_list.push_back(state);
    for (int i = 0; i < NUM_STEPS; ++i) {
        SokobanGameState s = state_list[0];
        for (const auto dir : state_list[0].legal_actions()) {
            SokobanGameState child = s;
            child.apply_action(dir);
            state_list.push_back(child);
        }
        std::vector<float> obs = state_list[0].get_observation();
        (void)obs;
        uint64_t hash = state_list[0].get_hash();
        (void)hash;
    }
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Total time for " << NUM_STEPS << " steps: " << ms_double.count() / 1000 << std::endl;
    std::cout << "Time per step :  " << ms_double.count() / 1000 / NUM_STEPS << std::endl;
}

int main() {
    test_throughput();
}