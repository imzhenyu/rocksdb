# pragma once
# include "rrdb.client.perf.h"

namespace dsn { namespace apps {  

class rrdb_perf_test_client_impl : public rrdb_perf_test_client
{
public:
    using rrdb_perf_test_client::rrdb_perf_test_client;

    virtual void send_one_put(int payload_bytes, int key_space_size) override
    {
        update_request req;

        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer(payload_bytes < 128 ? 128 : payload_bytes);
        writer.write("key.", 4);
        writer.write(rs);
        req.key = writer.get_buffer();

        while (writer.total_size() < payload_bytes)
        {
            const char* bf = "@@@@@@@@@@";
            int sz = std::min(10, payload_bytes - writer.total_size());
            writer.write(bf, sz);
        }
        req.value = writer.get_buffer();

        put(
            req,
            [this, context = prepare_send_one()](error_code err, int&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }

    virtual void send_one_remove(int payload_bytes, int key_space_size) override
    {
        ::dsn::blob req;

        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer;
        writer.write("key.", 4);
        writer.write(rs);
        req = writer.get_buffer();

        remove(
            req,
            [this, context = prepare_send_one()](error_code err, int32_t&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }

    virtual void send_one_get(int payload_bytes, int key_space_size) override
    {
        ::dsn::blob req;

        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer;
        writer.write("key.", 4);
        writer.write(rs);
        req = writer.get_buffer();
        
        get(
            req,
            [this, context = prepare_send_one()](error_code err, read_response&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }
};

} } 
