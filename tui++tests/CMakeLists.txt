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

# The tests assert() their way through most suites; Release builds define
# NDEBUG, which would compile the assertions out and make the pack pass
# vacuously (test_Menu works around that with its own CHECK, but the other
# suites would verify nothing). Re-enable assertions for this target in every
# configuration, so a regression always fails the build where it was
# introduced.
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /UNDEBUG)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -UNDEBUG)
endif()

# Run the regression pack (the unit tests of this executable; the interactive
# font tools need explicit arguments and are not exercised) after the tests
# executable is (re)built, so a regression fails the build right where it was
# introduced instead of waiting for a manual test run.
#
# The run is a POST_BUILD step of the executable, NOT an ALL custom target: a
# phony ALL target executes its command on every `cmake --build` even when
# nothing changed, while a POST_BUILD step lives in the executable's own build
# rule and runs only when the link actually happened -- i.e. only when the
# tui++ library or the test sources were rebuilt. A no-op build (nothing to
# build) and a build that touched only other targets (e.g. a demo) skip the
# suite, because the binary it would exercise did not change. To run the suite
# by hand: `cmake --build build --target tui++tests_regression`, or invoke the
# executable directly. Iterating with a slow test suite in the loop? Disable
# it with -DTUI_RUN_REGRESSION_ON_BUILD=OFF.
option(TUI_RUN_REGRESSION_ON_BUILD "Run the regression tests when the tests executable is rebuilt" ON)

if(TUI_RUN_REGRESSION_ON_BUILD)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND $<TARGET_FILE:${PROJECT_NAME}>
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Running the ${PROJECT_NAME} regression tests"
      VERBATIM)

  # A manual handle for the suite (never part of the plain build):
  # `cmake --build build --target ${PROJECT_NAME}_regression` runs the tests
  # executable and its dependencies.
  add_custom_target(${PROJECT_NAME}_regression
      COMMAND $<TARGET_FILE:${PROJECT_NAME}>
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Running the ${PROJECT_NAME} regression tests"
      VERBATIM)
  add_dependencies(${PROJECT_NAME}_regression ${PROJECT_NAME})
endif()

#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
