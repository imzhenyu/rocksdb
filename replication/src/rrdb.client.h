# pragma once
# include "rrdb.code.definition.h"
# include <iostream>


namespace dsn { namespace apps { 
class rrdb_client 
    : public virtual ::dsn::clientlet
{
public:
    rrdb_client(::dsn::rpc_address server) { _server = server; }
    rrdb_client() { }
    virtual ~rrdb_client() {}
    
 
    // ---------- call RPC_RRDB_RRDB_PUT ------------
    // - synchronous 
    std::pair< ::dsn::error_code, int> put_sync(
        const update_request& args,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0), 
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::wait_and_unwrap< int>(
            ::dsn::rpc::call(
                server_addr.unwrap_or(_server),
                RPC_RRDB_RRDB_PUT,
                args,
                nullptr,
                empty_callback,
                hash,
                timeout,
                0
                )
            );
    }
    
    // - asynchronous with on-stack update_request and int  
    template<typename TCallback>
    ::dsn::task_ptr put(
        const update_request& args,
        TCallback&& callback,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0),
        int reply_hash = 0,
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::call(
                    server_addr.unwrap_or(_server), 
                    RPC_RRDB_RRDB_PUT, 
                    args,
                    this,
                    std::forward<TCallback>(callback),
                    hash, 
                    timeout, 
                    reply_hash
                    );
    }
 
    // ---------- call RPC_RRDB_RRDB_REMOVE ------------
    // - synchronous 
    std::pair< ::dsn::error_code, int> remove_sync(
        const ::dsn::blob& args,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0), 
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::wait_and_unwrap< int>(
            ::dsn::rpc::call(
                server_addr.unwrap_or(_server),
                RPC_RRDB_RRDB_REMOVE,
                args,
                nullptr,
                empty_callback,
                hash,
                timeout,
                0
                )
            );
    }
    
    // - asynchronous with on-stack ::dsn::blob and int  
    template<typename TCallback>
    ::dsn::task_ptr remove(
        const ::dsn::blob& args,
        TCallback&& callback,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0),
        int reply_hash = 0,
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::call(
                    server_addr.unwrap_or(_server), 
                    RPC_RRDB_RRDB_REMOVE, 
                    args,
                    this,
                    std::forward<TCallback>(callback),
                    hash, 
                    timeout, 
                    reply_hash
                    );
    }
 
    // ---------- call RPC_RRDB_RRDB_MERGE ------------
    // - synchronous 
    std::pair< ::dsn::error_code, int> merge_sync(
        const update_request& args,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0), 
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::wait_and_unwrap< int>(
            ::dsn::rpc::call(
                server_addr.unwrap_or(_server),
                RPC_RRDB_RRDB_MERGE,
                args,
                nullptr,
                empty_callback,
                hash,
                timeout,
                0
                )
            );
    }
    
    // - asynchronous with on-stack update_request and int  
    template<typename TCallback>
    ::dsn::task_ptr merge(
        const update_request& args,
        TCallback&& callback,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0),
        int reply_hash = 0,
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::call(
                    server_addr.unwrap_or(_server), 
                    RPC_RRDB_RRDB_MERGE, 
                    args,
                    this,
                    std::forward<TCallback>(callback),
                    hash, 
                    timeout, 
                    reply_hash
                    );
    }
 
    // ---------- call RPC_RRDB_RRDB_GET ------------
    // - synchronous 
    std::pair< ::dsn::error_code, read_response> get_sync(
        const ::dsn::blob& args,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0), 
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::wait_and_unwrap< read_response>(
            ::dsn::rpc::call(
                server_addr.unwrap_or(_server),
                RPC_RRDB_RRDB_GET,
                args,
                nullptr,
                empty_callback,
                hash,
                timeout,
                0
                )
            );
    }
    
    // - asynchronous with on-stack ::dsn::blob and read_response  
    template<typename TCallback>
    ::dsn::task_ptr get(
        const ::dsn::blob& args,
        TCallback&& callback,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0),
        int reply_hash = 0,
        uint64_t hash = 0,
        dsn::optional< ::dsn::rpc_address> server_addr = dsn::none
        )
    {
        return ::dsn::rpc::call(
                    server_addr.unwrap_or(_server), 
                    RPC_RRDB_RRDB_GET, 
                    args,
                    this,
                    std::forward<TCallback>(callback),
                    hash, 
                    timeout, 
                    reply_hash
                    );
    }

private:
    ::dsn::rpc_address _server;
};

} } 