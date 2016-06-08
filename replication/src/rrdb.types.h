# pragma once
# include <dsn/service_api_cpp.h>

//
// uncomment the following line if you want to use 
// data encoding/decoding from the original tool instead of rDSN
// in this case, you need to use these tools to generate
// type files with --gen=cpp etc. options
//
# if defined(DSN_USE_THRIFT_SERIALIZATION)

# include "rrdb_types.h"

# elif defined(DSN_USE_PROTO_SERIALIZATION)

# include "rrdb.pb.h"

# else // use rDSN's data encoding/decoding

namespace dsn { namespace apps { 
    // ---------- update_request -------------
    struct update_request
    {
        ::dsn::blob key;
        ::dsn::blob value;
    };

    inline void marshall(::dsn::binary_writer& writer, const update_request& val)
    {
        marshall(writer, val.key);
        marshall(writer, val.value);
    }

    inline void unmarshall(::dsn::binary_reader& reader, /*out*/ update_request& val)
    {
        unmarshall(reader, val.key);
        unmarshall(reader, val.value);
    }

    // ---------- update_response -------------
    struct update_response
    {
        int32_t error;
        int32_t app_id;
        int32_t pidx;
        int64_t ballot;
        int64_t decree;
        int64_t seqno;
        std::string server;
    };

    inline void marshall(::dsn::binary_writer& writer, const update_response& val)
    {
        marshall(writer, val.error);
        marshall(writer, val.app_id);
        marshall(writer, val.pidx);
        marshall(writer, val.ballot);
        marshall(writer, val.decree);
        marshall(writer, val.seqno);
        marshall(writer, val.server);
    }

    inline void unmarshall(::dsn::binary_reader& reader, /*out*/ update_response& val)
    {
        unmarshall(reader, val.error);
        unmarshall(reader, val.app_id);
        unmarshall(reader, val.pidx);
        unmarshall(reader, val.ballot);
        unmarshall(reader, val.decree);
        unmarshall(reader, val.seqno);
        unmarshall(reader, val.server);
    }

    // ---------- read_response -------------
    struct read_response
    {
        int32_t error;
        ::dsn::blob value;
        int32_t app_id;
        int32_t pidx;
        int64_t ballot;
        std::string server;
    };

    inline void marshall(::dsn::binary_writer& writer, const read_response& val)
    {
        marshall(writer, val.error);
        marshall(writer, val.value);
        marshall(writer, val.app_id);
        marshall(writer, val.pidx);
        marshall(writer, val.ballot);
        marshall(writer, val.server);
    }

    inline void unmarshall(::dsn::binary_reader& reader, /*out*/ read_response& val)
    {
        unmarshall(reader, val.error);
        unmarshall(reader, val.value);
        unmarshall(reader, val.app_id);
        unmarshall(reader, val.pidx);
        unmarshall(reader, val.ballot);
        unmarshall(reader, val.server);
    }

} } 

#endif 
