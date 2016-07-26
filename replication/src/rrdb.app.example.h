# pragma once
# include "rrdb.client.h"
# include "rrdb.client.perf.h"
# include "rrdb.server.impl.h"
# include <memory>

namespace dsn { namespace apps { 
// server app example
class rrdb_server_app : 
    public ::dsn::service_app
{
public:
    rrdb_server_app(dsn_gpid gpid)
        : ::dsn::service_app(gpid) {}

    virtual ::dsn::error_code start(int argc, char** argv) override
    {
        _rrdb_svc.open_service(gpid());
        return ::dsn::ERR_OK;
    }

    virtual ::dsn::error_code stop(bool cleanup = false) override
    {
        _rrdb_svc.close_service(gpid());
        return ::dsn::ERR_OK;
    }

private:
    rrdb_service _rrdb_svc;
};

// client app example
class rrdb_client_app : 
    public ::dsn::service_app, 
    public virtual ::dsn::clientlet
{
public:
    rrdb_client_app(dsn_gpid gpid)
        : ::dsn::service_app(gpid) {}
        
    ~rrdb_client_app() 
    {
        stop();
    }

    virtual ::dsn::error_code start(int argc, char** argv) override
    {
        if (argc < 3)
        {
            printf ("Usage: <exe> server-host:server-port/service-url test_case\n");
            return ::dsn::ERR_INVALID_PARAMETERS;
        }

        // argv[1]: e.g., dsn://mycluster/simple-kv.instance0
        _server = url_host_address(argv[1]);

        _rrdb_client.reset(new rrdb_client(_server));

        if (strcmp(argv[2], "simple_test")==0)
        {
            dsn::tasking::enqueue(LPC_RRDB_TEST_TIMER, this, [this](){ on_simple_test(1, 2); });
            dsn::tasking::enqueue(LPC_RRDB_TEST_TIMER, this, [this](){ on_simple_test(2, 2); });
        }
        else if (strcmp(argv[2], "ttl_test")==0)
        {
            dsn::tasking::enqueue(LPC_RRDB_TEST_TIMER, this, [this](){ on_ttl_test(); });
        }
        else
        {
            return dsn::ERR_INVALID_PARAMETERS;
        }
        return ::dsn::ERR_OK;
    }

    virtual ::dsn::error_code stop(bool cleanup = false) override
    {
        _rrdb_client.reset();
        return ::dsn::ERR_OK;
    }

    void on_ttl_test()
    {
        size_t length = 128;
        std::shared_ptr<char> buffer1(new char[length], std::default_delete<char[]>());
        std::shared_ptr<char> buffer2(new char[length], std::default_delete<char[]>());

        char* pkey = buffer1.get();
        char* pvalue = buffer2.get();

        strcpy(pkey, "hellohello");
        strcpy(pvalue, "worldworld");

        int key_length = strlen(pkey);
        int val_length = strlen(pvalue);

        //don't set expire ts
        dsn::apps::update_request request;
        request.key.assign(pkey, 0, key_length);
        request.value.assign(pvalue, 0, val_length);
        do {
            std::pair<error_code, update_response> result = _rrdb_client->put_sync(request);
            if (result.first == ERR_OK)
                break;
        } while (true);
        std::cout << "set key with expire_ts 0 ok" << std::endl;

        //expect we can read it
        std::this_thread::sleep_for(std::chrono::seconds(2));
        do {
            std::pair<error_code, read_response> result = _rrdb_client->get_sync(blob(pkey, 0, key_length));
            if (result.first == ERR_OK)
            {
                dassert(result.second.error==0, "");
                std::string val(result.second.value.data(), result.second.value.length());
                std::cout << "read key " << pkey << " ok, got value " << val << std::endl;
                break;
            }
        } while (true);

        //set ttl for a value
        request.expire_ts = 5000;
        do {
            std::pair<error_code, update_response> result = _rrdb_client->put_sync(request);
            if (result.first == ERR_OK)
                break;
        } while (true);
        std::cout << "set key with expire ts " << request.expire_ts << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(8));
        do {
            std::pair<error_code, read_response> result = _rrdb_client->get_sync(blob(pkey, 0, key_length));
            if (result.first == ERR_OK) {
                dassert(result.second.error!=0, "");
                std::cout << "get the value failed, should be expired" << std::endl;
                break;
            }
        } while (true);

        dsn_exit(0);
    }

