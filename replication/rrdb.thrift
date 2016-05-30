
include "dsn.thrift"

namespace cpp dsn.apps

struct update_request
{
    1:dsn.blob      key;
    2:dsn.blob      value;
}

struct read_response
{
    1:i32      error;
    2:string   value;
}

service rrdb
{
    i32 put(1:update_request update);
    i32 remove(1:dsn.blob key);
    i32 merge(1:update_request update);
    read_response get(1:dsn.blob key);
}


