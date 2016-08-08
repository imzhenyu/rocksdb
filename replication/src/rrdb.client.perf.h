# pragma once

# include "rrdb.client.h"

namespace dsn { namespace apps { 
class rrdb_perf_test_client
    : public rrdb_client,
      public ::dsn::service::perf_client_helper
{
public:
    rrdb_perf_test_client(::dsn::rpc_address server)
        : rrdb_client(server)
    {
    }

    virtual void send_one(int payload_bytes, int key_space_size, const std::vector<double>& ratios) override
    {
        auto prob = (double)dsn_random32(0, 1000) / 1000.0;
        if (0) {}
        else if (prob <= ratios[0])
        {
            send_one_put(payload_bytes, key_space_size);
        }
        else if (prob <= ratios[1])
        {
            send_one_remove(payload_bytes, key_space_size);
        }
        else if (prob <= ratios[2])
        {
            send_one_get(payload_bytes, key_space_size);
        }
        else { /* nothing to do */ }
    }

    void send_one_put(int payload_bytes, int key_space_size)
    {
        update_request req;
        // TODO: randomize the value of req
        auto rs = random64(0, 10000000) % key_space_size;
        req.key = dsn::blob((const char*)&rs, 0, sizeof(rs));
        
        auto buffer = std::shared_ptr<char>((char*)dsn_transient_malloc(payload_bytes), [](char* ptr){
                dsn_transient_free(ptr);
        });
        req.value = dsn::blob(buffer, payload_bytes);

        put(
            req,
            [this, context = prepare_send_one()](error_code err, int&& resp)
            {
                end_send_one(context, err);
            },
            _timeout, rs
            );
    }

    void send_one_remove(int payload_bytes, int key_space_size)
    {
        ::dsn::blob req;
        auto rs = random64(0, 10000000) % key_space_size;
        req = dsn::blob((const char*)&rs, 0, sizeof(rs));

        remove(
            req,
            [this, context = prepare_send_one()](error_code err, int&& resp)
            {
                end_send_one(context, err);
            },
            _timeout, rs
            );
    }

    void send_one_get(int payload_bytes, int key_space_size)
    {
        ::dsn::blob req;
        auto rs = random64(0, 10000000) % key_space_size;
        req = dsn::blob((const char*)&rs, 0, sizeof(rs));
        get(
            req,
            [this, context = prepare_send_one()](error_code err, read_response&& resp)
            {
                end_send_one(context, err);
            },
            _timeout, rs
            );
    }
};

} } 
