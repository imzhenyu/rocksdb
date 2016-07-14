#include <atomic>
#include <unistd.h>
#include <memory>

#include "../proxy_lib/redis_parser.h"

using namespace dsn::proxy;

namespace dsn { namespace apps {

class proxy_app: public dsn::service_app
{
public:
    proxy_app(dsn_gpid gpid): service_app(gpid) {}
    virtual ~proxy_app() {}

    virtual dsn::error_code start(int argc, char **argv) override
    {
        if (argc < 2)
            return dsn::ERR_INVALID_PARAMETERS;
        proxy_session::factory f = [](proxy_stub* p, dsn::rpc_address remote) {
            return std::make_shared<redis::redis_parser>(p, remote);
        };
        _proxy.reset(new proxy_stub(f, argv[1]));
        return dsn::ERR_OK;
    }
    virtual dsn::error_code stop(bool) override
    {
        return dsn::ERR_OK;
    }
private:
    std::unique_ptr<dsn::proxy::proxy_stub> _proxy;
};

}}

void register_apps()
{
    dsn::register_app<dsn::apps::proxy_app>("proxy");
}

volatile int exit_flags(0);
void signal_handler(int signal_id)
{
    if (signal_id == SIGTERM)
    {
        dsn_exit(0);
    }
}

int main(int argc, char** argv)
{
    register_apps();
    signal(SIGTERM, signal_handler);

    if (argc == 1)
    {
        dsn_run_config("config.ini", false);
    }
    else
    {
        dsn_run(argc, argv, false);
    }

    while (exit_flags == 0)
    {
        pause();
    }
    return 0;
}
