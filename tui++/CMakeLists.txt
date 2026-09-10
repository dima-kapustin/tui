project(tui++ VERSION 1.0)

find_package(Threads)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*/*.cpp"
)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc/*/*.h"
)

add_library ( ${PROJECT_NAME} ${SOURCES} ${HEADERS} )
target_include_directories( ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Inc  )
target_link_libraries( ${PROJECT_NAME} PRIVATE
    Threads::Threads
    )

if(MINGW)
    # Link the GCC runtime statically into every executable that uses the
    # library. A Windows shell's PATH may put another MinGW toolchain first
    # (Git's /mingw64/bin is the common one), and Windows resolves
    # libstdc++-6.dll, libgcc_s_seh-1.dll and libwinpthread-1.dll by name from
    # the PATH -- so a binary built with one toolchain loads the other's C++
    # runtime and two incompatible implementations of iostreams, exceptions
    # and allocation meet in one process. The release test run used to die in
    # the first std::ofstream it constructed for exactly this reason. A static
    # runtime makes a binary self-contained whatever the PATH holds.
    #
    # INTERFACE, not PUBLIC: the library is a static archive, so the option
    # belongs to the executables linking it.
    target_link_options(${PROJECT_NAME} INTERFACE -static)
endif()



#install ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})
