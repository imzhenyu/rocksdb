#pragma once
#include <string>
#include "rrdb_client.h"
#include "rrdb.client.h"
#include "key_utils.h"

namespace dsn{ namespace apps{

class rrdb_client_impl : public irrdb_client
{
public:
    rrdb_client_impl(const char* cluster_name, const char* app_name);
    virtual ~rrdb_client_impl();

    virtual const char* get_cluster_name() const override;

    virtual const char* get_app_name() const override;

    virtual int set(
        const std::string& hashkey,
        const std::string& sortkey,
        const std::string& value,
        int64_t timeout_milliseconds = 5000,
        int64_t ttl_milliseconds = 0,
        internal_info* info = NULL
        ) override;

    virtual int get(
        const std::string& hashkey,
        const std::string& sortkey,
        std::string& value,
        int64_t timeout_milliseconds = 5000,
        internal_info* info = NULL
        ) override;

    virtual int del(
        const std::string& hashkey,
        const std::string& sortkey,
        int64_t timeout_milliseconds = 5000,
        internal_info* info = NULL
        ) override;

    virtual const char* get_error_string(int error_code) const override;

    static void init_error();

private:
    static int get_client_error(int server_error);
    static int get_rocksdb_server_error(int rocskdb_error);

private:
    std::string _cluster_name;
    std::string _app_name;
    std::string _server_uri;
    rpc_address _server_address;
    rrdb_client *_client;

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
