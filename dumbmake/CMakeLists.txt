set(DUMBMAKE_SOURCES
	dumbmake_get_option.c
)

set(DATADIR "${CMAKE_INSTALL_PREFIX}/data")
set(STATEDIR "${CMAKE_INSTALL_PREFIX}/save")

add_library(dumbmake STATIC "")
target_sources(dumbmake PRIVATE ${DUMBMAKE_SOURCES})
target_compile_definitions(dumbmake PRIVATE -DGAMESDATADIR="${DATADIR}" -DGAMESSTATEDIR="${STATEDIR}")
