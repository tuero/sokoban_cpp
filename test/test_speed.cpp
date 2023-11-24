#include <sokoban/sokoban.h>

#include <chrono>
#include <iostream>

using namespace sokoban;

using std::chrono::duration;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

constexpr int NUM_STEPS = 10000;
constexpr int MILLISECONDS_PER_SECOND = 1000;

void test_speed() {
    const std::string board_str =
        "10|10|01|01|01|01|01|01|01|01|01|01|01|03|04|04|01|01|01|01|01|01|01|04|02|02|04|01|01|01|01|01|01|04|03|03|"
        "04|01|01|01|01|01|01|04|02|03|01|01|01|01|01|01|01|04|04|04|01|01|01|01|01|01|01|04|01|01|01|01|01|01|01|01|"
        "01|02|00|01|01|01|01|01|01|01|01|04|04|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01";
    GameParameters params = kDefaultGameParams;
    params["game_board_str"] = GameParameter(board_str);
    SokobanGameState state(params);

    std::cout << "starting ..." << std::endl;

    auto t1 = high_resolution_clock::now();
    for (int i = 0; i < NUM_STEPS; ++i) {
        state.apply_action(Action::kUp);
        const std::vector<float> obs = state.get_observation();
        (void)obs;
        const uint64_t hash = state.get_hash();
        (void)hash;
    }
    auto t2 = high_resolution_clock::now();
    const duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Total time for " << NUM_STEPS << " steps: " << ms_double.count() / MILLISECONDS_PER_SECOND
              << std::endl;
    std::cout << "Time per step :  " << ms_double.count() / MILLISECONDS_PER_SECOND / NUM_STEPS << std::endl;
}

int main() {
    test_speed();
}
