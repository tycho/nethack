find_package(ZLIB)

set(LIBNETHACK_SOURCES
	src/allmain.c
	src/apply.c
	src/artifact.c
	src/attrib.c
	src/ball.c
	src/bones.c
	src/botl.c
	src/cmd.c
	src/dbridge.c
	src/decl.c
	src/detect.c
	src/dig.c
	src/display.c
	src/dlb.c
	src/do.c
	src/dog.c
	src/dogmove.c
	src/dokick.c
	src/do_name.c
	src/dothrow.c
	src/do_wear.c
	src/drawing.c
	src/dump.c
	src/dungeon.c
	src/eat.c
	src/end.c
	src/engrave.c
	src/exper.c
	src/explode.c
	src/extralev.c
	src/files.c
	src/fountain.c
	src/hack.c
	src/history.c
	src/invent.c
	src/level.c
	src/light.c
	src/localtime.c
	src/lock.c
	src/log.c
	src/mail.c
	src/makemon.c
	src/mcastu.c
	src/memfile.c
	src/messages.c
	src/mhitm.c
	src/mhitq.c
	src/mhitu.c
	src/minion.c
	src/mklev.c
	src/mkmap.c
	src/mkmaze.c
	src/mkobj.c
	src/mkroom.c
	src/mon.c
	src/mondata.c
	src/monmove.c
	src/monst.c
	src/mplayer.c
	src/mthrowu.c
	src/muse.c
	src/music.c
	src/newrng.c
	src/objects.c
	src/objnam.c
	src/o_init.c
	src/options.c
	src/pager.c
	src/pickup.c
	src/pline.c
	src/polyself.c
	src/potion.c
	src/pray.c
	src/priest.c
	src/prop.c
	src/quest.c
	src/questpgr.c
	src/read.c
	src/rect.c
	src/region.c
	src/restore.c
	src/role.c
	src/rumors.c
	src/save.c
	src/shk.c
	src/shknam.c
	src/sit.c
	src/sounds.c
	src/spell.c
	src/sp_lev.c
	src/steal.c
	src/steed.c
	src/symclass.c
	src/teleport.c
	src/timeout.c
	src/topten.c
	src/track.c
	src/trap.c
	src/uhitm.c
	src/u_init.c
	src/vault.c
	src/version.c
	src/vision.c
	src/weapon.c
	src/were.c
	src/wield.c
	src/windows.c
	src/wizard.c
	src/worm.c
	src/worn.c
	src/write.c
	src/zap.c
	${CMAKE_CURRENT_BINARY_DIR}/readonly.c
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/artinames.h
	MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/include/artilist.h
	COMMAND ${CMAKE_C_COMPILER} -E  ${CMAKE_CURRENT_SOURCE_DIR}/include/artilist.h -o ${CMAKE_CURRENT_BINARY_DIR}/artinames.h -DARTINAMES_H
)

#
# data definitions generator
#
add_executable(makedefs
	util/makedefs.c src/objects.c src/monst.c
	${CMAKE_CURRENT_BINARY_DIR}/artinames.h
)
target_include_directories(makedefs PRIVATE include ../libnethack_common/include ${CMAKE_CURRENT_BINARY_DIR})

