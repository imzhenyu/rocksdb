# pragma once

# include "rrdb.server.h"
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
            rrdb_service_impl(dsn_gpid gpid);

            // the following methods may set physical error if internal error occurs
            void on_put(const update_request& update, ::dsn::rpc_replier<int>& reply) override;
            void on_remove(const ::dsn::blob& key, ::dsn::rpc_replier<int>& reply) override;
            void on_merge(const update_request& update, ::dsn::rpc_replier<int>& reply) override;
            void on_get(const ::dsn::blob& key, ::dsn::rpc_replier<read_response>& reply) override;

            // open the db
            // if create_new == true, then first clear data and then create new db
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            //  - ERR_LOCAL_APP_FAILURE
            ::dsn::error_code start(int argc, char** argv) override;

            // close the db
            // if clear_state == true, then clear data after close the db
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            ::dsn::error_code stop(bool clear_state) override;

            // sync generate checkpoint
            // returns:
            //  - ERR_OK
            //  - ERR_WRONG_TIMING
            //  - ERR_NO_NEED_OPERATE
            //  - ERR_LOCAL_APP_FAILURE
            //  - ERR_FILE_OPERATION_FAILED
            ::dsn::error_code sync_checkpoint(int64_t version) override;

            // async generate checkpoint
            // returns:
            //  - ERR_OK
            //  - ERR_WRONG_TIMING
            //  - ERR_NO_NEED_OPERATE
            //  - ERR_LOCAL_APP_FAILURE
            //  - ERR_FILE_OPERATION_FAILED
            //  - ERR_TRY_AGAIN
            ::dsn::error_code async_checkpoint(int64_t version) override;

            int64_t get_last_checkpoint_decree() override { return _last_durable_decree.load(); }

            // get the last checkpoint
            // if succeed:
            //  - the checkpoint files path are put into "state.files"
            //  - the checkpoint_info are serialized into "state.meta"
            //  - the "state.from_decree_excluded" and "state.to_decree_excluded" are set properly
            // returns:
            //  - ERR_OK
            //  - ERR_OBJECT_NOT_FOUND
            //  - ERR_FILE_OPERATION_FAILED
            ::dsn::error_code get_checkpoint(
                int64_t start,
                int64_t local_commit,
                void*   learn_request,
                int     learn_request_size,
                /* inout */ app_learn_state& state
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
            ::dsn::error_code apply_checkpoint(dsn_chkpt_apply_mode mode, int64_t local_commit, const dsn_app_learn_state& state) override;

            int get_physical_error() override { return _physical_error; }

        private:
            // parse checkpoint directories in the data dir
            // checkpoint directory format is: "checkpoint.{decree}"
            // returns the last checkpoint info
            int64_t parse_checkpoints();

            // garbage collection checkpoints according to _max_checkpoint_count
            void gc_checkpoints();

            ::dsn::error_code checkpoint_internal(int64_t version);

            const char* data_dir() { return _data_dir.c_str(); }
            int64_t last_durable_decree() { return _last_durable_decree.load(); }
            void    set_last_durable_decree(int64_t d) { _last_durable_decree = d; }            
            void    set_physical_error(int err) { _physical_error = err; }

        private:
            rocksdb::DB           *_db;
            
            rocksdb::Options      _db_opts;
            rocksdb::WriteOptions _wt_opts;
            rocksdb::ReadOptions  _rd_opts;

            volatile bool         _is_open;
            const int             _max_checkpoint_count;

            volatile bool                _is_checkpointing; // whether the db is doing checkpoint
            ::dsn::utils::ex_lock_nr     _checkpoints_lock; // protected the following checkpoints vector
            std::vector<int64_t>         _checkpoints;      // ordered checkpoints
            std::string                  _data_dir;
            std::atomic<int64_t>         _last_durable_decree;

            int                   _physical_error;
        };

        // --------- inline implementations -----------------
    }
}
