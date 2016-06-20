# include "rrdb.server.impl.h"
# include <algorithm>
# include <dsn/cpp/utils.h>
# include <rocksdb/utilities/checkpoint.h>

# ifdef __TITLE__
# undef __TITLE__
# endif
#define __TITLE__ "rrdb.server.impl"

namespace dsn {
    namespace apps {
        
        static std::string chkpt_get_dir_name(int64_t d)
        {
            char buffer[256];
            sprintf(buffer, "checkpoint.%" PRId64 "", d);
            return std::string(buffer);
        }

        static bool chkpt_init_from_dir(const char* name, int64_t& d)
        {
            return 1 == sscanf(name, "checkpoint.%" PRId64 "", &d)
                && std::string(name) == chkpt_get_dir_name(d);
        }

        rrdb_service_impl::rrdb_service_impl(dsn_gpid gpid) 
            : ::dsn::replicated_service_app_type_1(gpid),
              _max_checkpoint_count(3)
        {
            _is_open = false;
            _is_checkpointing = false;
            _last_durable_decree = 0;
            _data_dir = dsn_get_app_data_dir(gpid);

            // init db options
            _db_opts.write_buffer_size =
                (size_t)dsn_config_get_value_uint64("replication",
                "rocksdb_write_buffer_size",
                134217728,
                "rocksdb options.write_buffer_size, default 128MB"
                );
            _db_opts.max_write_buffer_number =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_max_write_buffer_number",
                3,
                "rocksdb options.max_write_buffer_number, default 3"
                );
            _db_opts.max_background_compactions =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_max_background_compactions",
                20,
                "rocksdb options.max_background_compactions, default 20"
                );
            _db_opts.num_levels =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_num_levels",
                6,
                "rocksdb options.num_levels, default 6"
                );
            _db_opts.target_file_size_base =
                dsn_config_get_value_uint64("replication",
                "rocksdb_target_file_size_base",
                67108864,
                "rocksdb options.write_buffer_size, default 64MB"
                );
            _db_opts.max_bytes_for_level_base =
                dsn_config_get_value_uint64("replication",
                "rocksdb_max_bytes_for_level_base",
                10485760,
                "rocksdb options.max_bytes_for_level_base, default 10MB"
                );
            _db_opts.max_grandparent_overlap_factor =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_max_grandparent_overlap_factor",
                10,
                "rocksdb options.max_grandparent_overlap_factor, default 10"
                );
            _db_opts.level0_file_num_compaction_trigger =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_level0_file_num_compaction_trigger",
                4,
                "rocksdb options.level0_file_num_compaction_trigger, 4"
                );
            _db_opts.level0_slowdown_writes_trigger =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_level0_slowdown_writes_trigger",
                8,
                "rocksdb options.level0_slowdown_writes_trigger, default 8"
                );
            _db_opts.level0_stop_writes_trigger =
                (int)dsn_config_get_value_uint64("replication",
                "rocksdb_level0_stop_writes_trigger",
                12,
                "rocksdb options.level0_stop_writes_trigger, default 12"
                );

            // disable write ahead logging as replication handles logging instead now
            _wt_opts.disableWAL = true;
        }

        int64_t rrdb_service_impl::parse_checkpoints()
        {
            std::vector<std::string> dirs;
            utils::filesystem::get_subdirectories(data_dir(), dirs, false);

            utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);

            _checkpoints.clear();
            for (auto& d : dirs)
            {
                int64_t ci;
                std::string d1 = d.substr(strlen(data_dir()) + 1);
                if (chkpt_init_from_dir(d1.c_str(), ci))
                {
                    _checkpoints.push_back(ci);
                }
                else if (d1.find("checkpoint") != std::string::npos)
                {
                    dwarn("invalid checkpoint directory %s, remove ...",
                        d.c_str()
                        );
                    utils::filesystem::remove_path(d);
                }
            }

            std::sort(_checkpoints.begin(), _checkpoints.end());

            return _checkpoints.size() > 0 ? *_checkpoints.rbegin() : 0;
        }

        void rrdb_service_impl::gc_checkpoints()
        {
            while (true)
            {
                int64_t del_d;

                {
                    utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                    if (_checkpoints.size() <= _max_checkpoint_count)
                        break;
                    del_d = *_checkpoints.begin();
                    _checkpoints.erase(_checkpoints.begin());
                }

                auto old_cpt = chkpt_get_dir_name(del_d);
                auto old_cpt_dir = utils::filesystem::path_combine(data_dir(), old_cpt);
                if (utils::filesystem::directory_exists(old_cpt_dir))
                {
                    if (utils::filesystem::remove_path(old_cpt_dir))
                    {
                        ddebug("%s: checkpoint %s removed by garbage collection", data_dir(), old_cpt_dir.c_str());
                    }
                    else
                    {
                        derror("%s: remove checkpoint %s failed by garbage collection", data_dir(), old_cpt_dir.c_str());
                        {
                            // put back if remove failed
                            utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                            _checkpoints.push_back(del_d);
                            std::sort(_checkpoints.begin(), _checkpoints.end());
                        }
                        break;
                    }
                }
                else
                {
                    dwarn("%s: checkpoint %s does not exist, garbage collection ignored", data_dir(), old_cpt_dir.c_str());
                }
            }
        }

        
        void rrdb_service_impl::on_put(const update_request& update, ::dsn::rpc_replier<int>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir());
                        
            rocksdb::Slice skey(update.key.data(), update.key.length());
            rocksdb::Slice svalue(update.value.data(), update.value.length());

            auto status = _db->Put(_wt_opts, skey, svalue);
            if (!status.ok())
            {
                derror("%s failed, status = %s", __FUNCTION__, status.ToString().c_str());
                set_physical_error(status.code());
            }

            reply(status.code());
        }

        void rrdb_service_impl::on_remove(const ::dsn::blob& key, ::dsn::rpc_replier<int>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir());

            rocksdb::Slice skey(key.data(), key.length());

            auto status = _db->Delete(_wt_opts, skey);
            if (!status.ok())
            {
                derror("%s failed, status = %s", __FUNCTION__, status.ToString().c_str());
                set_physical_error(status.code());
            }

            reply(status.code());
        }

        void rrdb_service_impl::on_merge(const update_request& update, ::dsn::rpc_replier<int>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir());

            rocksdb::Slice skey(update.key.data(), update.key.length());
            rocksdb::Slice svalue(update.value.data(), update.value.length());

            auto status = _db->Merge(_wt_opts, skey, svalue);
            if (!status.ok())
            {
                derror("%s failed, status = %s", __FUNCTION__, status.ToString().c_str());
                set_physical_error(status.code());
            }

            reply(status.code());
        }

        void rrdb_service_impl::on_get(const ::dsn::blob& key, ::dsn::rpc_replier<read_response>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir());

            read_response resp;
            rocksdb::Slice skey(key.data(), key.length());
            rocksdb::Status status = _db->Get(_rd_opts, skey, &resp.value);
            if (!status.ok() && !status.IsNotFound())
            {
                derror("%s failed, status = %s", __FUNCTION__, status.ToString().c_str());
            }
            resp.error = status.code();
            reply(resp);
        }

        ::dsn::error_code rrdb_service_impl::start(int argc, char** argv)
        {
            dassert(!_is_open, "rrdb service %s is already opened", data_dir());
			
			auto path = utils::filesystem::path_combine(data_dir(), "rdb");
			::dsn::utils::filesystem::remove_path(path);

            rocksdb::Options opts = _db_opts;
            opts.create_if_missing = true;
            opts.error_if_exists = false;

            int64_t last_checkpoint;
            last_checkpoint = parse_checkpoints();
            gc_checkpoints();
						
			if (last_checkpoint > 0)
			{
				auto cname = chkpt_get_dir_name(last_checkpoint);
                auto cpath = utils::filesystem::path_combine(data_dir(), cname);
				bool r = ::dsn::utils::filesystem::rename_path(cpath, path);
				dassert(r, "");
			}
			else
			{
				::dsn::utils::filesystem::create_directory(path);
			}

            auto status = rocksdb::DB::Open(opts, path, &_db);
            if (status.ok())
            {
				if (last_checkpoint > 0)
				{
					auto err = checkpoint_internal(last_checkpoint);
					dassert(err == ERR_OK, "");
				}

                set_last_durable_decree(last_checkpoint);

                ddebug("open app %s: durable_decree = %" PRId64 "",
                    data_dir(), last_durable_decree()
                    );

                _is_open = true;

				open_service(gpid());
                return ERR_OK;
            }
            else
            {
                derror("open rocksdb %s failed, status = %s",
                    path.c_str(),
                    status.ToString().c_str()
                    );
                return ERR_LOCAL_APP_FAILURE;
            }
        }

        ::dsn::error_code rrdb_service_impl::stop(bool clear_state)
        {
            if (!_is_open)
            {
                dassert(_db == nullptr, "");
                dassert(!clear_state, "should not be here if do clear");
                return ERR_OK;
            }

            close_service(gpid());

            _is_open = false;
            delete _db;
            _db = nullptr;

            _is_checkpointing = false;
            {
                utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                _checkpoints.clear();
            }

            if (clear_state)
            {
                if (!utils::filesystem::remove_path(data_dir()))
                {
                    return ERR_FILE_OPERATION_FAILED;
                }
            }

            return ERR_OK;
        }

		::dsn::error_code rrdb_service_impl::checkpoint_internal(int64_t version)
		{
			rocksdb::Checkpoint* chkpt = nullptr;
			auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
			if (!status.ok())
			{
				derror("%s: create Checkpoint object failed, status = %s",
					data_dir(),
					status.ToString().c_str()
				);
				_is_checkpointing = false;
				return ERR_LOCAL_APP_FAILURE;
			}

			auto dir = chkpt_get_dir_name(version);
			auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
			if (utils::filesystem::directory_exists(chkpt_dir))
			{
				dwarn("%s: checkpoint directory %s already exist, remove it first",
					data_dir(),
					chkpt_dir.c_str()
				);
				if (!utils::filesystem::remove_path(chkpt_dir))
				{
					derror("%s: remove old checkpoint directory %s failed",
						data_dir(),
						chkpt_dir.c_str()
					);
					_is_checkpointing = false;
					return ERR_FILE_OPERATION_FAILED;
				}
			}

			status = chkpt->CreateCheckpoint(chkpt_dir);
			delete chkpt;
			chkpt = nullptr;
			if (!status.ok())
			{
				derror("%s: create checkpoint failed, status = %s",
					data_dir(),
					status.ToString().c_str()
				);
				utils::filesystem::remove_path(chkpt_dir);
				_is_checkpointing = false;
				return ERR_LOCAL_APP_FAILURE;
			}

			ddebug("%s: create checkpoint %s succeed",
				data_dir(),
				chkpt_dir.c_str()
			);

			return ERR_OK;
		}

        ::dsn::error_code rrdb_service_impl::checkpoint(int64_t version)
        {
            if (!_is_open || _is_checkpointing)
                return ERR_WRONG_TIMING;

            if (last_durable_decree() == version)
                return ERR_NO_NEED_OPERATE;

            _is_checkpointing = true;
            
			auto err = checkpoint_internal(version);
			if (err != ERR_OK) return err;

            {
                utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                _checkpoints.push_back(version);
                std::sort(_checkpoints.begin(), _checkpoints.end());
                set_last_durable_decree(_checkpoints.back());
            }

            gc_checkpoints();

            _is_checkpointing = false;
            return ERR_OK;
        }

        // Must be thread safe.
        ::dsn::error_code rrdb_service_impl::checkpoint_async(int64_t version)
        {
            return ERR_NOT_IMPLEMENTED;

            /*if (!_is_open || _is_checkpointing)
                return ERR_WRONG_TIMING;

            if (last_durable_decree() == version)
                return ERR_NO_NEED_OPERATE;

            _is_checkpointing = true;

            rocksdb::Checkpoint* chkpt = nullptr;
            auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
            if (!status.ok())
            {
                derror("%s: create Checkpoint object failed, status = %s",
                    data_dir(),
                    status.ToString().c_str()
                    );
                _is_checkpointing = false;
                return ERR_LOCAL_APP_FAILURE;
            }

            uint64_t decree = 0;
            {
                utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                if (!_checkpoints.empty())
                {
                    decree = _checkpoints.back();
                }
            }

            char buf[256];
            sprintf(buf, "checkpoint.tmp.%" PRIu64 "", dsn_now_us());
            std::string tmp_dir = utils::filesystem::path_combine(data_dir(), buf);
            if (utils::filesystem::directory_exists(tmp_dir))
            {
                dwarn("%s: tmp directory %s already exist, remove it first",
                    data_dir(),
                    tmp_dir.c_str()
                    );
                if (!utils::filesystem::remove_path(tmp_dir))
                {
                    derror("%s: remove old tmp directory %s failed",
                        data_dir(),
                        tmp_dir.c_str()
                        );
                    _is_checkpointing = false;
                    return ERR_FILE_OPERATION_FAILED;
                }
            }

            status = chkpt->CreateCheckpointQuick(tmp_dir, &decree);
            delete chkpt;
            chkpt = nullptr;
            if (!status.ok())
            {
                derror("%s: async create checkpoint failed, status = %s",
                    data_dir(),
                    status.ToString().c_str()
                    );
                utils::filesystem::remove_path(tmp_dir);
                _is_checkpointing = false;
                return status.IsTryAgain() ? ERR_TRY_AGAIN : ERR_LOCAL_APP_FAILURE;
            }

            int64_t ci;
            auto dir = chkpt_get_dir_name(ci);
            auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
            if (utils::filesystem::directory_exists(chkpt_dir))
            {
                dwarn("%s: checkpoint directory %s already exist, remove it first",
                    data_dir(),
                    chkpt_dir.c_str()
                    );
                if (!utils::filesystem::remove_path(chkpt_dir))
                {
                    derror("%s: remove old checkpoint directory %s failed",
                        data_dir(),
                        chkpt_dir.c_str()
                        );
                    utils::filesystem::remove_path(tmp_dir);
                    _is_checkpointing = false;
                    return ERR_FILE_OPERATION_FAILED;
                }
            }

            if (!utils::filesystem::rename_path(tmp_dir, chkpt_dir))
            {
                derror("%s: rename checkpoint directory from %s to %s failed",
                    data_dir(),
                    tmp_dir.c_str(),
                    chkpt_dir.c_str()
                    );
                utils::filesystem::remove_path(tmp_dir);
                _is_checkpointing = false;
                return ERR_FILE_OPERATION_FAILED;
            }

            {
                utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                _checkpoints.push_back(ci);
                std::sort(_checkpoints.begin(), _checkpoints.end());
                set_last_durable_decree(_checkpoints.back());
            }

            ddebug("%s: async create checkpoint %s succeed",
                data_dir(),
                chkpt_dir.c_str()
                );

            gc_checkpoints();

            _is_checkpointing = false;
            return ERR_OK;*/
        }
        
        ::dsn::error_code rrdb_service_impl::get_checkpoint(
            int64_t start,
            int64_t local_commit,
            void*   learn_request,
            int     learn_request_size,
            /* inout */ app_learn_state& state
            )
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir());

            int64_t ci;
            {
                utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                if (_checkpoints.size() > 0)
                    ci = *_checkpoints.rbegin();
            }

            if (ci == 0)
            {
                derror("%s: no checkpoint found", data_dir());
                return ERR_OBJECT_NOT_FOUND;
            }

            auto dir = chkpt_get_dir_name(ci);
            auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);

            state.files.clear();
            auto succ = utils::filesystem::get_subfiles(chkpt_dir, state.files, true);
            if (!succ)
            {
                derror("%s: list files in checkpoint dir %s failed", data_dir(), chkpt_dir.c_str());
                return ERR_FILE_OPERATION_FAILED;
            }

            // attach checkpoint info
            binary_writer writer;
            writer.write_pod(ci);
            state.meta_state = std::move(writer.get_buffer());

            state.from_decree_excluded = 0;
            state.to_decree_included = ci;

            ddebug("%s: get checkpoint succeed, from_decree_excluded = 0, to_decree_included = %" PRId64,
                   data_dir(),
                   state.to_decree_included);
            return ERR_OK;
        }

        ::dsn::error_code rrdb_service_impl::apply_checkpoint(int64_t local_commit, const dsn_app_learn_state& state, dsn_chkpt_apply_mode mode)
        {
            ::dsn::error_code err;
            int64_t ci;

            dassert(state.meta_state_size > 0, "the learned state does not have checkpint meta info");
            binary_reader reader(blob((const char*)state.meta_state_ptr, 0, state.meta_state_size));
            reader.read_pod(ci);

            if (mode == DSN_CHKPT_COPY)
            {
                dassert(state.to_decree_included > last_durable_decree()
                    && ci == state.to_decree_included,
                    "");

                auto dir = chkpt_get_dir_name(ci);
                auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
                auto old_dir = utils::filesystem::remove_file_name(state.files[0]);
                auto succ = utils::filesystem::rename_path(old_dir, chkpt_dir);

                if (succ)
                {
                    utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
                    _checkpoints.push_back(ci);
                    set_last_durable_decree(ci);
                    err = ERR_OK;
                }
                else
                {
                    err = ERR_FILE_OPERATION_FAILED;
                }                
                return err;
            }

            if (_is_open)
            {
                err = stop(true);
                if (err != ERR_OK)
                {
                    derror("close rocksdb %s failed, err = %s", data_dir(), err.to_string());
                    return err;
                }
            }

            // clear data dir
            if (!utils::filesystem::remove_path(data_dir()))
            {
                derror("clear data directory %s failed", data_dir());
                return ERR_FILE_OPERATION_FAILED;
            }

            // reopen the db with the new checkpoint files
            if (state.file_state_count > 0)
            {
                // create data dir
                if (!utils::filesystem::create_directory(data_dir()))
                {
                    derror("create data directory %s failed", data_dir());
                    return ERR_FILE_OPERATION_FAILED;
                }

                // move learned files from learn_dir to data_dir/rdb
                std::string learn_dir = utils::filesystem::remove_file_name(std::string(state.files[state.file_state_count - 1]));
                std::string new_dir = utils::filesystem::path_combine(data_dir(), "rdb");
                if (!utils::filesystem::rename_path(learn_dir, new_dir))
                {
                    derror("rename %s to %s failed", learn_dir.c_str(), new_dir.c_str());
                    return ERR_FILE_OPERATION_FAILED;
                }

                err = start(0, nullptr);
            }
            else
            {
                dwarn("%s: apply empty checkpoint, create new rocksdb", data_dir());
                err = start(0, nullptr);
            }

            if (err != ERR_OK)
            {
                derror("open rocksdb %s failed, err = %s", data_dir(), err.to_string());
                return err;
            }

            // because there is no checkpoint in the new db
            dassert(_is_open, "");
            dassert(last_durable_decree() == 0, "");

            // checkpoint immediately if need
            if (state.to_decree_included > 0)
            {
                err = checkpoint(state.to_decree_included);
                if (err != ERR_OK)
                {
                    derror("%s: checkpoint failed, err = %s", data_dir(), err.to_string());
                    return err;
                }
                dassert(state.to_decree_included == last_durable_decree(),
                    "commit and durable decree mismatch after checkpoint: %" PRId64 " vs %" PRId64,
                    state.to_decree_included,
                    last_durable_decree()
                    );
            }

            ddebug("%s: apply checkpoint succeed, last_durable_decree = %" PRId64,
                   data_dir(),
                   state.to_decree_included
                );
            return ERR_OK;
        }
    }
}