    void on_simple_test(int start, int gap)
    {
        size_t length = 128;
        std::shared_ptr<char> buffer1(new char[length], std::default_delete<char[]>());
        std::shared_ptr<char> buffer2(new char[length], std::default_delete<char[]>());

        char* pkey = buffer1.get();
        char* pvalue = buffer2.get();
        while (true)
        {
            snprintf(pkey, length, "hello%d", start);
            snprintf(pvalue, length, "world%d", start);
            int key_length = strlen(pkey);
            int value_length = strlen(pvalue);

            bool set_ok = false;
            {
                dsn::apps::update_request request;
                request.key.assign(pkey, 0, key_length);
                request.value.assign(pvalue, 0, value_length);
                std::pair<error_code, update_response> result = _rrdb_client->put_sync(request);
                std::cout << "call rrdb put " << pkey << ", return " << result.first.to_string() << std::endl;
                if (result.first == ERR_OK && result.second.error==0)
                {
                    set_ok = true;
                }
            }

            if (set_ok)
            {
                std::pair<error_code, read_response> result =
                        _rrdb_client->get_sync(dsn::blob(pkey, 0, key_length));
                std::cout << "call rrdb get " << pkey << ", return " << result.first.to_string() << std::endl;
                if (result.first == ERR_OK)
                {
                    dassert(result.second.error==0, "");
                    dsn::blob& get_value = result.second.value;
                    dassert(memcmp(get_value.data(), pvalue, value_length) == 0, "");
                }
            }

            bool remove_ok = false;
            {
                std::pair<error_code, update_response> result =
                        _rrdb_client->remove_sync(dsn::blob(pkey, 0, key_length));
                std::cout << "call rrdb remove " << pkey << ", return " << result.first.to_string() << std::endl;
                if (result.first == ERR_OK && result.second.error==0)
                {
                    remove_ok = true;
                }
            }

            if (remove_ok)
            {
                std::pair<error_code, read_response> result =
                        _rrdb_client->get_sync(dsn::blob(pkey, 0, key_length));
                std::cout << "call rrdb get " << pkey << " after remove, return " << result.first.to_string() << std::endl;
                if (result.first==ERR_OK)
                {
                    dassert(result.second.error!=0, "");
                }
            }

            start+=gap;
        }
    }

private:
    ::dsn::task_ptr _timer;
    ::dsn::url_host_address _server;
    
    std::unique_ptr<rrdb_client> _rrdb_client;
};

class rrdb_perf_test_client_app :
    public ::dsn::service_app, 
    public virtual ::dsn::clientlet
{
public:
    rrdb_perf_test_client_app(dsn_gpid gpid)
        : ::dsn::service_app(gpid)
    {
        _rrdb_client = nullptr;
    }

    ~rrdb_perf_test_client_app()
    {
        stop();
    }

    virtual ::dsn::error_code start(int argc, char** argv) override
    {
        if (argc < 1)
            return ::dsn::ERR_INVALID_PARAMETERS;

        // argv[1]: e.g., dsn://mycluster/simple-kv.instance0
        _server = ::dsn::url_host_address(argv[1]);

        _rrdb_client = new rrdb_perf_test_client(_server);
        _rrdb_client->start_test("rrdb.perf-test.case.", 3);
        return ::dsn::ERR_OK;
    }

    virtual ::dsn::error_code stop(bool cleanup = false) override
    {
        if (_rrdb_client != nullptr)
        {
            delete _rrdb_client;
            _rrdb_client = nullptr;
        }
        
        return ::dsn::ERR_OK;
    }
    
private:
    rrdb_perf_test_client *_rrdb_client;
    ::dsn::rpc_address _server;
};

} } 
