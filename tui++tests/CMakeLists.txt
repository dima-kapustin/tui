cmake_minimum_required(VERSION 3.20)
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET_NAME})


add_executable(${PROJECT_NAME}
  Src/test_utf-8.cpp
  Src/test_EnumMask.cpp
  Src/test_GlyphIterator.cpp
  Src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    tui++
)



#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
