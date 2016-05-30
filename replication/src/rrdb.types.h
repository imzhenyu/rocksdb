# pragma once
# include <dsn/service_api_cpp.h>
# include <dsn/cpp/serialization.h>


# include "rrdb_types.h" 


namespace dsn { namespace apps { 
    GENERATED_TYPE_SERIALIZATION(update_request, THRIFT)
    GENERATED_TYPE_SERIALIZATION(read_response, THRIFT)

} } 