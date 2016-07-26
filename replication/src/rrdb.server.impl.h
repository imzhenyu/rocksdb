# pragma once

# include "rrdb.server.h"
# include "key_ttl_compaction_filter.h"
# include <rocksdb/db.h>
# include <vector>

# include <dsn/cpp/replicated_service_app.h>

namespace dsn {
    namespace apps {
        class rrdb_service_impl : 
            public rrdb_service,
            public replicated_service_app_type_1
        {
        public:
            explicit rrdb_service_impl(dsn_gpid gpid);
            virtual ~rrdb_service_impl() {}

            // the following methods may set physical error if internal error occurs
            virtual void on_put(const update_request& update, ::dsn::rpc_replier<update_response>& reply) override;
            virtual void on_remove(const ::dsn::blob& key, ::dsn::rpc_replier<update_response>& reply) override;
            virtual void on_get(const ::dsn::blob& key, ::dsn::rpc_replier<read_response>& reply) override;

            // the following methods are for stateful apps with layer 2 support
 
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            //  - ERR_LOCAL_APP_FAILURE
            virtual ::dsn::error_code start(int argc, char** argv) override;

            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            virtual ::dsn::error_code stop(bool clear_state) override;

            virtual void on_batched_write_requests(int64_t decree, dsn_message_t* requests, int count) override;

            virtual int get_physical_error() override { return _physical_error; }

            // returns:
            //  - ERR_OK
            //  - ERR_WRONG_TIMING
            //  - ERR_NO_NEED_OPERATE
            //  - ERR_LOCAL_APP_FAILURE
            //  - ERR_FILE_OPERATION_FAILED
            virtual ::dsn::error_code sync_checkpoint(int64_t last_commit) override;

            // returns:
            //  - ERR_OK
            //  - ERR_WRONG_TIMING
            //  - ERR_NO_NEED_OPERATE
            //  - ERR_LOCAL_APP_FAILURE
            //  - ERR_FILE_OPERATION_FAILED
            //  - ERR_TRY_AGAIN
            virtual ::dsn::error_code async_checkpoint(int64_t last_commit) override;

            virtual int64_t get_last_checkpoint_decree() override { return last_durable_decree(); }

            // get the last checkpoint
            // if succeed:
            //  - the checkpoint files path are put into "state.files"
            //  - the checkpoint_info are serialized into "state.meta"
            //  - the "state.from_decree_excluded" and "state.to_decree_excluded" are set properly
            // returns:
            //  - ERR_OK
            //  - ERR_OBJECT_NOT_FOUND
            //  - ERR_FILE_OPERATION_FAILED
            virtual ::dsn::error_code get_checkpoint(
                int64_t learn_start,
                int64_t local_commit,
                void*   learn_request,
                int     learn_request_size,
                app_learn_state& state
                ) override;

            // apply checkpoint, this will clear and recreate the db
            // if succeed:
            //  - last_committed_decree() == last_durable_decree()
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            //  - error code of close()
            //  - error code of open()
            //  - error code of checkpoint()
            virtual ::dsn::error_code apply_checkpoint(
                dsn_chkpt_apply_mode mode,
                int64_t local_commit,
                const dsn_app_learn_state& state
                ) override;

        private:
            // parse checkpoint directories in the data dir
            // checkpoint directory format is: "checkpoint.{decree}"
            void parse_checkpoints();

            // garbage collection checkpoints according to _max_checkpoint_count
            void gc_checkpoints();

            int64_t last_durable_decree() { return _last_durable_decree.load(); }
            void set_last_durable_decree(int64_t decree) { _last_durable_decree.store(decree); }
            
        private:
            dsn_gpid                     _gpid;
            std::string                  _primary_address;
            std::string                  _replica_name;
            std::string                  _data_dir;

            rocksdb::KeyWithTTLCompactionFilter _key_ttl_compaction_filter;
            rocksdb::Options             _db_opts;
            rocksdb::WriteOptions        _wt_opts;
            rocksdb::ReadOptions         _rd_opts;

            rocksdb::DB*                 _db;
            volatile bool                _is_open;
            std::atomic<int64_t>         _last_durable_decree;

            rocksdb::WriteBatch          _batch;
            std::vector< ::dsn::rpc_replier<update_response>> _batch_repliers;
            int                          _physical_error;

            const int                    _max_checkpoint_count;
            volatile bool                _is_checkpointing; // whether the db is doing checkpoint
            ::dsn::utils::ex_lock_nr     _checkpoints_lock; // protected the following checkpoints vector
            std::vector<int64_t>         _checkpoints;      // ordered checkpoints
        };

        // --------- inline implementations -----------------
    }
}
