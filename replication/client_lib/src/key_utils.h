#pragma once
#include <string>
#include <dsn/utility/utils.h>

namespace dsn{ namespace apps{

void rrdb_generate_key(::dsn::blob& key, const std::string& hash_key, const std::string& sort_key);
void rrdb_generate_key(::dsn::blob& key, const ::dsn::blob& hash_key, const ::dsn::blob& sort_key);
uint64_t rrdb_key_hash(const ::dsn::blob& key);

}} //namespace

