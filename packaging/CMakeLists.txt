include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (NOT DEFINED sokoban_INSTALL_CMAKEDIR)
   set(sokoban_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/sokoban"
       CACHE STRING "Path to sokoban CMake files")
endif ()

install(TARGETS sokoban EXPORT sokoban_Targets
        RUNTIME COMPONENT sokoban_Runtime
        LIBRARY COMPONENT sokoban_Runtime
        NAMELINK_COMPONENT sokoban_Development
        ARCHIVE COMPONENT sokoban_Development
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(DIRECTORY "${sokoban_SOURCE_DIR}/include/" 
        TYPE INCLUDE
        COMPONENT sokoban_Development)

if (BUILD_SHARED_LIBS)
    set(type shared)
    message(STATUS "Building sokoban shared lib")
else ()
    set(type static)
    message(STATUS "Building sokoban static lib")
endif ()

install(EXPORT sokoban_Targets
        DESTINATION "${sokoban_INSTALL_CMAKEDIR}"
        NAMESPACE sokoban::
        FILE sokoban-${type}-targets.cmake
        COMPONENT sokoban_Development)

write_basic_package_version_file(
    sokobanConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)

install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/sokobanConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/sokobanConfigVersion.cmake"
        DESTINATION "${sokoban_INSTALL_CMAKEDIR}"
        COMPONENT sokoban_Development)
