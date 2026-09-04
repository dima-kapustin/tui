cmake_minimum_required(VERSION 3.20)
project(tui)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions("-DUNICODE")
endif()

add_subdirectory(tui++)
add_subdirectory(tui++tests)
add_subdirectory(MenuBarDemo)
add_subdirectory(PerfProbe)