macro(add_makedefs)
	set(options "")
	set(oneValueArgs TYPE OUTPUT NICK)
	set(multiValueArgs "")
	cmake_parse_arguments(ADD_MAKEDEFS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	add_custom_command(
		OUTPUT ${ADD_MAKEDEFS_OUTPUT}
		DEPENDS makedefs
		COMMAND $<TARGET_FILE:makedefs> ${ADD_MAKEDEFS_TYPE} ${ADD_MAKEDEFS_OUTPUT}
	)
	add_custom_target(makedefs-${ADD_MAKEDEFS_NICK} DEPENDS ${ADD_MAKEDEFS_OUTPUT})
endmacro()

macro(gen_data_file)
	set(options NHDAT)
	set(oneValueArgs MAKEDEFS_ARG OUTPUT)
	set(multiValueArgs DEPENDS INPUTS)
	cmake_parse_arguments(GEN_DATA_FILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(${GEN_DATA_FILE_NHDAT})
		set(NHDAT_FILES ${NHDAT_FILES} ${GEN_DATA_FILE_OUTPUT})
	endif()

	add_custom_command(
		OUTPUT ${GEN_DATA_FILE_OUTPUT}
		DEPENDS makedefs ${GEN_DATA_FILE_DEPENDS}
		COMMAND $<TARGET_FILE:makedefs> ${GEN_DATA_FILE_MAKEDEFS_ARG} ${GEN_DATA_FILE_DEPENDS} ${GEN_DATA_FILE_OUTPUT}
	)
endmacro()

add_makedefs(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/onames.h TYPE -o NICK onames)
add_makedefs(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/readonly.c TYPE -m NICK readonly)
add_makedefs(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/date.h TYPE -v NICK date)
add_makedefs(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/verinfo.h TYPE -w NICK verinfo)
add_makedefs(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pm.h TYPE -p NICK pm)

#
# dlb nhdat archive tool
#
add_executable(dlb
	util/dlb_main.c src/dlb.c)
target_include_directories(dlb PRIVATE include ../libnethack_common/include)

#
# dungeon compiler
#
bison_target(dgn_parser util/dgn_comp.y ${CMAKE_CURRENT_BINARY_DIR}/dgn_yacc.c
	COMPILE_FLAGS "--defines=${CMAKE_CURRENT_BINARY_DIR}/dgn_comp.h")
flex_target(dgn_scanner util/dgn_comp.l ${CMAKE_CURRENT_BINARY_DIR}/dgn_lex.c)
add_flex_bison_dependency(dgn_scanner dgn_parser)
add_executable(dgn_comp
	util/dgn_main.c
	${CMAKE_CURRENT_BINARY_DIR}/dgn_yacc.c
	${CMAKE_CURRENT_BINARY_DIR}/dgn_lex.c
	${CMAKE_CURRENT_BINARY_DIR}/verinfo.h
)
target_include_directories(dgn_comp PRIVATE include util ${CMAKE_CURRENT_BINARY_DIR} ../libnethack_common/include)

#
# level compiler
#
bison_target(lev_parser util/lev_comp.y ${CMAKE_CURRENT_BINARY_DIR}/lev_yacc.c
	COMPILE_FLAGS "--defines=${CMAKE_CURRENT_BINARY_DIR}/lev_comp.h")
flex_target(lev_scanner util/lev_comp.l ${CMAKE_CURRENT_BINARY_DIR}/lev_lex.c)
add_flex_bison_dependency(lev_scanner lev_parser)
add_executable(lev_comp
	util/lev_main.c
	src/monst.c
	src/objects.c
	src/symclass.c
	${CMAKE_CURRENT_BINARY_DIR}/lev_yacc.c
	${CMAKE_CURRENT_BINARY_DIR}/lev_lex.c
	${CMAKE_CURRENT_BINARY_DIR}/onames.h
	${CMAKE_CURRENT_BINARY_DIR}/readonly.c
	${CMAKE_CURRENT_BINARY_DIR}/verinfo.h
	${CMAKE_CURRENT_BINARY_DIR}/pm.h
)
target_include_directories(lev_comp PRIVATE include util ${CMAKE_CURRENT_BINARY_DIR} ../libnethack_common/include)

macro(compile_levels)
	set(options "")
	set(oneValueArgs "")
	set(multiValueArgs NAMES)
	cmake_parse_arguments(COMPILE_LEVEL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	foreach(LEVNAME ${COMPILE_LEVEL_NAMES})
		set(LEVEL_TAGS ${LEVEL_TAGS} ${LEVNAME}.tag)
		add_custom_command(
			OUTPUT ${LEVNAME}.tag
			DEPENDS lev_comp
			COMMENT "Compiling level ${LEVNAME}"
			COMMAND $<TARGET_FILE:lev_comp> -q ${CMAKE_CURRENT_SOURCE_DIR}/dat/${LEVNAME}.des && ${CMAKE_COMMAND} -E touch ${LEVNAME}.tag
		)
	endforeach()
endmacro()

#
# libnethack
#
add_library(nethack SHARED "")
target_sources(nethack PRIVATE
	${LIBNETHACK_SOURCES}
	${CMAKE_CURRENT_BINARY_DIR}/onames.h
	${CMAKE_CURRENT_BINARY_DIR}/date.h
	${CMAKE_CURRENT_BINARY_DIR}/verinfo.h
	${CMAKE_CURRENT_BINARY_DIR}/pm.h
	${CMAKE_CURRENT_BINARY_DIR}/readonly.c
)
target_include_directories(nethack PUBLIC include ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(nethack PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(nethack ${ZLIB_LIBRARIES} libnethack_common)
install(TARGETS nethack
	    RUNTIME DESTINATION .
		LIBRARY DESTINATION lib)

#
# nethack data files
#
set(NHDAT_FILES "")
gen_data_file(
	MAKEDEFS_ARG -d
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dat/data.base
	OUTPUT data
	NHDAT)
gen_data_file(
	MAKEDEFS_ARG -e
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dat/dungeon.def
	OUTPUT dungeon.pdf)
gen_data_file(
	MAKEDEFS_ARG -q
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dat/quest.txt
	OUTPUT quest.dat
	NHDAT)
gen_data_file(
	MAKEDEFS_ARG -r
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dat/rumors.tru ${CMAKE_CURRENT_SOURCE_DIR}/dat/rumors.fal
	OUTPUT rumors
	NHDAT)
gen_data_file(
	MAKEDEFS_ARG -h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dat/oracles.txt
	OUTPUT oracles
	NHDAT)
add_custom_command(
	OUTPUT dungeon
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dgn_comp dungeon.pdf
	COMMAND $<TARGET_FILE:dgn_comp> < dungeon.pdf > dungeon)
set(NHDAT_FILES ${NHDAT_FILES} dungeon)

set(LEVEL_TAGS "")
compile_levels(NAMES Arch Barb bigroom castle Caveman endgame gehennom Healer Knight knox medusa mines Monk oracle Priest Ranger Rogue Samurai sokoban Tourist tower Valkyrie Wizard yendor)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nhdat
	DEPENDS dlb ${NHDAT_FILES} ${LEVEL_TAGS}
	COMMAND $<TARGET_FILE:dlb> cf ${CMAKE_CURRENT_BINARY_DIR}/nhdat ${NHDAT_FILES} *.lev)
add_custom_target(nhdat-file ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/nhdat)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/nhdat
	DESTINATION data)
