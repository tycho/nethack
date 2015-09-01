cmake_minimum_required (VERSION 2.8.4)
project (DynaHack C)

# path prefix to install the game under, defaults to $HOME/dynahack
if (NOT INSTALL_BASE)
    if (WIN32)
        set(INSTALL_BASE "$ENV{USERPROFILE}")
    else ()
        set(INSTALL_BASE "$ENV{HOME}")
    endif ()
endif ()

# default installation paths
if (BINDIR)
    set (BINDIR "${BINDIR}" CACHE PATH "binary location")
else ()
    set (BINDIR "${INSTALL_BASE}/dynahack/dynahack-data"
         CACHE PATH "binary location" )
endif()

if (LIBDIR)
    set (LIBDIR "${LIBDIR}" CACHE PATH "library location")
else ()
    set (LIBDIR "${INSTALL_BASE}/dynahack/dynahack-data"
         CACHE PATH "library location" )
endif()

if (DATADIR)
    set (DATADIR "${DATADIR}" CACHE PATH "data location")
else ()
    set (DATADIR "${INSTALL_BASE}/dynahack/dynahack-data"
         CACHE PATH "data location" )
endif()

if (SHELLDIR)
    set (SHELLDIR "${SHELLDIR}" CACHE PATH "launch script location")
else ()
    set (SHELLDIR "${INSTALL_BASE}/dynahack"
         CACHE PATH "launch script location" )
endif()

set (ALL_STATIC FALSE CACHE BOOL "Fully static build")
if (NOT ALL_STATIC)
    set (LIB_TYPE SHARED)
    if (UNIX)
        # SMART RPATH DEFAULT VALUE
        #
        # Objective: Use a relative rpath (i.e. "$ORIGIN...") if LIBDIR is
        # a subpath of BINDIR, otherwise use an absolute rpath.
        #
        # TODO (by somebody who is not me): Mac OS X uses some combination
        # of @rpath / @loader_path / @executable_path rather than $ORIGIN.
        string (LENGTH ${LIBDIR} LIBDIR_LENGTH)
        string (LENGTH ${BINDIR} BINDIR_LENGTH)
        if (${LIBDIR_LENGTH} LESS ${BINDIR_LENGTH})
            # LIBDIR is too short to be a subpath of BINDIR
            set (GAME_RPATH_DEFAULT ${LIBDIR})
        else ()
            string (SUBSTRING ${LIBDIR} 0 ${BINDIR_LENGTH} LIBDIR_PREFIX)
            string (COMPARE EQUAL ${LIBDIR_PREFIX} ${BINDIR} LIBDIR_PREFIX_EQUALS_BINDIR)
            if (LIBDIR_PREFIX_EQUALS_BINDIR)
                # LIBDIR is a subpath of BINDIR, so use $ORIGIN
                if (${LIBDIR_LENGTH} GREATER ${BINDIR_LENGTH})
                    # Get relative path from BINDIR to LIBDIR
                    string (SUBSTRING ${LIBDIR} ${BINDIR_LENGTH} -1 LIBDIR_REL_BINDIR)
                    set (GAME_RPATH_DEFAULT_ORIGIN_SUFFIX ${LIBDIR_REL_BINDIR})
                else ()
                    set (GAME_RPATH_DEFAULT_ORIGIN_SUFFIX "")
                endif ()
                set (GAME_RPATH_DEFAULT "\$ORIGIN${GAME_RPATH_DEFAULT_ORIGIN_SUFFIX}")
            else ()
                # LIBDIR is separate from BINDIR
                set (GAME_RPATH_DEFAULT ${LIBDIR})
            endif ()
        endif ()

        # Generally, this should be either be set to:
        #
        # 1. LIBDIR relative to BINDIR (e.g. "$ORIGIN") for a relocatable build
        # 2. LIBDIR absolute path for an installation with fixed paths
        set (GAME_RPATH ${GAME_RPATH_DEFAULT}
             CACHE PATH "extra dynamic library search paths for binary")
    endif ()
else ()
    add_definitions(-DSTATIC_BUILD)
    set (LIB_TYPE STATIC)
endif ()

# DynaHack server currently uses several Linux-specific apis.
# Generalizing the code to the point where it will work on other systems
# is not impossible, but the work has not been done.
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (ENABLE_SERVER FALSE CACHE BOOL "Build the DynaHack network server")
else ()
    set (ENABLE_SERVER FALSE)
endif ()

set (ENABLE_NETCLIENT FALSE CACHE BOOL "Enable network client mode")

if (CMAKE_COMPILER_IS_GNUCC)
    set (CMAKE_C_FLAGS_DEBUG "-Wall -g3 -Wold-style-definition -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wpointer-arith -Wformat-security")
    if (UNIX AND NOT CYGWIN)
	add_definitions (-fvisibility=hidden)
    endif ()
endif ()

# dynahack core
add_subdirectory (libnitrohack)

# dynahack network client
if (ENABLE_NETCLIENT)
    add_subdirectory (libnitrohack_client)
endif ()

# dynahack text ui
add_subdirectory (nitrohack)

if (ENABLE_SERVER)
    add_subdirectory (nitrohack_server)
endif ()
