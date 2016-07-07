RRDB_ERR_CODE(RRDB_ERR_OK,                    0,      "success");
RRDB_ERR_CODE(RRDB_ERR_UNKNOWN,               -1,     "unknown error");
RRDB_ERR_CODE(RRDB_ERR_TIMEOUT,               -2,     "timeout");
RRDB_ERR_CODE(RRDB_ERR_OBJECT_NOT_FOUND,      -3,     "object not found");
RRDB_ERR_CODE(RRDB_ERR_NETWORK_FAILURE,       -4,     "network failure");

// SERVER ERROR
RRDB_ERR_CODE(RRDB_ERR_APP_NOT_EXIST,         -101,   "app not exist");
RRDB_ERR_CODE(RRDB_ERR_APP_EXIST,             -102,   "app already exist");
RRDB_ERR_CODE(RRDB_ERR_SERVER_INTERNAL_ERROR, -103,   "server internal error");
RRDB_ERR_CODE(RRDB_ERR_SERVER_CHANGED,        -104,   "server changed");

// CLIENT ERROR
RRDB_ERR_CODE(RRDB_ERR_INVALID_APP_NAME,      -201,   "app name is invalid, only letters, digits or underscore is valid");
RRDB_ERR_CODE(RRDB_ERR_INVALID_HASH_KEY,      -202,   "hash key can't be empty");
RRDB_ERR_CODE(RRDB_ERR_INVALID_VALUE,         -203,   "value can't be empty");
RRDB_ERR_CODE(RRDB_ERR_INVALID_PAR_COUNT,     -204,   "partition count must be a power of 2");
RRDB_ERR_CODE(RRDB_ERR_INVALID_REP_COUNT,     -205,   "replication count must be 3");

// SERVER ERROR
// start from -301

// ROCKSDB SERVER ERROR
RRDB_ERR_CODE(RRDB_ERR_NOT_FOUND,             -1001,  "not found");
RRDB_ERR_CODE(RRDB_ERR_CORRUPTION,            -1002,  "corruption");
RRDB_ERR_CODE(RRDB_ERR_NOT_SUPPORTED,         -1003,  "not supported");
RRDB_ERR_CODE(RRDB_ERR_INVALID_ARGUMENT,      -1004,  "invalid argument");
RRDB_ERR_CODE(RRDB_ERR_IO_ERROR,              -1005,  "io error");
RRDB_ERR_CODE(RRDB_ERR_MERGE_IN_PROGRESS,     -1006,  "merge in progress");
RRDB_ERR_CODE(RRDB_ERR_INCOMPLETE,            -1007,  "incomplete");
RRDB_ERR_CODE(RRDB_ERR_SHUTDOWN_IN_PROGRESS,  -1008,  "shutdown in progress");
RRDB_ERR_CODE(RRDB_ERR_ROCKSDB_TIME_OUT,      -1009,  "rocksdb time out");
RRDB_ERR_CODE(RRDB_ERR_ABORTED,               -1010,  "aborted");
RRDB_ERR_CODE(RRDB_ERR_BUSY,                  -1011,  "busy");
RRDB_ERR_CODE(RRDB_ERR_EXPIRED,               -1012,  "expired");
RRDB_ERR_CODE(RRDB_ERR_TRY_AGAIN,             -1013,  "try again");
