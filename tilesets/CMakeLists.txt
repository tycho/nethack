add_executable(tilecompile
	util/tilecompile.c
	util/tileset-read.c
	util/tileset-write.c
	src/fallback-tileset-image.c
	src/tilesequence.c
	../libnethack/src/drawing.c
	../libnethack/src/monst.c
	../libnethack/src/objects.c
	../libnethack/src/symclass.c
	../libnethack_common/src/hacklib.c
	../libnethack_common/src/utf8conv.c
	../libnethack_common/src/xmalloc.c
	../nethack/src/brandings.c
)
add_dependencies(tilecompile makedefs-onames makedefs-pm)
target_include_directories(tilecompile PRIVATE include ../libnethack_common/include ../libnethack/include ../nethack/include ${CMAKE_CURRENT_BINARY_DIR}/../libnethack)

add_executable(basecchar
	util/basecchar.c
	src/tilesequence.c
	../libnethack/src/drawing.c
	../libnethack/src/monst.c
	../libnethack/src/objects.c
	../libnethack/src/symclass.c
	../libnethack_common/src/hacklib.c
	../libnethack_common/src/xmalloc.c
	../nethack/src/brandings.c
)
add_dependencies(basecchar makedefs-onames makedefs-pm)
target_include_directories(basecchar PRIVATE include ../libnethack_common/include ../libnethack/include ../nethack/include ${CMAKE_CURRENT_BINARY_DIR}/../libnethack)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/base.txt
	DEPENDS basecchar
	COMMAND $<TARGET_FILE:basecchar> -o ${CMAKE_CURRENT_BINARY_DIR}/base.txt)

set(ASCII_TILEDATA
	${CMAKE_CURRENT_BINARY_DIR}/base.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/ascii_overrides.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/dungeoncolors.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/rogue_overrides.txt)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/textascii.nh4ct
	DEPENDS tilecompile ${ASCII_TILEDATA}
	COMMAND $<TARGET_FILE:tilecompile> -q -W -t nh4ct -n ASCII -z 0 0 -o ${CMAKE_CURRENT_BINARY_DIR}/textascii.nh4ct ${ASCII_TILEDATA})
add_custom_target(ascii-tiles ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/textascii.nh4ct)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/textascii.nh4ct
	DESTINATION data)

set(UNICODE_TILEDATA
	${CMAKE_CURRENT_BINARY_DIR}/base.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/ascii_overrides.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/unicode_overrides.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/dungeoncolors.txt
	${CMAKE_CURRENT_SOURCE_DIR}/dat/text/rogue_overrides.txt)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/textunicode.nh4ct
	DEPENDS tilecompile ${UNICODE_TILEDATA}
	COMMAND $<TARGET_FILE:tilecompile> -q -W -t nh4ct -n Unicode -z 0 0 -o ${CMAKE_CURRENT_BINARY_DIR}/textunicode.nh4ct ${UNICODE_TILEDATA})
add_custom_target(unicode-tiles ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/textunicode.nh4ct)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/textunicode.nh4ct
	DESTINATION data)
