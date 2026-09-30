# C interface shared library (libvacaskcinterface.so) for driving VACASK
# from a co-simulation master, mirrors Xyce's libxycecinterface.
# The control block interpreter lives in simulator/, so it is compiled in.
add_library(vacaskcinterface SHARED
    vacaskcinterface.h
    vacaskcinterface.cpp
    ../simulator/platform.cpp
    ../simulator/config.cpp
    ../simulator/progressbar.cpp
    ../simulator/cmd.cpp
)
target_include_directories(vacaskcinterface PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../simulator")

target_compile_definitions(vacaskcinterface PUBLIC PROGRAM_NAME=${PROGRAM_NAME})
target_compile_definitions(vacaskcinterface PUBLIC PROGRAM_VERSION=${PROGRAM_VERSION})
target_compile_definitions(vacaskcinterface PUBLIC PROGRAM_COPYRIGHT=${PROGRAM_COPYRIGHT})
target_compile_definitions(vacaskcinterface PUBLIC PROGRAM_HOMEPAGE="${PROGRAM_HOMEPAGE}")

add_dependencies(vacaskcinterface simlib)

# Keep simlib's symbols private to the library, export only vacask_*
target_link_options(vacaskcinterface PRIVATE "-Wl,--exclude-libs,ALL")

# Boost is linked statically into the executable, but its static libraries
# are not position independent. Link the shared ones instead.
set(CINTERFACE_LIBRARIES ${SIM_EXE_LIBRARIES})
if (NOT WIN32)
    list(REMOVE_ITEM CINTERFACE_LIBRARIES ${Boost_LIBRARIES})
    foreach(component filesystem process)
        find_library(BOOST_${component}_SHARED
            NAMES libboost_${component}.so
            HINTS ${Boost_LIBRARY_DIRS} /usr/lib/x86_64-linux-gnu
        )
        if (BOOST_${component}_SHARED)
            list(APPEND CINTERFACE_LIBRARIES ${BOOST_${component}_SHARED})
        endif()
    endforeach()
endif()

target_link_libraries(
    vacaskcinterface
    simlib
    ${CINTERFACE_LIBRARIES}
)
