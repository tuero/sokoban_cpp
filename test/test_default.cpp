#include <sokoban/sokoban.h>

#include <iostream>

using namespace sokoban;

void test_default() {
    GameParameters params = kDefaultGameParams;
    SokobanGameState state(params);
    std::cout << state.is_solution() << std::endl;
}

int main() {
    test_default();
}