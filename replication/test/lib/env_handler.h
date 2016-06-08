#pragma once

#include <string>
#include <dsn/service_api_cpp.h>

namespace pegasus{
    namespace test {

class env_handler
{
public:
    env_handler(){}
    virtual ~env_handler(){}
    virtual uint32_t meta_count() = 0;
    virtual uint32_t replica_count() = 0;

    // TODO: add zookeeper control

    virtual dsn::error_code start_meta(int index) = 0;
    virtual dsn::error_code start_replica(int index) = 0;
    virtual dsn::error_code start_all() {return start_remain();}
    virtual dsn::error_code start_remain() = 0;

    virtual dsn::error_code kill_meta(int index) = 0;
    virtual dsn::error_code kill_meta(dsn::rpc_address addr) = 0;
    virtual dsn::error_code kill_meta_primary() = 0;
    virtual dsn::error_code kill_meta_all() = 0;
    virtual dsn::error_code kill_replica(dsn::rpc_address addr) = 0;
    virtual dsn::error_code kill_replica(int index) = 0;
    //virtual int kill_replica_primary(int app_id, int partition_index) = 0;
    virtual dsn::error_code kill_replica_all() = 0;
    virtual dsn::error_code kill_all() = 0;

    // virtual int wait_meta_work() = 0;
    // virtual int wait_replica_work(int app_id, int ) = 0;
};

}} // namespace
