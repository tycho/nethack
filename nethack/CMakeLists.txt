
set(NETHACK_SOURCES
	src/brandings.c
	src/color.c
	src/dialog.c
	src/extrawin.c
	src/gameover.c
	src/getline.c
	src/keymap.c
	src/mail.c
	src/main.c
	src/map.c
	src/menu.c
	src/messages.c
	src/motd.c
	src/options.c
	src/outchars.c
	src/playerselect.c
	src/replay.c
	src/rungame.c
	src/sidebar.c
	src/status.c
	src/topten.c
	src/windows.c
	../tilesets/src/tilesequence.c
	$<TARGET_OBJECTS:uncursed_tty>
)


add_executable(nethack-exe "")
target_sources(nethack-exe PRIVATE ${NETHACK_SOURCES})
target_include_directories(nethack-exe PUBLIC include)
target_include_directories(nethack-exe PRIVATE ../tilesets/include)
target_link_libraries(nethack-exe dumbmake nethack uncursed ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(nethack-exe PROPERTIES OUTPUT_NAME nethack)

install(TARGETS nethack-exe
	    RUNTIME DESTINATION .
		LIBRARY DESTINATION lib)
