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

## Installing Python Bindings
```shell
git clone https://github.com/tuero/sokoban_cpp.git
pip install ./sokoban_cpp
```

If you get a `GLIBCXX_3.4.XX not found` error at runtime, 
then you most likely have an older `libstdc++` in your virtual environment `lib/` 
which is taking presidence over your system version.
Either correct your `$PATH`, or update your virtual environment `libstdc++`.

For example, if using anaconda
```shell
conda install conda-forge::libstdcxx-ng
```

## Level Format
Levels are expected to be formatted as `|` delimited strings, where the first 2 entries are the rows/columns of the level,
then the following `rows * cols` entries are the element ID (see `Element` in `definitions.h`).

## Notice
The image tile assets under `/tiles/` are taken from [Rocks'n'Diamonds](https://www.artsoft.org/). 
A copy of the license for those materials can be found alongside the assets.
