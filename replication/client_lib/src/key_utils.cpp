#include "key_utils.h"

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "rrdb.key.utils"

namespace dsn{ namespace apps{

void rrdb_generate_key(dsn::blob& key, const std::string& hash_key, const std::string& sort_key)
{
    int len = 4 + hash_key.size() + sort_key.size();
    char* buf = new char[len];
    // hash_key_length is in big endian
    *(int32_t*)buf = htobe32( (int32_t)hash_key.size() );
    hash_key.copy(buf + 4, hash_key.size(), 0);
    if (sort_key.size() > 0)
        sort_key.copy(buf + 4 + hash_key.size(), sort_key.size(), 0);
    std::shared_ptr<char> buffer(buf);
    key.assign(std::move(buffer), 0, len);
}

void rrdb_generate_key(::dsn::blob& key, const ::dsn::blob& hash_key, const ::dsn::blob& sort_key)
{
    int len = 4 + hash_key.length() + sort_key.length();
    char* buf = new char[len];
    // hash_key_length is in big endian
    *(int32_t*)buf = htobe32( (int32_t)hash_key.length() );
    memcpy(buf + 4, hash_key.data(), hash_key.length());
    if (sort_key.length() > 0)
        memcpy(buf + 4 + hash_key.length(), sort_key.data(), sort_key.length());
    std::shared_ptr<char> buffer(buf);
    key.assign(std::move(buffer), 0, len);
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

}} // namespace

