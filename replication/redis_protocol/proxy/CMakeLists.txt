set(MY_PROJ_NAME rproxy_server)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_INC_PATH ../proxy_lib ../../client_lib/include)

set(MY_PROJ_LIBS redis_proxy rrdb.clientlib ${MY_PROJ_LIBS})

set(MY_BINPLACES "config.ini")

dsn_add_executable()
