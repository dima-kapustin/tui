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

# Run the regression pack (the unit tests of this executable; the interactive
# font tools need explicit arguments and are not exercised) after EVERY build
# by default, so a regression fails the build right where it was introduced
# instead of waiting for a manual test run. The run is a separate ALL target:
# a plain `cmake --build` of the whole project executes it even when nothing
# relinked, while building an individual target (`--target TextAreaDemo`) or
# this executable alone stays untouched. Iterating with a slow test suite in
# the loop? Disable it with -DTUI_RUN_REGRESSION_ON_BUILD=OFF.
option(TUI_RUN_REGRESSION_ON_BUILD "Run the regression tests after every build" ON)

if(TUI_RUN_REGRESSION_ON_BUILD)
  add_custom_target(${PROJECT_NAME}_regression
      ALL
      COMMAND $<TARGET_FILE:${PROJECT_NAME}>
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Running the ${PROJECT_NAME} regression tests"
      VERBATIM)
  # Ordering only: the run happens after the tests executable (and, through
  # its link, the tui++ library) is up to date.
  add_dependencies(${PROJECT_NAME}_regression ${PROJECT_NAME})
endif()

#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
