#pragma once

namespace pegasus {

#define PEGASUS_ERR_CODE(x, y, z) static const int x = y
#include <pegasus/error_def.h>
#undef PEGASUS_ERR_CODE

} //namespace
