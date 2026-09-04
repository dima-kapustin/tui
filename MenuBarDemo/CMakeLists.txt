cmake_minimum_required(VERSION 3.20)

# MenuBarDemo - the interactive MenuBar demo for tui++.
#
# Builds ONE component tree (a Frame with a File/Edit MenuBar over a content
# panel) and runs it on either terminal backend:
#
#     MenuBarDemo            text screen  (cell-based escape sequences)
#     MenuBarDemo text       same as the default
#     MenuBarDemo sixel      sixel screen (pixel-based graphics)
#
# This is a self-contained CMake project: when configured on its own it pulls
# in the sibling tui++ library automatically:
#
#     cmake -S MenuBarDemo -B MenuBarDemo/build
#     cmake --build MenuBarDemo/build -- -j8
#     MenuBarDemo/build/MenuBarDemo [text|sixel]
#
# It can also be built as part of the tui super-build (see the top-level
# CMakeLists.txt), where the tui++ target already exists.
project(MenuBarDemo)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions("-DUNICODE")
endif()

if(NOT TARGET tui++)
    # Standalone configuration: build the tui++ library from the sibling
    # directory. In the tui super-build the target already exists and this
    # block is skipped.
    add_subdirectory(../tui++ "${CMAKE_CURRENT_BINARY_DIR}/tui++")
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    tui++
)

#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
