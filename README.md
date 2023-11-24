# sokoban_cpp

A C++ implementation of Sokoban. 

## Include to Your Project: CMake

### FetchContent
```shell
include(FetchContent)
# ...
FetchContent_Declare(sokoban
    GIT_REPOSITORY https://github.com/tuero/sokoban_cpp.git
    GIT_TAG master
)

# make available
FetchContent_MakeAvailable(sokoban)
link_libraries(sokoban)
```

### Git Submodules
```shell
# assumes project is cloned into external/sokoban_cpp
add_subdirectory(external/sokoban_cpp)
link_libraries(sokoban)
```

## Level Format
Levels are expected to be formatted as `|` delimited strings, where the first 2 entries are the rows/columns of the level,
then the following `rows * cols` entries are the element ID (see `Element` in `definitions.h`).
