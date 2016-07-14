#include "rrdb_client_factory_impl.h"

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "rrdb.client.factory.impl"

namespace dsn{ namespace apps{

std::unordered_map<std::string, rrdb_client_factory_impl::app_to_client_map> rrdb_client_factory_impl::_cluster_to_clients;
dsn::service::zlock* rrdb_client_factory_impl::_map_lock;

bool rrdb_client_factory_impl::initialize(const char* config_file)
{
    //use config file to run
    char exe[] = "client";
    char config[1024];
    sprintf(config, "%s", config_file);
    char* argv[] = { exe, config};
    dsn_run(2, argv, false);
    rrdb_client_impl::init_error();
    _map_lock = new dsn::service::zlock();
    return true;
}

irrdb_client* rrdb_client_factory_impl::get_client(const char* cluster_name, const char* app_name)
{
    if (cluster_name == nullptr || cluster_name[0] == '\0') {
        derror("invalid parameter 'cluster_name'");
        return nullptr;
    }
    if (app_name == nullptr || app_name[0] == '\0') {
        derror("invalid parameter 'app_name'");
        return nullptr;
    }

    service::zauto_lock l(*_map_lock);
    auto it = _cluster_to_clients.find(cluster_name);
    if (it == _cluster_to_clients.end())
    {
        it = _cluster_to_clients.insert(cluster_to_app_map::value_type(cluster_name, app_to_client_map())).first;
    }

    app_to_client_map& app_to_clients = it->second;
    auto it2 = app_to_clients.find(app_name);
    if (it2 == app_to_clients.end())
    {
        rrdb_client_impl* client = new rrdb_client_impl(cluster_name, app_name);
        it2 = app_to_clients.insert(app_to_client_map::value_type(app_name, client)).first;
    }

    return it2->second;
}

}}
