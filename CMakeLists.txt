cmake_minimum_required (VERSION 3.23)

project(sokoban VERSION 1.0.3)

option(BUILD_PYTHON_MODULE "Build the python library wrapper" OFF)

# Let sokoban_SHARED_LIBS override BUILD_SHARED_LIBS
if (DEFINED sokoban_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${sokoban_SHARED_LIBS}")
endif ()

# Required for python modules
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# CPP library
add_library(sokoban)
add_library(sokoban::sokoban ALIAS sokoban)
set_target_properties(sokoban PROPERTIES
                      VERSION ${sokoban_VERSION}
                      SOVERSION ${sokoban_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
)
target_include_directories(
    sokoban PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_compile_features(sokoban PUBLIC cxx_std_20)
target_sources(sokoban PRIVATE 
    include/sokoban/definitions.h 
    include/sokoban/sokoban.h 
    include/sokoban/sokoban_base.h 
    src/sokoban_base.cpp 
)


# Include the install rules if the user wanted them (included by default when top-level)
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)
option(sokoban_INCLUDE_PACKAGING "Include packaging rules for sokoban" "${is_top_level}")
if(sokoban_INCLUDE_PACKAGING)
    add_subdirectory(packaging)
endif()


# Pybind11
if(BUILD_PYTHON_MODULE)
    include(FetchContent)
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
