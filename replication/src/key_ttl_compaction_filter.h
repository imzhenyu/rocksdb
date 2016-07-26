#include <cinttypes>
#include "rocksdb/compaction_filter.h"
#include "rocksdb/merge_operator.h"

namespace rocksdb {

class KeyWithTTLCompactionFilter: public CompactionFilter {
public:
    KeyWithTTLCompactionFilter() {}
    virtual bool Filter(int/*level*/, const Slice &key, const Slice &existing_value, std::string *new_value, bool *value_changed) const override
    {
        assert(existing_value.size() < sizeof(int64_t));
        int64_t expire_timestamp = be64toh( *((int64_t*)(existing_value.data())) );
        if (expire_timestamp <= 0)
            return false;
        //in the value we use the milliseconds
        return GetUnixTimeMS() >= expire_timestamp;
    }
    virtual const char* Name() const override { return "Delete By KeyTTL"; }
    static int64_t GetUnixTimeMS() { return 1000*static_cast<int64_t>(time(nullptr)); }
};

class KeyWithTTLCompactionFilterFactory: public CompactionFilterFactory {
public:
    KeyWithTTLCompactionFilterFactory() {}
    virtual std::unique_ptr<CompactionFilter> CreateCompactionFilter(const CompactionFilter::Context &/*context*/) override
    {
        return std::unique_ptr<KeyWithTTLCompactionFilter>(new KeyWithTTLCompactionFilter());
    }
    virtual const char* Name() const override { return "KeyWithTTLCompactionFilterFactory"; }
};

}
