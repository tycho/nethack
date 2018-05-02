set(LIBUNCURSED_SOURCES
	src/libuncursed.c
	src/plugins.c
)

add_library(uncursed STATIC "")
target_sources(uncursed PRIVATE ${LIBUNCURSED_SOURCES})
target_include_directories(uncursed PUBLIC include)

add_library(uncursed_tty OBJECT src/plugins/tty.c src/plugins/tty.cxx)
target_include_directories(uncursed_tty PUBLIC include)
