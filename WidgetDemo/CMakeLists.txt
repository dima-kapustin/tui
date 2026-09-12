cmake_minimum_required(VERSION 3.20)

# WidgetDemo - the widgets, context menus and dialogs demo for tui++.
#
# Builds ONE component tree (a Frame with a File/View/Dialogs MenuBar over a
# panel of ToggleButtons/CheckBoxes/RadioButtons, a Switch, a shadowed Button
# and ComboBoxes, plus the comment TextField and its standard context menu) and
# runs it on either terminal backend:
#
#     WidgetDemo            text screen  (cell-based escape sequences)
#     WidgetDemo text       same as the default
#     WidgetDemo sixel      sixel screen (pixel-based graphics)
#
# Mouse/keyboard: click toggles; Tab moves the focus; the combo boxes are
# operated with the arrow keys (dropdown), Enter (pick) and typing (lookup;
# editable combo); the right button (or Shift+F10) opens the context menus,
# the panel's and the standard text menu of the fields; the Dialogs menu opens
# a modal and a modeless dialog.
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
