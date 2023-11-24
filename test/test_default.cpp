#include <sokoban/sokoban.h>

#include <iostream>

using namespace sokoban;

void test_default() {
    const GameParameters params = kDefaultGameParams;
    const SokobanGameState state(params);
    std::cout << state.is_solution() << std::endl;
}

int main() {
    test_default();
}
