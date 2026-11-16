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

# Boost is linked statically into the executable. A shared library can take
# static libraries only if they are position independent (b2 ... link=static
# cxxflags=-fPIC cflags=-fPIC, see README.md), so the shared ones are linked
# where every component has one. Where one is missing the static libraries
# stay: a library linked with only some of Boost, or with none of it, links
# without a message and then cannot be loaded.
set(CINTERFACE_LIBRARIES ${SIM_EXE_LIBRARIES})
if (NOT WIN32)
    # A Boost found through its CMake files does not set Boost_LIBRARY_DIRS;
    # its libraries are two levels above Boost_DIR (<lib>/cmake/Boost-<version>)
    set(CINTERFACE_BOOST_HINTS ${Boost_LIBRARY_DIRS})
    if (Boost_DIR)
        get_filename_component(CINTERFACE_BOOST_LIBDIR "${Boost_DIR}/../.." ABSOLUTE)
        list(APPEND CINTERFACE_BOOST_HINTS "${CINTERFACE_BOOST_LIBDIR}")
    endif()
    set(CINTERFACE_BOOST_SHARED "")
    set(CINTERFACE_BOOST_ALL_SHARED TRUE)
    foreach(component filesystem process)
        find_library(BOOST_${component}_SHARED
            NAMES libboost_${component}.so
            HINTS ${CINTERFACE_BOOST_HINTS} /usr/lib/x86_64-linux-gnu
        )
        if (BOOST_${component}_SHARED)
            list(APPEND CINTERFACE_BOOST_SHARED ${BOOST_${component}_SHARED})
        else()
            set(CINTERFACE_BOOST_ALL_SHARED FALSE)
        endif()
    endforeach()
    if (CINTERFACE_BOOST_ALL_SHARED)
        list(REMOVE_ITEM CINTERFACE_LIBRARIES ${Boost_LIBRARIES})
        list(APPEND CINTERFACE_LIBRARIES ${CINTERFACE_BOOST_SHARED})
        message(STATUS "C interface: shared Boost libraries: ${CINTERFACE_BOOST_SHARED}")
    else()
        message(STATUS "C interface: no shared Boost for every component, linking the static libraries (they must be built with -fPIC)")
    endif()
endif()

# An unresolved symbol is a link error, not a library that cannot be loaded
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(vacaskcinterface PRIVATE "-Wl,-z,defs")
endif()

target_link_libraries(
    vacaskcinterface
    simlib
    ${CINTERFACE_LIBRARIES}
)
