include "dsn.thrift"

namespace cpp dsn.apps

struct update_request
{
    1:dsn.blob      key;
    2:dsn.blob      value;
}

struct update_response
{
    1:i32           error;
    2:i32           app_id;
    3:i32           pidx;
    4:i64           ballot;
    5:i64           decree;
    6:i64           seqno;
    7:string        server;
}

struct read_response
{
    1:i32           error;
    2:dsn.blob      value;
    3:i32           app_id;
    4:i32           pidx;
    5:i64           ballot;
    6:string        server;
}

service rrdb
{
    update_response put(1:update_request update);
    update_response remove(1:dsn.blob key);
    read_response get(1:dsn.blob key);
}

