cmake_minimum_required(VERSION 3.20)

# WidgetDemo - the toggleable widgets and combo box demo for tui++.
#
# Builds ONE component tree (a Frame with a View MenuBar over a panel of
# ToggleButtons/CheckBoxes/RadioButtons and ComboBoxes) and runs it on either
# terminal backend:
#
#     WidgetDemo            text screen  (cell-based escape sequences)
#     WidgetDemo text       same as the default
#     WidgetDemo sixel      sixel screen (pixel-based graphics)
#
# Mouse/keyboard: click toggles; Tab moves the focus; the combo boxes are
# operated with the arrow keys (dropdown), Enter (pick) and typing (lookup;
# editable combo).
project(WidgetDemo)

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
