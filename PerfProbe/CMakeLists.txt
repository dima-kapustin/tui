cmake_minimum_required(VERSION 3.20)

# PerfProbe - measures the text screen's emission path.
#
# Replaces stdout with a counting sink and replays MenuBarDemo-style
# scenarios (full refresh, region repaints, popup open/close, multi-region
# passes) reporting, per scenario, the bytes emitted, the flush count and
# the CPU time. Used to evaluate emission-strategy proposals (shadow-diffed
# rows, single flush per pass) with measured numbers.
#
# Self-contained like MenuBarDemo: standalone or via the tui super-build.
project(PerfProbe)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions("-DUNICODE")
endif()

if(NOT TARGET tui++)
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
