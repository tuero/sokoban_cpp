cmake_minimum_required (VERSION 3.12)

project(sokoban)

set(SOKOBAN_SOURCES
    src/definitions.h
    src/sokoban_base.cpp 
    src/sokoban_base.h 
    src/util.cpp 
    src/util.h
)

# Build library
add_library(sokoban STATIC ${SOKOBAN_SOURCES})
target_compile_features(sokoban PUBLIC cxx_std_17)
target_include_directories(sokoban PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_include_directories(sokoban SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/include/libnop/include)

# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
