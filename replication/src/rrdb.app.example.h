# pragma once
# include "rrdb.client.h"
# include "rrdb.client.perf.impl.h"
# include "rrdb.server.h"

namespace dsn { namespace apps { 

// client app example
class rrdb_client_app : 
    public ::dsn::service_app,
    public virtual ::dsn::clientlet
{
public:
    rrdb_client_app() {}
    
    ~rrdb_client_app() 
    {
        stop();
    }

    virtual ::dsn::error_code start(int argc, char** argv)
    {
        if (argc < 2)
            return ::dsn::ERR_INVALID_PARAMETERS;

        std::vector< ::dsn::rpc_address> meta_servers;
        ::dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);
        
        _rrdb_client.reset(new rrdb_client(meta_servers, argv[1]));
        _timer = ::dsn::tasking::enqueue_timer(LPC_RRDB_TEST_TIMER, this, [this]{on_test_timer();}, std::chrono::seconds(1));
        return ::dsn::ERR_OK;
    }

    virtual void stop(bool cleanup = false)
    {
        _timer->cancel(true);
 
        _rrdb_client.reset();
    }

    void on_test_timer()
    {
        // test for service rrdb
        {
            update_request req;
            //sync:
            error_code err;
            update_response resp;
            std::tie(err, resp) = _rrdb_client->put_sync(req);
            std::cout << "call RPC_RRDB_RRDB_PUT end, return " << err.to_string() << std::endl;
            //async: 
            //_rrdb_client->put(req, empty_callback);
           
        }
        {
            ::dsn::blob req;
            //sync:
            error_code err;
            update_response resp;
            std::tie(err, resp) = _rrdb_client->remove_sync(req);
            std::cout << "call RPC_RRDB_RRDB_REMOVE end, return " << err.to_string() << std::endl;
            //async: 
            //_rrdb_client->remove(req, empty_callback);
           
        }
        {
            ::dsn::blob req;
            //sync:
            error_code err;
            read_response resp;
            std::tie(err, resp) = _rrdb_client->get_sync(req);
            std::cout << "call RPC_RRDB_RRDB_GET end, return " << err.to_string() << std::endl;
            //async: 
            //_rrdb_client->get(req, empty_callback);
           
        }
    }

private:
    ::dsn::task_ptr _timer;
    ::dsn::rpc_address _server;
    
    std::unique_ptr<rrdb_client> _rrdb_client;
};

class rrdb_perf_test_client_app : 
    public ::dsn::service_app,
    public virtual ::dsn::clientlet
{
public:
    rrdb_perf_test_client_app() {}

    ~rrdb_perf_test_client_app()
    {
        stop();
    }

    virtual ::dsn::error_code start(int argc, char** argv)
    {
        if (argc < 2)
            return ::dsn::ERR_INVALID_PARAMETERS;

        std::vector< ::dsn::rpc_address> meta_servers;
        ::dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);

        _rrdb_client.reset(new rrdb_perf_test_client_impl(meta_servers, argv[1]));
        _rrdb_client->start_test();
        return ::dsn::ERR_OK;
    }

    virtual void stop(bool cleanup = false)
    {
        _rrdb_client.reset();
    }
    
private:
    std::unique_ptr<rrdb_perf_test_client> _rrdb_client;
};

} } 
