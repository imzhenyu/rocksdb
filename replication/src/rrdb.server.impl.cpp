# include "rrdb.server.impl.h"
# include "key_utils.h"
# include <algorithm>
# include <dsn/utility/utils.h>
# include <rocksdb/utilities/checkpoint.h>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "rrdb.server.impl"

namespace dsn { namespace apps {
        
static std::string chkpt_get_dir_name(int64_t decree)
{
    char buffer[256];
    sprintf(buffer, "checkpoint.%" PRId64 "", decree);
    return std::string(buffer);
}

static bool chkpt_init_from_dir(const char* name, int64_t& decree)
{
    return 1 == sscanf(name, "checkpoint.%" PRId64 "", &decree)
        && std::string(name) == chkpt_get_dir_name(decree);
}

rrdb_service_impl::rrdb_service_impl(dsn_gpid gpid) 
    : ::dsn::replicated_service_app_type_1(gpid),
      _gpid(gpid),
      _db(nullptr),
      _is_open(false),
      _physical_error(0),
      _max_checkpoint_count(3),
      _is_checkpointing(false)
{
    _primary_address = ::dsn::rpc_address(dsn_primary_address()).to_string();
    char buf[256];
    sprintf(buf, "%u.%u@%s", gpid.u.app_id, gpid.u.partition_index, _primary_address.c_str());
    _replica_name = buf;
    _data_dir = dsn_get_app_data_dir(gpid);

    _log_on_not_found = dsn_config_get_value_bool("replication",
              "rocksdb_log_on_not_found",
              false,
              "print error log if data not found, default is false"
              );

    // init db options

    // rocksdb default: 4MB
    _db_opts.write_buffer_size =
        (size_t)dsn_config_get_value_uint64("replication",
                "rocksdb_write_buffer_size",
                16777216,
                "rocksdb options.write_buffer_size, default 16MB"
                );

    // rocksdb default: 2
    _db_opts.max_write_buffer_number =
        (int)dsn_config_get_value_uint64("replication",
                "rocksdb_max_write_buffer_number",
                2,
                "rocksdb options.max_write_buffer_number, default 2"
                );

    // rocksdb default: 1
    _db_opts.max_background_compactions =
        (int)dsn_config_get_value_uint64("replication",
                "rocksdb_max_background_compactions",
                2,
                "rocksdb options.max_background_compactions, default 2"
                );

    // rocksdb default: 7
    _db_opts.num_levels =
        dsn_config_get_value_uint64("replication",
                "rocksdb_num_levels",
                6,
                "rocksdb options.num_levels, default 6"
                );

    // rocksdb default: 2MB
    _db_opts.target_file_size_base =
        dsn_config_get_value_uint64("replication",
                "rocksdb_target_file_size_base",
                8388608,
                "rocksdb options.write_buffer_size, default 8MB"
                );

    // rocksdb default: 10MB
    _db_opts.max_bytes_for_level_base =
        dsn_config_get_value_uint64("replication",
                "rocksdb_max_bytes_for_level_base",
                41943040,
                "rocksdb options.max_bytes_for_level_base, default 40MB"
                );

    // rocksdb default: 10
    _db_opts.max_grandparent_overlap_factor =
        (int)dsn_config_get_value_uint64("replication",
                "rocksdb_max_grandparent_overlap_factor",
                10,
                "rocksdb options.max_grandparent_overlap_factor, default 10"
                );

    // rocksdb default: 4
    _db_opts.level0_file_num_compaction_trigger =
        (int)dsn_config_get_value_uint64("replication",
                "rocksdb_level0_file_num_compaction_trigger",
                4,
                "rocksdb options.level0_file_num_compaction_trigger, 4"
                );

    // rocksdb default: 20
    _db_opts.level0_slowdown_writes_trigger =
        (int)dsn_config_get_value_uint64("replication",
                "rocksdb_level0_slowdown_writes_trigger",
                20,
                "rocksdb options.level0_slowdown_writes_trigger, default 20"
                );

    // rocksdb default: 24
    _db_opts.level0_stop_writes_trigger =
        (int)dsn_config_get_value_uint64("replication",
                "rocksdb_level0_stop_writes_trigger",
                24,
                "rocksdb options.level0_stop_writes_trigger, default 24"
                );

    // disable write ahead logging as replication handles logging instead now
    _wt_opts.disableWAL = true;
}

void rrdb_service_impl::parse_checkpoints()
{
    std::vector<std::string> dirs;
    utils::filesystem::get_subdirectories(_data_dir, dirs, false);

    utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);

