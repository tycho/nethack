# build the curses window port of the game


if (NOT WIN32)
    set (USE_PDCURSES FALSE CACHE BOOL "Use PDCurses?")
    set (PDCURSES_LIBNAME "XCurses")
else ()
    set (USE_PDCURSES TRUE)
    set (PDCURSES_LIBNAME "pdcurses")
endif ()


if (USE_PDCURSES)
    set (PDCURSES_LIB_DIR "" CACHE PATH "Location of the pdcurses library")
    set (PDCURSES_INC_DIR "" CACHE PATH "Location of the pdcurses header")
    link_directories (${PDCURSES_LIB_DIR})
    include_directories (${PDCURSES_INC_DIR})
    add_definitions (-DPDCURSES)
endif ()
if (USE_OSX_HOMEBREW_CURSES)
    add_definitions(-DUSE_OSX_HOMEBREW_CURSES)
endif()


set (NH_CURSES_SRC
    src/main.c
    src/color.c
    src/dialog.c
    src/gameover.c
    src/getline.c
    src/keymap.c
    src/map.c
    src/menu.c
    src/messages.c
    src/options.c
    src/outchars.c
    src/playerselect.c
    src/replay.c
    src/rungame.c
    src/sidebar.c
    src/status.c
    src/topten.c
    src/windows.c)
if (ENABLE_NETCLIENT)
    set (NH_CURSES_SRC
        ${NH_CURSES_SRC}
        src/netgame.c
        src/netplay.c)
endif ()
if (WIN32)
    set (NH_CURSES_SRC ${NH_CURSES_SRC} rc/nh_ico.rc)
endif ()

include_directories (${DynaHack_SOURCE_DIR}/include
                     include)

add_definitions(-DDYNAHACKDIR="${DATADIR}")
if (ENABLE_NETCLIENT)
    add_definitions(-DNETCLIENT)
endif ()

link_directories (${DynaHack_BINARY_DIR}/libnitrohack/src)
link_directories (${DynaHack_BINARY_DIR}/libnitrohack_client)
add_executable (nitrohack_curses ${NH_CURSES_SRC} )
set_target_properties(nitrohack_curses PROPERTIES OUTPUT_NAME dynahack)
target_link_libraries(nitrohack_curses nitrohack)
if (ALL_STATIC)
    # kludge like the one below for jansson and libnitrohack_client.
    # It's actually libnitrohack that requires zlib, not nitrohack_curses,
    # but CMake can't chain dependencies transitively, AFAIK.
    target_link_libraries(nitrohack_curses z)
endif ()

if (USE_PDCURSES)
    target_link_libraries (nitrohack_curses ${PDCURSES_LIBNAME})
else ()
    if(USE_OSX_HOMEBREW_CURSES)
        target_link_libraries (nitrohack_curses ncurses)
    else ()
        target_link_libraries (nitrohack_curses ncursesw)
    endif ()
endif ()

if (ENABLE_NETCLIENT)
    add_dependencies (nitrohack_curses libnitrohack_client)
    target_link_libraries (nitrohack_curses nitrohack_client z)
    # this is a kludge; these libraries are required by libnitrohack_client, but
    # for static linking they must be specified here. I couldn't figure out how
    # to set the dependencies from the libnitrohack_client CMakeLists.txt
    if (ALL_STATIC)
	target_link_libraries(nitrohack_curses jansson)
	if (WIN32)
	    target_link_libraries(nitrohack_curses Ws2_32)
	endif ()
    endif()
endif ()


add_dependencies (nitrohack_curses libnitrohack)

if (UNIX)
    add_custom_command(OUTPUT dynahack.sh
                       COMMAND printf "%s\\n%s\\n" "#!/bin/sh"
                               "LD_LIBRARY_PATH=${LIBDIR} ${BINDIR}/dynahack $@"
                               >> dynahack.sh
                       VERBATIM)
    add_custom_target(nitrohack_curses_shell_script ALL
                      DEPENDS dynahack.sh)
    install(PROGRAMS ${DynaHack_BINARY_DIR}/nitrohack/dynahack.sh
            DESTINATION ${SHELLDIR} RENAME dynahack)
endif()

install(TARGETS nitrohack_curses
        DESTINATION ${BINDIR})
