#include <sokoban/sokoban.h>

#include <chrono>
#include <iostream>

using namespace sokoban;

using std::chrono::duration;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

namespace {
constexpr int NUM_STEPS = 1000000;
constexpr int MILLISECONDS_PER_SECOND = 1000;

void test_throughput() {
    const std::string board_str =
        "10|10|01|01|01|01|01|01|01|01|01|01|01|03|04|04|01|01|01|01|01|01|01|04|02|02|04|01|01|01|01|01|01|04|03|03|"
        "04|01|01|01|01|01|01|04|02|03|01|01|01|01|01|01|01|04|04|04|01|01|01|01|01|01|01|04|01|01|01|01|01|01|01|01|"
        "01|02|00|01|01|01|01|01|01|01|01|04|04|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01|01";
    const SokobanGameState state(board_str);
    std::vector<SokobanGameState> state_list;

    std::cout << "starting ..." << std::endl;

    const auto t1 = high_resolution_clock::now();
    state_list.reserve(NUM_STEPS * SokobanGameState::action_space_size());
    state_list.push_back(state);
    for (int i = 0; i < NUM_STEPS; ++i) {
        const SokobanGameState s = state_list[0];
        for (int dir = 0; dir < SokobanGameState::action_space_size(); ++dir) {
            SokobanGameState child = s;
            child.apply_action(static_cast<Action>(dir));
            state_list.push_back(child);
        }
        const std::vector<float> obs = state_list[0].get_observation();
        (void)obs;
        const uint64_t hash = state_list[0].get_hash();
        (void)hash;
    }
    const auto t2 = high_resolution_clock::now();
    const duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Total time for " << NUM_STEPS << " steps: " << ms_double.count() / MILLISECONDS_PER_SECOND
              << std::endl;
    std::cout << "Time per step :  " << ms_double.count() / MILLISECONDS_PER_SECOND / NUM_STEPS << std::endl;
}
}    // namespace

int main() {
    test_throughput();
}