    _checkpoints.clear();
    for (auto& d : dirs)
    {
        int64_t ci;
        std::string d1 = d.substr(_data_dir.size() + 1);
        if (chkpt_init_from_dir(d1.c_str(), ci))
        {
            _checkpoints.push_back(ci);
        }
        else if (d1.find("checkpoint") != std::string::npos)
        {
            ddebug("%s: invalid checkpoint directory %s, remove it",
                   _replica_name.c_str(), d.c_str());
            utils::filesystem::remove_path(d);
            if (!utils::filesystem::remove_path(d))
            {
                derror("%s: remove invalid checkpoint directory %s failed",
                       _replica_name.c_str(), d.c_str());
            }
        }
    }

    if (!_checkpoints.empty())
    {
        std::sort(_checkpoints.begin(), _checkpoints.end());
        set_last_durable_decree(_checkpoints.back());
    }
    else
    {
        set_last_durable_decree(0);
    }
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
            del_d = _checkpoints.front();
            _checkpoints.erase(_checkpoints.begin());
        }

        auto old_cpt_dir = utils::filesystem::path_combine(_data_dir, chkpt_get_dir_name(del_d));
        if (utils::filesystem::directory_exists(old_cpt_dir))
        {
            if (utils::filesystem::remove_path(old_cpt_dir))
            {
                ddebug("%s: checkpoint directory %s removed by garbage collection",
                       _replica_name.c_str(), old_cpt_dir.c_str());
            }
            else
            {
                derror("%s: checkpoint directory %s remove failed by garbage collection",
                       _replica_name.c_str(), old_cpt_dir.c_str());
            }
        }
        else
        {
            ddebug("%s: checkpoint directory %s does not exist, ignored by garbage collection",
                   _replica_name.c_str(), old_cpt_dir.c_str());
        }
    }
}

