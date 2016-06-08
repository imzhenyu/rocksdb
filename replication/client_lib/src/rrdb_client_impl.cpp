#include <cctype>
#include <algorithm>
#include <string>

#include <dsn/cpp/auto_codes.h>
#include "rrdb_client_impl.h"
#include "rrdb_error.h"
#include "rrdb.code.definition.h"

using namespace dsn;

namespace dsn{ namespace apps{

#define ROCSKDB_ERROR_START -1000

std::unordered_map<int, std::string> rrdb_client_impl::_client_error_to_string;
std::unordered_map<int, int> rrdb_client_impl::_server_error_to_client;

void rrdb_generate_key(dsn::blob& key, const std::string& hash_key, const std::string& sort_key)
{
    int len = 4 + hash_key.size() + sort_key.size();
    char* buf = new char[len];
    // hash_key_length is in big endian
    *(int32_t*)buf = htobe32( (int32_t)hash_key.size() );
    hash_key.copy(buf + 4, hash_key.size(), 0);
    sort_key.copy(buf + 4 + hash_key.size(), sort_key.size(), 0);
    std::shared_ptr<char> buffer(buf);
    key.assign(buffer, 0, len);
}

uint64_t rrdb_key_hash(const ::dsn::blob& key)
{
    dassert(key.length() > 4, "key length must be greater than 4");
    // hash_key_length is in big endian
    int length = be32toh( *(int32_t*)(key.data()) );
    dassert(key.length() >= 4 + length, "key length must be greater than 4 + hash_key length");
    dsn::blob hash_key(key.buffer_ptr(), 4, length);
    return dsn_crc64_compute(hash_key.data(), hash_key.length(), 0);
}

rrdb_client_impl::rrdb_client_impl(const char* app_name, const std::vector< ::dsn::rpc_address>& meta_servers)
    :_app_name(app_name), _client(meta_servers, app_name)
{
    for (auto& addr : meta_servers) {
        if (!_cluster_meta_servers.empty()) {
            _cluster_meta_servers += ",";
        }
        _cluster_meta_servers += addr.to_string();
    }
}

const char* rrdb_client_impl::get_cluster_meta_servers() const
{
    return _cluster_meta_servers.c_str();
}

const char* rrdb_client_impl::get_app_name() const
{
    return _app_name.c_str();
}

int rrdb_client_impl::set(
        const std::string& hash_key,
        const std::string& sort_key,
        const std::string& value,
        int timeout_milliseconds,
        internal_info* info
        )
{
    // check params
    if(hash_key.empty())
        return RRDB_ERR_INVALID_HASH_KEY;

    update_request req;
    rrdb_generate_key(req.key, hash_key, sort_key);
    req.value.assign(value.c_str(), 0, value.size());
    auto pr = _client.put_sync(req, std::chrono::milliseconds(timeout_milliseconds));
    if (info != nullptr)
    {
        if (pr.first == ERR_OK)
        {
            info->app_id = pr.second.app_id;
            info->pidx = pr.second.pidx;
            info->ballot = pr.second.ballot;
            info->decree = pr.second.decree;
            info->seqno = pr.second.seqno;
            info->server = pr.second.server;
        }
        else
        {
            info->app_id = _client.get_app_id() > 0 ?
                        _client.get_app_id() : -1;
            info->pidx = _client.get_partition_count() > 0 ?
                        _client.get_partition_index(_client.get_partition_count(), _client.get_key_hash(req)) : -1;
            info->ballot = -1;
            info->decree = -1;
            info->seqno = -1;
        }
    }
    return get_client_error(pr.first == ERR_OK ? get_rocksdb_server_error(pr.second.error) : pr.first.get());
}


int rrdb_client_impl::get(
        const std::string& hash_key,
        const std::string& sort_key,
        std::string& value,
        int timeout_milliseconds,
        internal_info* info
        )
{
    // check params
    if(hash_key.empty())
        return RRDB_ERR_INVALID_HASH_KEY;

    dsn::blob req;
    rrdb_generate_key(req, hash_key, sort_key);
    auto pr = _client.get_sync(req, std::chrono::milliseconds(timeout_milliseconds));
    if(pr.first == ERR_OK && pr.second.error == 0)
        value.assign(pr.second.value.data(), pr.second.value.length());
    if (info != nullptr)
    {
        if (pr.first == ERR_OK)
        {
            info->app_id = pr.second.app_id;
            info->pidx = pr.second.pidx;
            info->ballot = pr.second.ballot;
            info->decree = -1;
            info->seqno = -1;
            info->server = pr.second.server;
        }
        else
        {
            info->app_id = _client.get_app_id() > 0 ?
                        _client.get_app_id() : -1;
            info->pidx = _client.get_partition_count() > 0 ?
                        _client.get_partition_index(_client.get_partition_count(), _client.get_key_hash(req)) : -1;
            info->ballot = -1;
            info->decree = -1;
            info->seqno = -1;
        }
    }
    return get_client_error(pr.first == ERR_OK ? get_rocksdb_server_error(pr.second.error) : pr.first.get());
}

int rrdb_client_impl::del(
        const std::string& hash_key,
        const std::string& sort_key,
        int timeout_milliseconds,
        internal_info* info
        )
{
    // check params
    if(hash_key.empty())
        return RRDB_ERR_INVALID_HASH_KEY;

    dsn::blob req;
    rrdb_generate_key(req, hash_key, sort_key);
    auto pr = _client.remove_sync(req, std::chrono::milliseconds(timeout_milliseconds));
    if (info != nullptr)
    {
        if (pr.first == ERR_OK)
        {
            info->app_id = pr.second.app_id;
            info->pidx = pr.second.pidx;
            info->ballot = pr.second.ballot;
            info->decree = pr.second.decree;
            info->seqno = pr.second.seqno;
            info->server = pr.second.server;
        }
        else
        {
            info->app_id = _client.get_app_id() > 0 ?
                        _client.get_app_id() : -1;
            info->pidx = _client.get_partition_count() > 0 ?
                        _client.get_partition_index(_client.get_partition_count(), _client.get_key_hash(req)) : -1;
            info->ballot = -1;
            info->decree = -1;
            info->seqno = -1;
        }
    }
    return get_client_error(pr.first == ERR_OK ? get_rocksdb_server_error(pr.second.error) : pr.first.get());
}

const char* rrdb_client_impl::get_error_string(int error_code) const
{
    auto it = _client_error_to_string.find(error_code);
    dassert(it != _client_error_to_string.end(), "client error %d have no error string", error_code);
    return it->second.c_str();
}

/*static*/ void rrdb_client_impl::init_error()
{
    _client_error_to_string.clear();
    #define RRDB_ERR_CODE(x, y, z) _client_error_to_string[y] = z
    #include "rrdb_error_def.h"
    #undef RRDB_ERR_CODE

    _server_error_to_client.clear();
    _server_error_to_client[dsn::ERR_OK] = RRDB_ERR_OK;
    _server_error_to_client[dsn::ERR_TIMEOUT] = RRDB_ERR_TIMEOUT;
    _server_error_to_client[dsn::ERR_FILE_OPERATION_FAILED] = RRDB_ERR_SERVER_INTERNAL_ERROR;
    _server_error_to_client[dsn::ERR_OBJECT_NOT_FOUND] = RRDB_ERR_OBJECT_NOT_FOUND;

    _server_error_to_client[dsn::replication::ERR_APP_NOT_EXIST] = RRDB_ERR_APP_NOT_EXIST;
    _server_error_to_client[dsn::replication::ERR_APP_EXIST] = RRDB_ERR_APP_EXIST;

    // rocksdb error;
    for(int i = 1001; i < 1013; i++)
    {
        _server_error_to_client[-i] = -i;
    }
}

/*static*/ int rrdb_client_impl::get_client_error(int server_error)
{
    auto it = _server_error_to_client.find(server_error);
    if(it != _server_error_to_client.end())
        return it->second;
    derror("can't find corresponding client error definition, server error:[%d:%s]", server_error, dsn::error_code(server_error).to_string());
    return RRDB_ERR_UNKNOWN;
}

/*static*/ int rrdb_client_impl::get_rocksdb_server_error(int rocskdb_error)
{
    return (rocskdb_error == 0) ? 0 : ROCSKDB_ERROR_START - rocskdb_error;
}
}} // namespace
