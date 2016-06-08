# pragma once

# include "rrdb.server.h"
# include <rocksdb/db.h>
# include <vector>

namespace dsn {
    namespace apps {
        class rrdb_service_impl : public rrdb_service
        {
        public:
            rrdb_service_impl(::dsn::replication::replica* replica);

            // the following methods may set physical error if internal error occurs
            virtual void on_put(const update_request& update, ::dsn::replication::rpc_replication_app_replier<update_response>& reply) override;
            virtual void on_remove(const ::dsn::blob& key, ::dsn::replication::rpc_replication_app_replier<update_response>& reply) override;
            virtual void on_get(const ::dsn::blob& key, ::dsn::replication::rpc_replication_app_replier<read_response>& reply) override;

            // open the db
            // if create_new == true, then first clear data and then create new db
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            //  - ERR_LOCAL_APP_FAILURE
            virtual ::dsn::error_code open(bool create_new) override;

            // close the db
            // if clear_state == true, then clear data after close the db
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            virtual ::dsn::error_code close(bool clear_state) override;

            // sync generate checkpoint
            // returns:
            //  - ERR_OK
            //  - ERR_WRONG_TIMING
            //  - ERR_NO_NEED_OPERATE
            //  - ERR_LOCAL_APP_FAILURE
            //  - ERR_FILE_OPERATION_FAILED
            virtual ::dsn::error_code checkpoint() override;

            // async generate checkpoint
            // returns:
            //  - ERR_OK
            //  - ERR_WRONG_TIMING
            //  - ERR_NO_NEED_OPERATE
            //  - ERR_LOCAL_APP_FAILURE
            //  - ERR_FILE_OPERATION_FAILED
            //  - ERR_TRY_AGAIN
            virtual ::dsn::error_code checkpoint_async() override;

            // get the last checkpoint
            // if succeed:
            //  - the checkpoint files path are put into "state.files"
            //  - the checkpoint_info are serialized into "state.meta"
            //  - the "state.from_decree_excluded" and "state.to_decree_excluded" are set properly
            // returns:
            //  - ERR_OK
            //  - ERR_OBJECT_NOT_FOUND
            //  - ERR_FILE_OPERATION_FAILED
            virtual ::dsn::error_code get_checkpoint(::dsn::replication::decree start,
                                                     const blob& learn_req,
                                                     /*out*/ ::dsn::replication::learn_state& state) override;

            // apply checkpoint, this will clear and recreate the db
            // if succeed:
            //  - last_committed_decree() == last_durable_decree()
            // returns:
            //  - ERR_OK
            //  - ERR_FILE_OPERATION_FAILED
            //  - error code of close()
            //  - error code of open()
            //  - error code of checkpoint()
            virtual ::dsn::error_code apply_checkpoint(::dsn::replication::learn_state& state,
                                                       ::dsn::replication::chkpt_apply_mode mode) override;

        private:
            struct checkpoint_info
            {
                ::dsn::replication::decree d;
                rocksdb::SequenceNumber    seq;
                checkpoint_info() : d(0), seq(0) {}
                // sort by first decree then seq increasingly
                bool operator< (const checkpoint_info& o) const
                {
                    return d < o.d || (d == o.d && seq < o.seq);
                }
            };

            // parse checkpoint directories in the data dir
            // checkpoint directory format is: "checkpoint.{decree}.{seq}"
            // returns the last checkpoint info
            checkpoint_info parse_checkpoints();

            // garbage collection checkpoints according to _max_checkpoint_count
            void gc_checkpoints();

            // write batch data into rocksdb
            void write_batch();

            // check _last_seq consistency
            void check_last_seq();

            // when in catch mode, increase _last_seq by one
            void catchup_one();

        private:
            rocksdb::DB           *_db;
            std::string           _primary_address;
            rocksdb::WriteBatch   _batch;
            std::vector< ::dsn::replication::rpc_replication_app_replier<update_response>> _batch_repliers;
            rocksdb::Options      _db_opts;
            rocksdb::WriteOptions _wt_opts;
            rocksdb::ReadOptions  _rd_opts;

            volatile bool         _is_open;
            const int             _max_checkpoint_count;

            // ATTENTION:
            // _last_committed_decree is totally controlled by rdsn, and set to the decree of last checkpoint when open.
            // _last_durable_decree is always set to the decree of last checkpoint.
                        
            rocksdb::SequenceNumber      _last_seq;         // always equal to DB::GetLatestSequenceNumber()
            volatile bool                _is_catchup;       // whether the db is in catch up mode
            rocksdb::SequenceNumber      _last_durable_seq; // valid only when _is_catchup is true
            volatile bool                _is_checkpointing; // whether the db is doing checkpoint
            ::dsn::utils::ex_lock_nr     _checkpoints_lock; // protected the following checkpoints vector
            std::vector<checkpoint_info> _checkpoints;      // ordered checkpoints
        };

        // --------- inline implementations -----------------
    }
}