void rrdb_service_impl::on_batched_write_requests(int64_t decree, dsn_message_t* requests, int count)
{
    dassert(_is_open, "");
    dassert(requests != nullptr, "");

    if (count == 0)
    {
        // write fake data
        // TODO(qinzuoyan): maybe no need to write fake data, just write empty batch
        _batch.Put(rocksdb::Slice(), rocksdb::Slice());
        _batch_repliers.emplace_back(nullptr);
    }
    else
    {
        for (int i = 0; i < count; ++i)
        {
            dsn_message_t request = requests[i];
            dassert(request != nullptr, "");
            ::dsn::message_ex* msg = (::dsn::message_ex*)request;
            if (msg->local_rpc_code == RPC_RRDB_RRDB_PUT)
            {
                update_request update;
                ::dsn::unmarshall(request, update);
                if (msg->header->client.partition_hash != 0)
                {
                    uint64_t partition_hash = rrdb_key_hash(update.key);
                    dassert(msg->header->client.partition_hash == partition_hash, "inconsistent partition hash");
                    int thread_hash = dsn_gpid_to_thread_hash(_gpid);
                    dassert(msg->header->client.thread_hash == thread_hash, "inconsistent thread hash");
                }

                rocksdb::Slice skey(update.key.data(), update.key.length());
                rocksdb::Slice svalue(update.value.data(), update.value.length());
                _batch.Put(skey, svalue);
                _batch_repliers.emplace_back(dsn_msg_create_response(request));
            }
            else if (msg->local_rpc_code == RPC_RRDB_RRDB_REMOVE)
            {
                ::dsn::blob key;
                ::dsn::unmarshall(request, key);
                if (msg->header->client.partition_hash != 0)
                {
                    uint64_t partition_hash = rrdb_key_hash(key);
                    dassert(msg->header->client.partition_hash == partition_hash, "inconsistent partition hash");
                    int thread_hash = dsn_gpid_to_thread_hash(_gpid);
                    dassert(msg->header->client.thread_hash == thread_hash, "inconsistent thread hash");
                }

                rocksdb::Slice skey(key.data(), key.length());
                _batch.Delete(skey);
                _batch_repliers.emplace_back(dsn_msg_create_response(request));
            }
            else
            {
                dassert(false, "rpc code not handled: %s", task_code(msg->local_rpc_code).to_string());
            }
        }
    }

    _wt_opts.given_decree = decree;
    auto status = _db->Write(_wt_opts, &_batch);
    if (!status.ok())
    {
        derror("%s: write rocksdb failed, decree = %" PRId64 ", error = %s",
               _replica_name.c_str(), decree, status.ToString().c_str());
        _physical_error = status.code();
    }

    update_response resp;
    resp.error = status.code();
    resp.app_id = _gpid.u.app_id;
    resp.partition_index = _gpid.u.partition_index;
    resp.decree = decree;
    resp.server = _primary_address;
    for (auto& r : _batch_repliers)
    {
        if (!r.is_empty())
        {
            r(resp);
        }
    }

    _batch.Clear();
    _batch_repliers.clear();
}

void rrdb_service_impl::on_put(const update_request& update, ::dsn::rpc_replier<update_response>& reply)
{
    dassert(false, "not implemented");
}

void rrdb_service_impl::on_remove(const ::dsn::blob& key, ::dsn::rpc_replier<update_response>& reply)
{
    dassert(false, "not implemented");
}

void rrdb_service_impl::on_get(const ::dsn::blob& key, ::dsn::rpc_replier<read_response>& reply)
{
    dassert(_is_open, "");

    read_response resp;
    auto id = get_gpid();
    resp.app_id = id.u.app_id;
    resp.partition_index = id.u.partition_index;
    resp.server = _primary_address;

    rocksdb::Slice skey(key.data(), key.length());
    std::string* value = new std::string();
    rocksdb::Status status = _db->Get(_rd_opts, skey, value);
    if (!status.ok() && (!status.IsNotFound() || _log_on_not_found))
    {
        derror("%s: read rocksdb failed, error = %s",
               _replica_name.c_str(), status.ToString().c_str());
    }

    resp.error = status.code();
    if (status.ok() && value->size() > 0)
    {
        // tricy code to avoid memory copy
        std::shared_ptr<char> b(&value->front(), [value](char*){delete value;});
        resp.value.assign(b, 0, value->size());
    }
    else
    {
        delete value;
    }
    reply(resp);
}

