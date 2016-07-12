#ifndef PROXY_LAYER_H
#define PROXY_LAYER_H

#include <dsn/service_api_cpp.h>
#include <dsn/cpp/zlocks.h>
#include <unordered_map>
#include <functional>

namespace dsn { namespace proxy {

DEFINE_TASK_CODE_RPC(RPC_CALL_RAW_SESSION_DISCONNECT, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)
DEFINE_TASK_CODE_RPC(RPC_CALL_RAW_MESSAGE, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)

DEFINE_THREAD_POOL_CODE(THREAD_POOL_PROXY_SERVER)
DEFINE_TASK_CODE(LPC_RPC_CALL_RAW_SCATTER, TASK_PRIORITY_COMMON, THREAD_POOL_PROXY_SERVER)

class proxy_stub;
class proxy_session: public std::enable_shared_from_this<proxy_session>, public dsn::clientlet
{
public:
    typedef std::function<std::shared_ptr<proxy_session> (proxy_stub* p, dsn::rpc_address raddr)> factory;
    proxy_session(proxy_stub* p, dsn::rpc_address raddr);
    virtual ~proxy_session();
    void on_recv_request(std::shared_ptr<proxy_session> _this, dsn_message_t msg);

    // called when proxy_stub remove this session
    virtual void on_remove_session(std::shared_ptr<proxy_session> _this) = 0;
    std::size_t hash() const { return hash_code; }

protected:
    //return true if no parse error, else return false
    virtual bool parse(dsn_message_t msg) = 0;
    dsn_message_t create_response();
    proxy_stub* stub;

private:
    //when get message from raw parser, request & response of "dsn_message_t" are not in couple
    //we need to backup one request to create a response struct.
    dsn_message_t backup_one_request;
    //the client address for which this session served
    dsn::rpc_address remote_address;
    std::size_t hash_code;
};

class proxy_stub: public ::dsn::serverlet<proxy_stub>
{
public:
    proxy_stub(const proxy_session::factory& f, const char* uri);
    virtual ~proxy_stub() override;
    const dsn::rpc_address get_service_uri() const { return _uri_address; }
    void open_service()
    {
        this->register_rpc_handler(RPC_CALL_RAW_MESSAGE, "raw_message", &proxy_stub::on_rpc_request);
        this->register_rpc_handler(RPC_CALL_RAW_SESSION_DISCONNECT, "raw_session_disconnect", &proxy_stub::on_recv_remove_session_request);
    }
    void close_service()
    {
        this->unregister_rpc_handler(RPC_CALL_RAW_MESSAGE);
        this->unregister_rpc_handler(RPC_CALL_RAW_SESSION_DISCONNECT);
    }
private:
    void on_rpc_request(dsn_message_t request);
    void on_recv_remove_session_request(dsn_message_t);

    service::zrwlock_nr _lock;
    std::unordered_map<rpc_address, std::shared_ptr<proxy_session> > _sessions;
    proxy_session::factory _factory;
    dsn::rpc_address _uri_address;
};

}}

#endif
