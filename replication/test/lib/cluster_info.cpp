#include <dsn/dist/replication/replication_other_types.h>

#include "cluster_info.h"

using namespace dsn::replication;
using namespace dsn;

namespace pegasus {
    namespace test{

cluster_info::cluster_info()
{}

void cluster_info::init(const std::vector<dsn::rpc_address>& meta_servers)
{
    _meta_servers.assign_group(dsn_group_build("meta.servers"));
    for (auto& m : meta_servers)
        dsn_group_add(_meta_servers.group_handle(), m.c_addr());
}


cluster_info::~cluster_info()
{}

void cluster_info::end_meta_request(task_ptr callback, int retry_times, error_code err, dsn_message_t request, dsn_message_t resp)
{
    if(err == dsn::ERR_TIMEOUT && retry_times < 5)
    {
        rpc_address leader = dsn_group_get_leader(_meta_servers.group_handle());
        rpc_address next = dsn_group_next(_meta_servers.group_handle(), leader.c_addr());
        dsn_group_set_leader(_meta_servers.group_handle(), next.c_addr());

        rpc::call(
            _meta_servers,
            request,
            this,
            [=, callback_capture = std::move(callback)](error_code err, dsn_message_t request, dsn_message_t response)
            {
                end_meta_request(std::move(callback_capture), retry_times + 1, err, request, response);
            },
            0
         );
    }
    else
        callback->enqueue_rpc_response(err, resp);
}

dsn::error_code cluster_info::get_meta_primary(dsn::rpc_address& addr)
{
    std::shared_ptr<configuration_list_apps_request> req(new configuration_list_apps_request());
    req->status = AS_AVAILABLE;

    auto resp_task = request_meta<configuration_list_apps_request>(
            RPC_CM_LIST_APPS,
            req
    );
    resp_task->wait();
    if (resp_task->error() != dsn::ERR_OK)
    {
        return resp_task->error();
    }

    dsn::replication::configuration_list_apps_response resp;
    ::unmarshall(resp_task->response(), resp);
    if(resp.err != dsn::ERR_OK)
    {
        return resp.err;
    }

    addr = dsn_group_get_leader(_meta_servers.group_handle());
    return ERR_OK;
}

}}