::dsn::error_code rrdb_service_impl::start(int argc, char** argv)
{
    dassert(!_is_open, "");
    ddebug("%s: start to open app %s",
           _replica_name.c_str(), _data_dir.c_str());

    rocksdb::Options opts = _db_opts;
    opts.create_if_missing = true;
    opts.error_if_exists = false;

    auto path = utils::filesystem::path_combine(_data_dir, "rdb");
    auto status = rocksdb::DB::Open(opts, path, &_db);
    if (status.ok())
    {
        parse_checkpoints();

        int64_t ci = _db->GetLastFlushedDecree();
        if (ci != last_durable_decree())
        {
            ddebug("%s: start to do async checkpoint, last_durable_decree = %" PRId64 ", last_flushed_decree = %" PRId64,
                   _replica_name.c_str(), last_durable_decree(), ci);
            auto err = async_checkpoint(ci);
            if (err != ERR_OK)
            {
                derror("%s: create checkpoint failed, error = %s",
                        _replica_name.c_str(), err.to_string());
                delete _db;
                _db = nullptr;
                return err;
            }
            dassert(ci == last_durable_decree(),
                    "last durable decree mismatch after checkpoint: %" PRId64 " vs %" PRId64,
                    ci, last_durable_decree());
        }

        ddebug("%s: open app succeed, last_durable_decree = %" PRId64 "",
               _replica_name.c_str(), last_durable_decree());

        _is_open = true;

        open_service(get_gpid());

        return ERR_OK;
    }
    else
    {
        derror("%s: open app failed, error = %s",
               _replica_name.c_str(), status.ToString().c_str());
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

    close_service(get_gpid());

    _is_open = false;
    delete _db;
    _db = nullptr;

    _is_checkpointing = false;
    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        _checkpoints.clear();
        set_last_durable_decree(0);
    }

    if (clear_state)
    {
        if (!utils::filesystem::remove_path(_data_dir))
        {
            derror("%s: clear directory %s failed when stop app",
                   _replica_name.c_str(), _data_dir.c_str());
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    return ERR_OK;
}

::dsn::error_code rrdb_service_impl::sync_checkpoint(int64_t last_commit)
{
    if (!_is_open || _is_checkpointing)
        return ERR_WRONG_TIMING;

    if (last_durable_decree() == last_commit)
        return ERR_NO_NEED_OPERATE;

    _is_checkpointing = true;
    
    rocksdb::Checkpoint* chkpt = nullptr;
    auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
    if (!status.ok())
    {
        derror("%s: create Checkpoint object failed, error = %s",
               _replica_name.c_str(), status.ToString().c_str());
        _is_checkpointing = false;
        return ERR_LOCAL_APP_FAILURE;
    }

    auto dir = chkpt_get_dir_name(last_commit);
    auto chkpt_dir = utils::filesystem::path_combine(_data_dir, dir);
    if (utils::filesystem::directory_exists(chkpt_dir))
    {
        ddebug("%s: checkpoint directory %s already exist, remove it first",
               _replica_name.c_str(), chkpt_dir.c_str());
        if (!utils::filesystem::remove_path(chkpt_dir))
        {
            derror("%s: remove old checkpoint directory %s failed",
                   _replica_name.c_str(), chkpt_dir.c_str());
            delete chkpt;
            chkpt = nullptr;
            _is_checkpointing = false;
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    status = chkpt->CreateCheckpoint(chkpt_dir);
    if (!status.ok())
    {
        // sometimes checkpoint may fail, and try again will succeed
        derror("%s: create checkpoint failed, error = %s, try again",
               _replica_name.c_str(), status.ToString().c_str());
        status = chkpt->CreateCheckpoint(chkpt_dir);
    }

    // destroy Checkpoint object
    delete chkpt;
    chkpt = nullptr;

    if (!status.ok())
    {
        derror("%s: create checkpoint failed, error = %s",
               _replica_name.c_str(), status.ToString().c_str());
        utils::filesystem::remove_path(chkpt_dir);
        if (!utils::filesystem::remove_path(chkpt_dir))
        {
            derror("%s: remove damaged checkpoint directory %s failed",
                   _replica_name.c_str(), chkpt_dir.c_str());
        }
        _is_checkpointing = false;
        return ERR_LOCAL_APP_FAILURE;
    }

    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        dassert(last_commit > last_durable_decree(), "");
        _checkpoints.push_back(last_commit);
        set_last_durable_decree(_checkpoints.back());
    }

    ddebug("%s: sync create checkpoint succeed, last_durable_decree = %" PRId64 "",
           _replica_name.c_str(), last_durable_decree());

    gc_checkpoints();

    _is_checkpointing = false;
    return ERR_OK;
}

// Must be thread safe.
::dsn::error_code rrdb_service_impl::async_checkpoint(int64_t /*last_commit*/)
{
    if (_is_checkpointing)
        return ERR_WRONG_TIMING;

    if (last_durable_decree() == _db->GetLastFlushedDecree())
        return ERR_NO_NEED_OPERATE;

    _is_checkpointing = true;

    rocksdb::Checkpoint* chkpt = nullptr;
    auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
    if (!status.ok())
    {
        derror("%s: create Checkpoint object failed, error = %s",
               _replica_name.c_str(), status.ToString().c_str());
        _is_checkpointing = false;
        return ERR_LOCAL_APP_FAILURE;
    }

    char buf[256];
    sprintf(buf, "checkpoint.tmp.%" PRIu64 "", dsn_now_us());
    std::string tmp_dir = utils::filesystem::path_combine(_data_dir, buf);
    if (utils::filesystem::directory_exists(tmp_dir))
    {
        ddebug("%s: temporary checkpoint directory %s already exist, remove it first",
               _replica_name.c_str(), tmp_dir.c_str());
        if (!utils::filesystem::remove_path(tmp_dir))
        {
            derror("%s: remove temporary checkpoint directory %s failed",
                   _replica_name.c_str(), tmp_dir.c_str());
            _is_checkpointing = false;
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    uint64_t ci = last_durable_decree();
    status = chkpt->CreateCheckpointQuick(tmp_dir, &ci);
    delete chkpt;
    chkpt = nullptr;
    if (!status.ok())
    {
        derror("%s: async create checkpoint failed, error = %s",
               _replica_name.c_str(), status.ToString().c_str());
        if (!utils::filesystem::remove_path(tmp_dir))
        {
            derror("%s: remove temporary checkpoint directory %s failed",
                   _replica_name.c_str(), tmp_dir.c_str());
        }
        _is_checkpointing = false;
        return status.IsTryAgain() ? ERR_TRY_AGAIN : ERR_LOCAL_APP_FAILURE;
    }

    auto chkpt_dir = utils::filesystem::path_combine(_data_dir, chkpt_get_dir_name(ci));
    if (utils::filesystem::directory_exists(chkpt_dir))
    {
        ddebug("%s: checkpoint directory %s already exist, remove it first",
               _replica_name.c_str(), chkpt_dir.c_str());
        if (!utils::filesystem::remove_path(chkpt_dir))
        {
            derror("%s: remove old checkpoint directory %s failed",
                   _replica_name.c_str(), chkpt_dir.c_str());
            if (!utils::filesystem::remove_path(tmp_dir))
            {
                derror("%s: remove temporary checkpoint directory %s failed",
                       _replica_name.c_str(), tmp_dir.c_str());
            }
            _is_checkpointing = false;
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    if (!utils::filesystem::rename_path(tmp_dir, chkpt_dir))
    {
        derror("%s: rename checkpoint directory from %s to %s failed",
               _replica_name.c_str(), tmp_dir.c_str(), chkpt_dir.c_str());
        if (!utils::filesystem::remove_path(tmp_dir))
        {
            derror("%s: remove temporary checkpoint directory %s failed",
                   _replica_name.c_str(), tmp_dir.c_str());
        }
        _is_checkpointing = false;
        return ERR_FILE_OPERATION_FAILED;
    }

    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        dassert(static_cast<int64_t>(ci) > last_durable_decree(), "");
        _checkpoints.push_back(ci);
        set_last_durable_decree(_checkpoints.back());
    }

    ddebug("%s: async create checkpoint succeed, last_durable_decree = %" PRId64 "",
           _replica_name.c_str(), last_durable_decree());

    gc_checkpoints();

    _is_checkpointing = false;
    return ERR_OK;
}

::dsn::error_code rrdb_service_impl::get_checkpoint(
    int64_t learn_start,
    int64_t local_commit,
    void*   learn_request,
    int     learn_request_size,
    app_learn_state& state
    )
{
    dassert(_is_open, "");

    int64_t ci = last_durable_decree();
    if (ci == 0)
    {
        derror("%s: no checkpoint found", _replica_name.c_str());
        return ERR_OBJECT_NOT_FOUND;
    }

    auto chkpt_dir = utils::filesystem::path_combine(_data_dir, chkpt_get_dir_name(ci));
    state.files.clear();
    if (!utils::filesystem::get_subfiles(chkpt_dir, state.files, true))
    {
        derror("%s: list files in checkpoint dir %s failed",
               _replica_name.c_str(), chkpt_dir.c_str());
        return ERR_FILE_OPERATION_FAILED;
    }

    state.from_decree_excluded = 0;
    state.to_decree_included = ci;

    ddebug("%s: get checkpoint succeed, from_decree_excluded = 0, to_decree_included = %" PRId64 "",
           _replica_name.c_str(), state.to_decree_included);
    return ERR_OK;
}

::dsn::error_code rrdb_service_impl::apply_checkpoint(
    dsn_chkpt_apply_mode mode,
    int64_t local_commit,
    const dsn_app_learn_state& state
    )
{
    ::dsn::error_code err;
    int64_t ci = state.to_decree_included;

    if (mode == DSN_CHKPT_COPY)
    {
        dassert(ci > last_durable_decree(),
                "state.to_decree_included(%" PRId64 ") <= last_durable_decree(%" PRId64 ")",
                ci, last_durable_decree());

        auto learn_dir = utils::filesystem::remove_file_name(state.files[0]);
        auto chkpt_dir = utils::filesystem::path_combine(_data_dir, chkpt_get_dir_name(ci));
        if (utils::filesystem::rename_path(learn_dir, chkpt_dir))
        {
            utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
            dassert(ci > last_durable_decree(), "");
            _checkpoints.push_back(ci);
            set_last_durable_decree(ci);
            err = ERR_OK;
        }
        else
        {
            derror("%s: rename directory %s to %s failed",
                   _replica_name.c_str(), learn_dir.c_str(), chkpt_dir.c_str());
            err = ERR_FILE_OPERATION_FAILED;
        }

        return err;
    }

    if (_is_open)
    {
        err = stop(true);
        if (err != ERR_OK)
        {
            derror("%s: close rocksdb %s failed, error = %s",
                   _replica_name.c_str(), err.to_string());
            return err;
        }
    }

    // clear data dir
    if (!utils::filesystem::remove_path(_data_dir))
    {
        derror("%s: clear data directory %s failed",
               _replica_name.c_str(), _data_dir.c_str());
        return ERR_FILE_OPERATION_FAILED;
    }

    // reopen the db with the new checkpoint files
    if (state.file_state_count > 0)
    {
        // create data dir
        if (!utils::filesystem::create_directory(_data_dir))
        {
            derror("%s: create data directory %s failed",
                   _replica_name.c_str(), _data_dir.c_str());
            return ERR_FILE_OPERATION_FAILED;
        }

        // move learned files from learn_dir to data_dir/rdb
        std::string learn_dir = utils::filesystem::remove_file_name(state.files[0]);
        std::string new_dir = utils::filesystem::path_combine(_data_dir, "rdb");
        if (!utils::filesystem::rename_path(learn_dir, new_dir))
        {
            derror("%s: rename directory %s to %s failed",
                   _replica_name.c_str(), learn_dir.c_str(), new_dir.c_str());
            return ERR_FILE_OPERATION_FAILED;
        }

        err = start(0, nullptr);
    }
    else
    {
        ddebug("%s: apply empty checkpoint, create new rocksdb", _replica_name.c_str());
        err = start(0, nullptr);
    }

    if (err != ERR_OK)
    {
        derror("%s: open rocksdb failed, error = %s",
               _replica_name.c_str(), err.to_string());
        return err;
    }

    dassert(_is_open, "");
    dassert(ci == last_durable_decree(), "");

    ddebug("%s: apply checkpoint succeed, last_durable_decree = %" PRId64,
           _replica_name.c_str(), last_durable_decree());
    return ERR_OK;
}

} }
