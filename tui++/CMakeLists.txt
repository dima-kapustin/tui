project(tui++ VERSION 1.0)

find_package(Threads)

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc/*.h"
)

add_library ( ${PROJECT_NAME} ${SOURCES} ${HEADERS} )
target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc  )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    Threads::Threads
    )



#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})
