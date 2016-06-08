#pragma once
#include <dsn/service_api_cpp.h>
#include <env_handler.h>
#include <cluster_info.h>

namespace pegasus{
    namespace test{

struct env_supervisor_object
{
    env_supervisor_object(){}
    bool                _is_running;
    dsn::rpc_address    _address;
};

class env_handler_supervisor: public env_handler{
public:
    env_handler_supervisor();
    virtual ~env_handler_supervisor();

    virtual uint32_t meta_count() override { return _meta_servers.size(); }
    virtual uint32_t replica_count() override { return _replica_servers.size(); }

    // start
    virtual dsn::error_code start_meta(int index) override;
    virtual dsn::error_code start_replica(int index) override;
    virtual dsn::error_code start_remain() override;

    // kill
    virtual dsn::error_code kill_meta(int index) override;
    virtual dsn::error_code kill_meta(dsn::rpc_address addr) override;
    virtual dsn::error_code kill_meta_primary() override;
    virtual dsn::error_code kill_meta_all() override;
    virtual dsn::error_code kill_replica(int index) override;
    virtual dsn::error_code kill_replica(dsn::rpc_address addr) override;
    virtual dsn::error_code kill_replica_all() override;
    virtual dsn::error_code kill_all() override;
private:
    void read_config();
    dsn::error_code start_task(const char* job, dsn::rpc_address);
    dsn::error_code stop_task(const char* job, dsn::rpc_address);
    std::string get_ip(dsn::rpc_address addr);

private:
    bool _use_proxy;
    std::string _proxy;
    std::string _user;
    std::string _password;
    std::string _cluster;
    std::vector<dsn::rpc_address> _meta_servers;
    std::vector<dsn::rpc_address> _replica_servers;
    std::vector<bool> _meta_status;
    std::vector<bool> _replica_status;
    cluster_info _info;
};

}}
