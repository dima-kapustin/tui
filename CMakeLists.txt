cmake_minimum_required(VERSION 3.20)
project(tui)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(tui++)
add_subdirectory(tui++tests)
