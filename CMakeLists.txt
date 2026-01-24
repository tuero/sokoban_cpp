cmake_minimum_required (VERSION 3.23)

project(sokoban)

option(BUILD_PYTHON_MODULE "Build the python library wrapper" OFF)

include(FetchContent)

# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(SOKOBAN_SOURCES
    src/definitions.h
    src/sokoban_base.cpp 
    src/sokoban_base.h 
)

# CPP library
add_library(sokoban STATIC ${SOKOBAN_SOURCES})
target_compile_features(sokoban PUBLIC cxx_std_17)
target_include_directories(sokoban PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Pybind11
if(BUILD_PYTHON_MODULE)
    message("Configuring Pybind11")
    FetchContent_Declare(pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11
        GIT_TAG master
        SYSTEM
    )
    FetchContent_MakeAvailable(pybind11)

    # Python module
    pybind11_add_module(pysokoban python/pysokoban.cpp)
    target_link_libraries(pysokoban PRIVATE sokoban)
    install(TARGETS pysokoban DESTINATION .)
endif()


# Build tests
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TESTS "Build the unit tests" OFF)
    if (${BUILD_TESTS})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
