// apps
# include "rrdb.app.example.h"
# include "rrdb.server.impl.h"
# include <dsn/utility/module_init.cpp.h>

void dsn_app_registration_rrdb()
{
    // register all possible service apps
    dsn::register_app_with_type_1_replication_support< ::dsn::apps::rrdb_service_impl>("rrdb");

    dsn::register_app< ::dsn::apps::rrdb_client_app>("rrdb.client");
    dsn::register_app< ::dsn::apps::rrdb_perf_test_client_app>("rrdb.client.perf");
}

# if 0

int main(int argc, char** argv)
{
    dsn_app_registration_rrdb();
    
    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);
    return 0;
}

# else

MODULE_INIT_BEGIN(rrdb)
    dsn_app_registration_rrdb();
MODULE_INIT_END

# endif
