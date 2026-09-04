cmake_minimum_required(VERSION 3.20)
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET_NAME})

# The tests executable: the unit tests and the interactive font tools, whose
# entry point (main.cpp) sits directly in Src. Standalone programs live in
# their own project directories (e.g. the MenuBarDemo project) and are built
# from the top-level CMakeLists.txt instead.
file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    tui++
)

#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
