cmake_minimum_required(VERSION 3.20)

# ScrollPaneDemo - exercises the JScrollPane/JViewport/JScrollBar machinery
# of tui++ on the text backend.
#
#     ScrollPaneDemo             text screen (cell-based escape sequences)
#     ScrollPaneDemo --log-events  ... with the event/resize/graphics log on
#
# The window shows one ScrollPane whose viewport hosts a LogView: a large
# synthetic log (2000 rows x 132 columns) that implements Swing's Scrollable
# contract, so both scroll bars appear and can be driven with the mouse
# (arrows, track, thumb drag, wheel) and the keyboard. The status line under
# the pane mirrors the scroll bar models' values.
#
# This is a self-contained CMake project: when configured on its own it pulls
# in the sibling tui++ library automatically (see MenuBarDemo for the same
# arrangement). It can also be built as part of the tui super-build.
project(ScrollPaneDemo)

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

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
