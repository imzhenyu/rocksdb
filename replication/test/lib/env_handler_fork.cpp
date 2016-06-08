#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <dsn/service_api_c.h>
#include <dsn/service_api_cpp.h>
#include <dsn/internal/configuration.h>

#include "env_handler_fork.h"

using namespace dsn;

namespace pegasus {
    namespace test{

env_handler_fork::env_handler_fork()
{
    read_config();
    prepare_env();
}

env_handler_fork::~env_handler_fork()
{
    delete[] _meta;
    delete[] _replica;
}

void env_handler_fork::read_config()
{
    const char* section = "pegasus.test";
    _config = std::shared_ptr<env_fork_config>(new env_fork_config);

    _config->app_dir = dsn_config_get_value_string(section, "app_dir", "", "app file full path");
    dassert(!_config->app_dir.empty(), "config app_dir not found");
    _config->app_file = dsn_config_get_value_string(section, "app_file", "", "app file full path");
    dassert(!_config->app_file.empty(), "config app_file not found");
    _config->config_dir = dsn_config_get_value_string(section, "config_dir", "", "app file full path");
    dassert(!_config->config_dir.empty(), "config app_dir not found");
    _config->config_file = dsn_config_get_value_string(section, "config_file", "", "target config file full path");
    dassert(!_config->config_file.empty(), "config_file not found");
    _config->working_directory_base = dsn_config_get_value_string(section, "working_directory_base", "", "working_directory_base");
    dassert(!_config->working_directory_base.empty(), "working directory base not found");

    char buffer[80];
    time_t rawtime;
    time (&rawtime);
    struct tm * timeinfo = localtime(&rawtime);
    strftime(buffer,80,"%F_%T",timeinfo);
    std::string str(buffer);
    _config->working_directory = _config->working_directory_base + "/" + str;

    uint32_t ipv4 = get_local_ipv4();

    dsn::configuration_ptr target_config(new dsn::configuration());
    int ret = target_config->load((_config->config_dir + "/" + _config->config_file).c_str());
    dassert(ret, "load config failed");

    uint16_t meta_init_port = (uint16_t)target_config->get_value<uint64_t>("apps.meta", "ports", 0, "meta server port");
    dassert(meta_init_port != 0, "no [apps.meta][ports]");
    _config->meta_count = (uint32_t)target_config->get_value<uint64_t>("apps.meta", "count", 0, "meta server count");
    dassert(_config->meta_count > 0, "no [apps.meta][count]");
    for (int i = 0; i < _config->meta_count; i++)
    {
        ::dsn::rpc_address ep(ipv4, meta_init_port + i);
        _config->meta_servers.push_back(ep);
    }

    uint16_t replica_init_port = (uint16_t)target_config->get_value<uint64_t>("apps.replica", "ports", 0, "replica server port");
    dassert(replica_init_port != 0, "no [apps.replica][ports]");
    _config->replica_count = (uint32_t)target_config->get_value<uint64_t>("apps.replica", "count", 0, "replica server count");
    dassert(_config->replica_count > 0, "no [apps.replica][count]");
    for (int i = 0; i < _config->replica_count; i++)
    {
        ::dsn::rpc_address ep(ipv4, replica_init_port + i);
        _config->replica_servers.push_back(ep);
    }
}

void env_handler_fork::prepare_env()
{
    _info.init(_config->meta_servers);

    char buffer[200], command[200];
    boost::filesystem::create_directories(_config->working_directory);
    boost::filesystem::permissions(_config->working_directory, boost::filesystem::all_all);

    int ret;
    _meta = new env_fork_object[_config->meta_count];
    for(int i = 0; i < _config->meta_count ; i++)
    {
        sprintf(buffer, "%s/meta%d", _config->working_directory.c_str(), i+1);
        ret = mkdir(buffer , 0755);
        dassert(ret == 0, "");

        sprintf(command, "cp -r %s/%s %s", _config->app_dir.c_str(), _config->app_file.c_str(), buffer);
        system(command);
        sprintf(command, "cp -r %s/%s %s", _config->config_dir.c_str(), _config->config_file.c_str(), buffer);
        system(command);

        _meta[i]._pid = 0;
        _meta[i]._address = _config->meta_servers[i];
        _meta[i]._dir.assign(buffer);
        _meta[i]._app_path = _meta[i]._dir + "/" + _config->app_file;

        sprintf(_meta[i]._argv[0], "%s", _config->app_file.c_str());
        sprintf(_meta[i]._argv[1], "%s", _config->config_file.c_str());
        sprintf(_meta[i]._argv[2], "-app_list");
        sprintf(_meta[i]._argv[3], "meta@%d", i+1);
    }

    _replica = new env_fork_object[_config->replica_count];
    for(int i = 0; i < _config->replica_count; i++)
    {
        sprintf(buffer, "%s/replica%d", _config->working_directory.c_str(), i + 1);
        ret = mkdir(buffer , 0755);
        dassert(ret == 0, "");

        sprintf(command, "cp -r %s/%s %s", _config->app_dir.c_str(), _config->app_file.c_str(), buffer);
        system(command);
        sprintf(command, "cp -r %s/%s %s", _config->config_dir.c_str(), _config->config_file.c_str(), buffer);
        system(command);

        _replica[i]._pid = 0;
        _replica[i]._address = _config->replica_servers[i];
        _replica[i]._dir.assign(buffer);
        _replica[i]._app_path = _replica[i]._dir + "/" + _config->app_file;

        sprintf(_replica[i]._argv[0], "%s", _config->app_file.c_str());
        sprintf(_replica[i]._argv[1], "%s", _config->config_file.c_str());
        sprintf(_replica[i]._argv[2], "-app_list");
        sprintf(_replica[i]._argv[3], "replica@%d", i+1);
    }

}

dsn::error_code env_handler_fork::start_meta(int index)
{
    dassert(index >= 1 && index <= _config->meta_count && _meta[index-1]._pid == 0, "invalid meta index:%d", index);
    index--;
    int pid = fork();
    if(pid == 0)
    {
        chdir(_meta[index]._dir.c_str());
        execl(_meta[index]._app_path.c_str(),
            _meta[index]._argv[0],
            _meta[index]._argv[1],
            _meta[index]._argv[2],
            _meta[index]._argv[3],
            (char *)0);
        exit(0);
    }
    else
    {
        _meta[index]._pid = pid;
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::start_replica(int index)
{
    dassert(index >=1 && index <= _config->replica_count && _replica[index-1]._pid == 0, "invalid replica index:%d", index);
    index--;
    int pid = fork();
    if(pid == 0)
    {
        chdir(_replica[index]._dir.c_str());

        execl(_replica[index]._app_path.c_str(),
                _replica[index]._argv[0],
                _replica[index]._argv[1],
                _replica[index]._argv[2],
                _replica[index]._argv[3],
                (char *)0);
        exit(0);
    }
    else
    {
        _replica[index]._pid = pid;
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::start_remain()
{
    for(int i = 0; i < _config->meta_count; i++)
    {
        if(_meta[i]._pid == 0)
            start_meta(i+1);
    }
    for(int i = 0; i < _config->replica_count; i++)
    {
        if(_replica[i]._pid == 0)
            start_replica(i+1);
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::kill_meta(int index)
{
    dassert(index >= 1 && index <= _config->meta_count && _meta[index-1]._pid != 0, "invalid index");
    index--;
    kill(_meta[index]._pid, SIGKILL);
    int status;
    waitpid(_meta[index]._pid, &status, 0);
    _meta[index]._pid = 0;
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::kill_meta_all()
{
    for(int i = 0; i < _config->meta_count; i++)
    {
        if(_meta[i]._pid != 0)
            kill_meta(i+1);
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::kill_meta_primary()
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

dsn::error_code env_handler_fork::kill_meta(dsn::rpc_address addr)
{
    for(int i = 0; i < _config->meta_count; i++)
    {
        if(_meta[i]._address == addr)
        {
            return kill_meta(i+1);
        }
    }
    return dsn::ERR_OK;
}



dsn::error_code env_handler_fork::kill_replica(dsn::rpc_address addr)
{
    for(int i = 0; i < _config->replica_count; i++)
    {
        if(_replica[i]._address == addr)
        {
            return kill_replica(i+1);
        }
    }
    return dsn::ERR_OK;
}


dsn::error_code env_handler_fork::kill_replica(int index)
{
    dassert(index >= 1 && index <= _config->replica_count && _replica[index-1]._pid != 0, "invalid index");
    index--;
    kill(_replica[index]._pid, SIGKILL);
    int status;
    waitpid(_replica[index]._pid, &status, 0);
    _replica[index]._pid = 0;
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::kill_replica_all()
{
    for(int i = 0; i < _config->replica_count; i++)
    {
        if(_replica[i]._pid != 0)
            kill_replica(i+1);
    }
    return dsn::ERR_OK;
}

dsn::error_code env_handler_fork::kill_all()
{
    kill_meta_all();
    kill_replica_all();
    return dsn::ERR_OK;
}

uint32_t env_handler_fork::get_local_ipv4()
{
    static const char* inteface = "eth0";

    uint32_t ip = dsn_ipv4_local(inteface);
    if (0 == ip)
    {
        char name[128];
        gethostname(name, sizeof(name));
        ip = dsn_ipv4_from_host(name);
    }
    return ip;
}

}}
