#pragma once
#include <stdint.h>
#include <string.h>
#include <dsn/utility/ports.h>

namespace dsn{ namespace apps{

// T may be std::string or ::dsn::blob.
template <typename T>
void rrdb_generate_key(::dsn::blob& key, const T& hash_key, const T& sort_key)
{
    dassert(hash_key.length() <= UINT16_MAX, "hash key length must be no more than UINT16_MAX");

    int len = 2 + hash_key.length() + sort_key.length();
    std::shared_ptr<char> buf(::dsn::make_shared_array<char>(len));

    // hash_key_len is in big endian
    uint16_t hash_key_len = hash_key.length();
    *((int16_t*)buf.get()) = htobe16((int16_t)hash_key_len);

    ::memcpy(buf.get() + 2, hash_key.data(), hash_key_len);

    if (sort_key.length() > 0)
    {
        ::memcpy(buf.get() + 2 + hash_key_len, sort_key.data(), sort_key.length());
    }

    key.assign(buf, 0, len);
}

inline uint64_t rrdb_key_hash(const ::dsn::blob& key)
{
    dassert(key.length() >= 2, "key length must be greater than 2");

    // hash_key_len is in big endian
    uint16_t hash_key_len = be16toh(*(int16_t*)(key.data()));

    if (hash_key_len > 0)
    {
        // hash_key_len > 0, compute hash from hash_key
        dassert(key.length() >= 2 + hash_key_len, "key length must be greater than (2 + hash_key_len)");
        return dsn_crc64_compute(key.buffer_ptr() + 2, hash_key_len, 0);
    }
    else
    {
        // hash_key_len == 0, compute hash from sort_key
        return dsn_crc64_compute(key.buffer_ptr() + 2, key.length() - 2, 0);
    }
}

}} //namespace

