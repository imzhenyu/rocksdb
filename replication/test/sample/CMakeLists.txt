set(MY_PROJ_NAME "rrdb.test.sample")
project(${MY_PROJ_NAME} C CXX)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH "../lib")

set(MY_PROJ_LIBS
    dsn.replication
    dsn.replication.meta_server
    dsn.replication.clientlib
    dsn.failure_detector
    dsn.replication.zookeeper_provider
    zookeeper_mt
    pegasus.test
    )

set(MY_PROJ_LIB_PATH "../..")

set(MY_BOOST_PACKAGES system filesystem)

file(GLOB
    RES_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ini"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cmd"
    )

# Extra files that will be installed
set(MY_BINPLACES ${RES_FILES})

# Copy libdsn.core.so and set rpath, to make rrdb use dsn shared lib in local directory.
if (UNIX)
    SET(CMAKE_INSTALL_RPATH ".")
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(MY_BINPLACES ${MY_BINPLACES} "${DSN_ROOT}/lib/libdsn.core.so" "${DSN_ROOT}/lib/libthrift.so")
endif()

dsn_add_executable()
