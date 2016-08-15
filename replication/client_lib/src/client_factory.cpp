#include <pegasus/client.h>
#include "rrdb_client_factory_impl.h"

namespace pegasus {

bool pegasus_client_factory::initialize(const char* config_file)
{
    return rrdb_client_factory_impl::initialize(config_file);
}

pegasus_client* pegasus_client_factory::get_client(const char* cluster_name, const char* app_name)
{
    return rrdb_client_factory_impl::get_client(cluster_name, app_name);
}

} //namesapce
