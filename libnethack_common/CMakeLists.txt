set(LIBNETHACK_COMMON_SOURCES
	src/common_options.c
	src/hacklib.c
	src/mail.c
	src/menulist.c
	src/netconnect.c
	src/trietable.c
	src/utf8conv.c
	src/xmalloc.c
)

add_library(libnethack_common STATIC "")
target_sources(libnethack_common PRIVATE ${LIBNETHACK_COMMON_SOURCES})
target_include_directories(libnethack_common PUBLIC include ../libnethack/include)
