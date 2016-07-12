set(MY_PROJ_NAME redis_proxy_test)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH
    ../../src
    ../proxy_lib
    )

if (UNIX)
    set(MY_PROJ_LIBS gtest pthread)
else()
    set(MY_PROJ_LIBS gtest)
endif()

set(MY_BOOST_PACKAGES "system")
set(MY_PROJ_LIBS
    redis_proxy
    rrdb.clientlib
    ${MY_PROJ_LIBS}
    )

set(MY_BINPLACES "config.ini")

dsn_add_executable()

