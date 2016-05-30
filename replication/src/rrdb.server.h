# pragma once
# include "rrdb.code.definition.h"
# include <iostream>

namespace dsn { namespace apps { 
class rrdb_service 
    : public ::dsn::serverlet< rrdb_service>
{
public:
    rrdb_service() : ::dsn::serverlet< rrdb_service>("rrdb") {}
    virtual ~rrdb_service() {}

protected:
    // all service handlers to be implemented further
    // RPC_RRDB_RRDB_PUT 
    virtual void on_put(const update_request& args, ::dsn::rpc_replier< int>& reply)
    {
        std::cout << "... exec RPC_RRDB_RRDB_PUT ... (not implemented) " << std::endl;
        int resp;
        reply(resp);
    }
    // RPC_RRDB_RRDB_REMOVE 
    virtual void on_remove(const ::dsn::blob& args, ::dsn::rpc_replier< int>& reply)
    {
        std::cout << "... exec RPC_RRDB_RRDB_REMOVE ... (not implemented) " << std::endl;
        int resp;
        reply(resp);
    }
    // RPC_RRDB_RRDB_MERGE 
    virtual void on_merge(const update_request& args, ::dsn::rpc_replier< int>& reply)
    {
        std::cout << "... exec RPC_RRDB_RRDB_MERGE ... (not implemented) " << std::endl;
        int resp;
        reply(resp);
    }
    // RPC_RRDB_RRDB_GET 
    virtual void on_get(const ::dsn::blob& args, ::dsn::rpc_replier< read_response>& reply)
    {
        std::cout << "... exec RPC_RRDB_RRDB_GET ... (not implemented) " << std::endl;
        read_response resp;
        reply(resp);
    }
    
public:
    void open_service(dsn_gpid gpid)
    {
        this->register_async_rpc_handler(RPC_RRDB_RRDB_PUT, "put", &rrdb_service::on_put, gpid);
        this->register_async_rpc_handler(RPC_RRDB_RRDB_REMOVE, "remove", &rrdb_service::on_remove, gpid);
        this->register_async_rpc_handler(RPC_RRDB_RRDB_MERGE, "merge", &rrdb_service::on_merge, gpid);
        this->register_async_rpc_handler(RPC_RRDB_RRDB_GET, "get", &rrdb_service::on_get, gpid);
    }

    void close_service(dsn_gpid gpid)
    {
        this->unregister_rpc_handler(RPC_RRDB_RRDB_PUT, gpid);
        this->unregister_rpc_handler(RPC_RRDB_RRDB_REMOVE, gpid);
        this->unregister_rpc_handler(RPC_RRDB_RRDB_MERGE, gpid);
        this->unregister_rpc_handler(RPC_RRDB_RRDB_GET, gpid);
    }
};

} } 