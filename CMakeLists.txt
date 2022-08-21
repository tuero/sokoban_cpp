project(sokoban)

cmake_minimum_required (VERSION 3.12)

# Set flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Testing
enable_testing()

set(SOKOBAN_SOURCES
    src/definitions.h
    src/sokoban_base.cpp 
    src/sokoban_base.h 
    src/util.cpp 
    src/util.h
)

# Build library
add_library(sokoban STATIC ${SOKOBAN_SOURCES})
target_compile_options(sokoban PRIVATE 
    -Wall -Wextra 
    $<$<CONFIG:RELEASE>:-O3> $<$<CONFIG:RELEASE>:-DNDEBUG>
    $<$<CONFIG:DEBUG>:-g> $<$<CONFIG:DEBUG>:-DDEBUG> $<$<CONFIG:DEBUG>:-pg>
)

target_include_directories(sokoban PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Build child targets
add_subdirectory(test)
