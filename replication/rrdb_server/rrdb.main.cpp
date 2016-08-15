// apps
# include "rrdb.server.impl.h"
# include <dsn/cpp/replicated_service_app.h>

void dsn_app_registration_rrdb()
{
    // register all possible service apps
    dsn::register_app_with_type_1_replication_support< ::dsn::apps::rrdb_service_impl>("rrdb");
}

# ifndef DSN_RUN_USE_SVCHOST

int main(int argc, char** argv)
{
    dsn_app_registration_rrdb();
    
    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);
    return 0;
}

# else

# include <dsn/internal/module_int.cpp.h>

MODULE_INIT_BEGIN
    dsn_app_registration_rrdb();
MODULE_INIT_END

# endif
