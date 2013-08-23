# libnethack toplevel

#convenient names for inout directories
set (LNH_SRC  ${DynaHack_SOURCE_DIR}/libnitrohack/src)
set (LNH_DAT  ${DynaHack_SOURCE_DIR}/libnitrohack/dat)
set (LNH_UTIL ${DynaHack_SOURCE_DIR}/libnitrohack/util)
set (LNH_INC ${DynaHack_SOURCE_DIR}/libnitrohack/include)

# various generated files live here
set (LNH_SRC_GEN  ${DynaHack_BINARY_DIR}/libnitrohack/src)
set (LNH_DAT_GEN  ${DynaHack_BINARY_DIR}/libnitrohack/dat)
set (LNH_UTIL_GEN ${DynaHack_BINARY_DIR}/libnitrohack/util)
set (LNH_INC_GEN ${DynaHack_BINARY_DIR}/libnitrohack/include)

# actual target definitions etc.
add_subdirectory (util)
add_subdirectory (dat)
add_subdirectory (src)
