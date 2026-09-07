cmake_minimum_required(VERSION 3.20)

# TextAreaDemo - the JTextArea-style editor demo of tui++ (text backend).
#
#     TextAreaDemo                    open an in-memory demo document
#     TextAreaDemo <file>             open a (huge) file read+editable in RAM
#     TextAreaDemo --log-events [...] with the event/resize/graphics log on
#
# The window hosts one ScrollPane whose viewport shows a TextArea bound to a
# TextBuffer (memory-mapped, lazily indexed). This is the test bed for viewing
# and editing very large files with bounded memory:
#
#   - arrows/Home/End/Ctrl+Home/Ctrl+End/PageUp/PageDown move the caret
#   - typing edits; the buffer is memory-mapped, only edited pages are copied
#   - F3 opens the search line, typing + Enter jumps to the next match
#     (F3 again repeats the last search), F4 toggles regexp search
#   - F5 toggles visible whitespace (space dots, tab arrows)
#   - the wheel and the scroll bar move through the file
#
# Self-contained CMake project (see MenuBarDemo). Quit with Ctrl+C.
project(TextAreaDemo)

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
