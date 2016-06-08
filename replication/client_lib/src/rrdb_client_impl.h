#pragma once
#include <string>
#include "rrdb_client.h"
#include "rrdb.client.h"

using namespace dsn::replication;

namespace dsn{ namespace apps{

void rrdb_generate_key(::dsn::blob& key, const std::string& hash_key, const std::string& sort_key);
uint64_t rrdb_key_hash(const ::dsn::blob& key);

class rrdb_client_hash_key : public rrdb_client
{
public:
    rrdb_client_hash_key(
            const std::vector< ::dsn::rpc_address>& meta_servers,
            const char* replicate_app_name)
        :rrdb_client(meta_servers, replicate_app_name)
    {}

    virtual uint64_t get_key_hash(const ::dsn::blob& key)
    {
        return rrdb_key_hash(key);
    }

    virtual uint64_t get_key_hash(const update_request& key)
    {
        return rrdb_key_hash(key.key);
    }
};

class rrdb_client_impl : public irrdb_client
{
public:
    rrdb_client_impl(const char* app_name, const std::vector< ::dsn::rpc_address>& meta_servers);
    virtual ~rrdb_client_impl(){}

    virtual const char* get_cluster_meta_servers() const override;

    virtual const char* get_app_name() const override;

    virtual int set(
        const std::string& hashkey,
        const std::string& sortkey,
        const std::string& value,
        int timeout_milliseconds = 5000,
        internal_info* info = NULL
        ) override;

    virtual int get(
        const std::string& hashkey,
        const std::string& sortkey,
        std::string& value,
        int timeout_milliseconds = 5000,
        internal_info* info = NULL
        ) override;

    virtual int del(
        const std::string& hashkey,
        const std::string& sortkey,
        int timeout_milliseconds = 5000,
        internal_info* info = NULL
        ) override;

    virtual const char* get_error_string(int error_code) const override;

    static void init_error();

private:
    static int get_client_error(int server_error);
    static int get_rocksdb_server_error(int rocskdb_error);

private:
    rrdb_client_hash_key _client;
    std::string _cluster_meta_servers;
    std::string _app_name;

    ///
    /// \brief _client_error_to_string
    /// store int to string for client call get_error_string()
    ///
    static std::unordered_map<int, std::string> _client_error_to_string;

    ///
    /// \brief _server_error_to_client
    /// translate server error to client, it will find from a map<int, int>
    /// the map is initialized in init_error() which will be called on client lib initailization.
    ///
    static std::unordered_map<int, int> _server_error_to_client;
};

}} //namespace
