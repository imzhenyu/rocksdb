#include <iostream>
#include <fstream>
#include <streambuf>
#include <arpa/inet.h>
#include "env_handler_supervisor.h"


using namespace dsn;

namespace pegasus {
    namespace test{

env_handler_supervisor::env_handler_supervisor()
{
    read_config();
}

env_handler_supervisor::~env_handler_supervisor()
{}

void env_handler_supervisor::read_config()
{
    const char* section = "pegasus.test";
    _use_proxy = dsn_config_get_value_bool(section, "use_proxy", false, "use proxy");
    if(_use_proxy)
    {
        _proxy = dsn_config_get_value_string(section, "proxy", "", "proxy");
        dassert(!_proxy.empty(), "");
    }
    _user = dsn_config_get_value_string(section, "user", "", "user");
    _password = dsn_config_get_value_string(section, "password", "", "password");
    _cluster = dsn_config_get_value_string(section, "cluster", "", "cluster");

    std::string meta_list_string = dsn_config_get_value_string(section, "meta_list", "", "meta list");
    std::list<std::string> lv;
    ::dsn::utils::split_args(meta_list_string.c_str(), lv, ',');
    for (auto& s : lv)
    {
        // name:port
        auto pos1 = s.find_first_of(':');
        if (pos1 != std::string::npos)
        {
            ::dsn::rpc_address ep(s.substr(0, pos1).c_str(), atoi(s.substr(pos1 + 1).c_str()));
            _meta_servers.push_back(ep);
            _meta_status.push_back(true);
        }
    }
    dassert(_meta_servers.size() > 0, "");

    std::string replica_list_string = dsn_config_get_value_string(section, "replica_list", "", "replica list");
    ::dsn::utils::split_args(replica_list_string.c_str(), lv, ',');
    for (auto& s : lv)
    {
        // name:port
        auto pos1 = s.find_first_of(':');
        if (pos1 != std::string::npos)
        {
            ::dsn::rpc_address ep(s.substr(0, pos1).c_str(), atoi(s.substr(pos1 + 1).c_str()));
            _replica_servers.push_back(ep);
            _replica_status.push_back(true);
        }
    }
    dassert(_replica_servers.size() > 0, "");

    _info.init(_meta_servers);
}


dsn::error_code env_handler_supervisor::start_meta(int index)
{
    index--;
    dassert(index >= 0 && index < _meta_servers.size() && _meta_status[index] == false, "invalid meta index:%d", index);
    dsn::error_code err = start_task("meta", _meta_servers[index]);
    if(err == ERR_OK)
    {
        _meta_status[index] = true;
    }
    return err;
}

dsn::error_code env_handler_supervisor::start_replica(int index)
{
    index--;
    dassert(index >= 0 && index < _replica_servers.size() && _replica_status[index] == false, "invalid meta index:%d", index);
    dsn::error_code err = start_task("replica", _replica_servers[index]);
    if(err == ERR_OK)
    {
        _replica_status[index] = true;
    }
    return err;
}

dsn::error_code env_handler_supervisor::start_remain()
{

    for(int i = 0; i < _meta_servers.size(); i++)
    {
        if(!_meta_status[i])
            start_meta(i+1);
    }
    for(int i = 0; i < _replica_servers.size(); i++)
    {
        if(!_replica_status[i])
            start_replica(i+1);
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_supervisor::kill_meta(int index)
{
    index--;
    dassert(index >= 0 && index < _meta_servers.size() && _meta_status[index] == true, "invalid meta index:%d", index);
    dsn::error_code err = stop_task("meta", _meta_servers[index]);
    if(err == ERR_OK)
    {
        _meta_status[index] = false;
    }
    return err;
}

dsn::error_code env_handler_supervisor::kill_meta_all()
{
    for(int i = 0; i < _meta_servers.size(); i++)
    {
        if(_meta_status[i])
            kill_meta(i+1);
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_supervisor::kill_meta_primary()
{
    dsn::rpc_address addr;
    dsn::error_code code = _info.get_meta_primary(addr);
    if(code == ERR_OK)
    {
        return kill_meta(addr);
    }
    else
    {
        std::cout << "!!!can't find primary, err:" << code.to_string() << std::endl;
        return code;
    }
}


dsn::error_code env_handler_supervisor::kill_meta(dsn::rpc_address addr)
{
    for(int i = 0; i < _meta_servers.size(); i++)
    {
        if(_meta_servers[i] == addr)
        {
            return kill_meta(i+1);
        }
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_supervisor::kill_replica(dsn::rpc_address addr)
{
    for(int i = 0; i < _replica_servers.size(); i++)
    {
        if(_replica_servers[i] == addr)
        {
            return kill_replica(i+1);
        }
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_supervisor::kill_replica(int index)
{
    index--;
    dassert(index >= 0 && index < _replica_servers.size() && _replica_status[index] == true, "invalid meta index:%d", index);
    dsn::error_code err = stop_task("replica", _replica_servers[index]);
    if(err == ERR_OK)
    {
        _replica_status[index] = false;
    }
    return err;
}

dsn::error_code env_handler_supervisor::kill_replica_all()
{
    for(int i = 0; i < _replica_servers.size(); i++)
    {
        if(_replica_status[i])
            kill_replica(i+1);
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_supervisor::kill_all()
{
    kill_meta_all();
    kill_replica_all();
    return dsn::ERR_OK;
}

dsn::error_code env_handler_supervisor::start_task(const char* job, dsn::rpc_address addr)
{
    std::stringstream command;
    command << "wget --user " << _user <<" --password " << _password << (_use_proxy?std::string(" -e \"http_proxy=")+_proxy+"\"":"")
            << " \"http://" << get_ip(addr) << ":9001/index.html?processname=pegasus--" << _cluster << "--" << job << "&action=start\" -O result.html";

    std::cout << "command:" << command.str() << std::endl;
    int ret = system(command.str().c_str());
    if(ret != 0)
    {
        return dsn::ERR_INACTIVE_STATE;
    }

    std::ifstream t("result.html");
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    std::stringstream expect_message;
    expect_message << "Process pegasus--" << _cluster << "--" << job << " started";

    std::size_t found = str.find(expect_message.str());
    if(found!=std::string::npos)
    {
        return ERR_OK;
    }
    return ERR_INACTIVE_STATE;

}

dsn::error_code env_handler_supervisor::stop_task(const char* job, dsn::rpc_address addr)
{
    std::stringstream command;
    command << "wget --user " << _user <<" --password " << _password << (_use_proxy?std::string(" -e \"http_proxy=")+_proxy+"\"":"")
            << " \"http://" << get_ip(addr) << ":9001/index.html?processname=pegasus--" << _cluster << "--" << job << "&action=stop\" -O result.html";

    std::cout << "command:" << command.str() << std::endl;
    int ret = system(command.str().c_str());
    if(ret != 0)
    {
        return dsn::ERR_INACTIVE_STATE;
    }

    std::ifstream t("result.html");
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    std::stringstream expect_message;
    expect_message << "Process pegasus--" << _cluster << "--" << job << " stopped";

    std::size_t found = str.find(expect_message.str());
    if(found!=std::string::npos)
    {
        return ERR_OK;
    }
    return ERR_INACTIVE_STATE;
}

std::string env_handler_supervisor::get_ip(dsn::rpc_address addr)
{
    struct in_addr ip_addr;
    ip_addr.s_addr = htonl(addr.ip());
    return std::string(inet_ntoa(ip_addr));
}

}}
