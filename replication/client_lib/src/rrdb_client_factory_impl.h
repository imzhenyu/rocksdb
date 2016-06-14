#pragma once
#include "rrdb_client.h"
#include "rrdb_error.h"
#include "rrdb_client_impl.h"

namespace dsn{ namespace apps{

class rrdb_client_factory_impl{
public:
    static bool initialize(const char* config_file);

    static irrdb_client* get_client(const char* cluster_name, const char* app_name);

private:
    typedef std::unordered_map<std::string, rrdb_client_impl*> app_to_client_map;
    typedef std::unordered_map<std::string, app_to_client_map> cluster_to_app_map;
    static cluster_to_app_map _cluster_to_clients;
    static dsn::service::zlock* _map_lock;
};

}} // namespace
