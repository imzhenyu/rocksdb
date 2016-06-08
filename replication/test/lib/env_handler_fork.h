#pragma once
#include <vector>
#include <dsn/service_api_cpp.h>
#include <env_handler.h>
#include "cluster_info.h"

namespace pegasus{
    namespace test{

struct env_fork_config
{
    // configuration
    //std::string zookeeper_app_dir;
    std::string app_dir;
    std::string app_file;
    std::string config_dir;
    std::string config_file;
    std::string working_directory_base;
    // int         zookeeper_count;
    uint32_t    meta_count;
    std::vector<dsn::rpc_address> meta_servers;
    uint32_t    replica_count;
    std::vector<dsn::rpc_address> replica_servers;
    std::string working_directory;
};

struct env_fork_object
{
    env_fork_object(){}
    uint32_t            _pid;
    dsn::rpc_address    _address;
    std::string         _dir;
    std::string         _app_path;
    char                _argv[4][100];
};

class env_handler_fork : public env_handler{
public:
    env_handler_fork();
    virtual ~env_handler_fork();

    virtual uint32_t meta_count() override { return _config->meta_count; }
    virtual uint32_t replica_count() override { return _config->replica_count; }

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
    virtual void prepare_env();
    uint32_t get_local_ipv4();

private:
    std::shared_ptr<env_fork_config> _config;
    env_fork_object* _meta;
    env_fork_object* _replica;
    cluster_info _info;
};

    }
}
