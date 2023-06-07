cmake_minimum_required(VERSION 3.20)
get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${TARGET_NAME})




file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    tui++
)



#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
