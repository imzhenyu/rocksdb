#pragma once
#include <dsn/service_api_cpp.h>
#include <dsn/dist/replication.h>

namespace pegasus {
    namespace test{

class cluster_info : public dsn::clientlet
{
public:
    cluster_info();
    void init(const std::vector<dsn::rpc_address>& meta_servers);
    virtual ~cluster_info();

    dsn::error_code get_meta_primary(dsn::rpc_address& addr);

private:

    void end_meta_request(dsn::task_ptr callback, int retry_times, dsn::error_code err, dsn_message_t request, dsn_message_t resp);

    template<typename TRequest>
    dsn::task_ptr request_meta(
            dsn_task_code_t code,
            std::shared_ptr<TRequest>& req,
            int timeout_milliseconds= 0,
            int reply_hash = 0
            )
    {
        dsn_message_t msg = dsn_msg_create_request(code, timeout_milliseconds, 0);
        dsn::task_ptr task = ::dsn::rpc::create_rpc_response_task(msg, nullptr, [](dsn::error_code, dsn_message_t, dsn_message_t) {}, reply_hash);
        ::marshall(msg, *req);
        dsn::rpc::call(
            _meta_servers,
            msg,
            this,
            [this, task] (dsn::error_code err, dsn_message_t request, dsn_message_t response)
            {
                end_meta_request(std::move(task), 0, err, request, response);
            },
            0
         );
        return task;
    }

private:
    dsn::rpc_address _meta_servers;
};

}}
