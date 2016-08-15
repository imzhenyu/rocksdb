cmake_minimum_required(VERSION 2.8.8)

set(DSN_ROOT "$ENV{DSN_ROOT}")
if((DSN_ROOT STREQUAL "") OR (NOT EXISTS "${DSN_ROOT}/"))
    message(FATAL_ERROR "Please make sure that DSN_ROOT is defined and does exists.")
endif()

include("${DSN_ROOT}/bin/dsn.cmake")

set(DSN_BUILD_RUNTIME FALSE)

dsn_common_setup()

add_subdirectory(rrdb_server)
add_subdirectory(client_lib)
add_subdirectory(shell)
add_subdirectory(rrdb_kill_test)
add_subdirectory(redis_protocol)
