set(MY_PROJ_NAME "pegasus.clientlib")
set(DSN_IN_CORE "TRUE")

if(UNIX)
    add_compile_options(-fPIC)
endif()

project(${MY_PROJ_NAME} C CXX)

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_SRC "../../base/rrdb_types.cpp")

set(MY_PROJ_INC_PATH "../../include")

set(MY_PROJ_LIB_PATH "")

# Copy libdsn.core.so and set rpath, to make rrdb use dsn shared lib in local directory.
if (UNIX)
    SET(CMAKE_INSTALL_RPATH ".")
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    #set(MY_BINPLACES "${DSN_ROOT}/lib/libdsn.core.so")
endif()

dsn_add_shared_library()
