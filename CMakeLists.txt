cmake_minimum_required(VERSION 3.10)
project(NetHack4 C CXX)

set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/nethack4dir2" CACHE PATH "installation root")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

find_package(BISON)
find_package(FLEX)
find_package(Threads)

add_definitions(
	-DDUMBMAKE
	-DAIMAKE_NORETURN=_Noreturn
)
add_compile_options(-include ${CMAKE_CURRENT_SOURCE_DIR}/dumbmake/dumbmake.h)

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
	add_definitions(
		-DAIMAKE_BUILDOS_linux
		-D_XOPEN_SOURCE=700
		-D_REENTRANT
	)
endif()
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "FreeBSD")
	add_definitions(
		-DAIMAKE_BUILDOS_freebsd
	)
endif()
if(APPLE)
	add_definitions(
		-DAIMAKE_BUILDOS_darwin
	)
endif()
if(CMAKE_HOST_WIN32)
	add_definitions(
		-DAIMAKE_BUILDOS_MSWin32
	)
endif()

if(UNIX AND NOT WIN32)
	if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
		add_compile_options(-fPIC)
	endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
endif(UNIX AND NOT WIN32)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(dumbmake)
add_subdirectory(nethack)
add_subdirectory(libnethack)
add_subdirectory(libnethack_common)
add_subdirectory(libuncursed)
add_subdirectory(tilesets)

install(DIRECTORY DESTINATION data)
install(DIRECTORY DESTINATION save)
