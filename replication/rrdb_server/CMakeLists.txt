set(MY_PROJ_NAME "rrdb")
project(${MY_PROJ_NAME} C CXX)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "../../base/rrdb_types.cpp")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH "../include" "../../include" "${DSN_ROOT}/include/ext")

set(MY_PROJ_LIBS "")

if (UNIX)
	set(MY_PROJ_LIBS rocksdb ${MY_PROJ_LIBS} z bz2 snappy rt aio pthread)
else()
	set(MY_PROJ_LIBS rocksdblib ${MY_PROJ_LIBS} rpcrt4)
endif()

set(MY_PROJ_LIB_PATH "../..")

set(MY_BOOST_PACKAGES "")

# Extra files that will be installed
file(GLOB RES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
set(MY_BINPLACES ${RES_FILES})

# Copy libdsn.core.so and set rpath, to make rrdb use dsn shared lib in local directory.
if (UNIX)
    SET(CMAKE_INSTALL_RPATH ".")
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    file(GLOB THRIFT_LIBS "${DSN_ROOT}/lib/libthrift.so*")
    set(MY_BINPLACES ${MY_BINPLACES} "${DSN_ROOT}/lib/libdsn.core.so" ${THRIFT_LIBS})
endif()

dsn_add_executable()
