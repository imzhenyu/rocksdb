# include "rrdb.server.impl.h"
# include <algorithm>
# include <dsn/cpp/utils.h>
# include <rocksdb/utilities/checkpoint.h>

# ifdef __TITLE__
# undef __TITLE__
# endif
#define __TITLE__ "rrdb.server.impl"

namespace dsn { namespace apps {
        
static std::string chkpt_get_dir_name(::dsn::replication::decree d, rocksdb::SequenceNumber seq)
{
    char buffer[256];
    sprintf(buffer, "checkpoint.%" PRId64 ".%" PRIu64 "", d, seq);
    return std::string(buffer);
}

static bool chkpt_init_from_dir(const char* name, ::dsn::replication::decree& d, rocksdb::SequenceNumber& seq)
{
    return 2 == sscanf(name, "checkpoint.%" PRId64 ".%" PRIu64 "", &d, &seq)
        && std::string(name) == chkpt_get_dir_name(d, seq);
}

rrdb_service_impl::rrdb_service_impl(::dsn::replication::replica* replica) : rrdb_service(replica), _max_checkpoint_count(3)
{
    _primary_address = rpc_address(dsn_primary_address()).to_string();
    _is_open = false;
    _last_seq = 0;
    _is_catchup = false;
    _last_durable_seq = 0;
    _is_checkpointing = false;

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

rrdb_service_impl::checkpoint_info rrdb_service_impl::parse_checkpoints()
{
    std::vector<std::string> dirs;
    utils::filesystem::get_subdirectories(data_dir(), dirs, false);

    utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);

    _checkpoints.clear();
    for (auto& d : dirs)
    {
        checkpoint_info ci;
        std::string d1 = d.substr(data_dir().length() + 1);
        if (chkpt_init_from_dir(d1.c_str(), ci.d, ci.seq))
        {
            _checkpoints.push_back(ci);
        }
        else if (d1.find("checkpoint") != std::string::npos)
        {
            dwarn("%s: invalid checkpoint directory %s, remove ...", replica_name(), d.c_str());
            utils::filesystem::remove_path(d);
        }
    }

    std::sort(_checkpoints.begin(), _checkpoints.end());

    return _checkpoints.size() > 0 ? *_checkpoints.rbegin() : checkpoint_info();
}

void rrdb_service_impl::gc_checkpoints()
{
    while (true)
    {
        checkpoint_info del_d;

        {
            utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
            if (_checkpoints.size() <= _max_checkpoint_count)
                break;
            del_d = *_checkpoints.begin();
            _checkpoints.erase(_checkpoints.begin());
        }

        auto old_cpt = chkpt_get_dir_name(del_d.d, del_d.seq);
        auto old_cpt_dir = utils::filesystem::path_combine(data_dir(), old_cpt);
        if (utils::filesystem::directory_exists(old_cpt_dir))
        {
            if (utils::filesystem::remove_path(old_cpt_dir))
            {
                ddebug("%s: checkpoint %s removed by garbage collection", replica_name(), old_cpt_dir.c_str());
            }
            else
            {
                derror("%s: remove checkpoint %s failed by garbage collection", replica_name(), old_cpt_dir.c_str());
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
            dwarn("%s: checkpoint %s does not exist, garbage collection ignored", replica_name(), old_cpt_dir.c_str());
        }
    }
}

void rrdb_service_impl::write_batch()
{
    dassert(_batch.Count() > 0, "");
    dassert(get_current_batch_decree() == last_committed_decree() + 1,
            "batch_decree(%" PRId64 ") vs. commit_decree(%" PRId64 ")",
            get_current_batch_decree(), last_committed_decree() + 1);

    update_response resp;
    resp.app_id = get_app_id();
    resp.pidx = get_partition_index();
    resp.ballot = get_current_batch_ballot();
    resp.decree = get_current_batch_decree();
    resp.server = _primary_address;

    auto opts = _wt_opts;
    opts.given_sequence_number = _last_seq + 1;
    opts.given_decree = resp.decree;
    auto status = _db->Write(opts, &_batch);
    if (status.ok())
    {
        _last_seq += _batch.Count();
        check_last_seq();
    }
    else
    {
        derror("%s: %s failed, gpid=%d.%d, ballot=%" PRId64 ", decree=%" PRId64 ", seqno=%" PRIu64 ", status=%s",
               replica_name(), __FUNCTION__, resp.app_id, resp.pidx, resp.ballot, resp.decree,
               opts.given_sequence_number, status.ToString().c_str());
        set_physical_error(status.code());
    }

    resp.error = status.code();
    resp.seqno = opts.given_sequence_number;
    for (auto& r : _batch_repliers)
    {
        r(resp);
        resp.seqno++;
    }

    _batch_repliers.clear();
    _batch.Clear();
}

void rrdb_service_impl::check_last_seq()
{
    dassert(_last_seq == _db->GetLatestSequenceNumber(),
            "_last_seq mismatch DB::GetLatestSequenceNumber(): %" PRIu64 " vs %" PRIu64 "",
            _last_seq,
            _db->GetLatestSequenceNumber()
           );
}

void rrdb_service_impl::catchup_one()
{
    dbg_dassert(_last_durable_seq < _last_seq,
            "incorrect seq values in catch-up: %" PRIu64 " vs %" PRIu64 "",
            _last_durable_seq,
            _last_seq
            );
    if (++_last_durable_seq == _last_seq)
    {
        _is_catchup = false;
        ddebug("%s: catch up done, last_seq = %" PRIu64 "", replica_name(), _last_seq);
    }
}

void rrdb_service_impl::on_put(const update_request& update, ::dsn::replication::rpc_replication_app_replier<update_response>& reply)
{
    dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

    if (_is_catchup)
    {
        dassert(reply.is_empty(), "catchup only happens during initialization");
        catchup_one();
        return;
    }

    rocksdb::Slice skey(update.key.data(), update.key.length());
    rocksdb::Slice svalue(update.value.data(), update.value.length());
    _batch.Put(skey, svalue);
    _batch_repliers.push_back(reply);
    if (get_current_batch_state() != BS_BATCH)
    {
        write_batch();
    }
}

void rrdb_service_impl::on_remove(const ::dsn::blob& key, ::dsn::replication::rpc_replication_app_replier<update_response>& reply)
{
    dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

    if (_is_catchup)
    {
        dassert(reply.is_empty(), "catchup only happens during initialization");
        catchup_one();
        return;
    }

    rocksdb::Slice skey(key.data(), key.length());
    _batch.Delete(skey);
    _batch_repliers.push_back(reply);
    if (get_current_batch_state() != BS_BATCH)
    {
        write_batch();
    }
}

void rrdb_service_impl::on_get(const ::dsn::blob& key, ::dsn::replication::rpc_replication_app_replier<read_response>& reply)
{
    dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

    read_response resp;
    rocksdb::Slice skey(key.data(), key.length());
    std::string* value = new std::string();
    rocksdb::Status status = _db->Get(_rd_opts, skey, value);
    if (!status.ok() && !status.IsNotFound())
    {
        derror("%s: %s failed, status = %s", replica_name(), __FUNCTION__, status.ToString().c_str());
    }
    resp.error = status.code();
    resp.app_id = get_app_id();
    resp.pidx = get_partition_index();
    resp.ballot = get_ballot();
    resp.server = _primary_address;
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

::dsn::error_code rrdb_service_impl::open(bool create_new)
{
    dassert(!_is_open, "rrdb service %s is already opened", data_dir().c_str());
    ddebug("%s: start to open app %s, create_new = %s", replica_name(), data_dir().c_str(), create_new ? "true" : "false");

    rocksdb::Options opts = _db_opts;
    opts.create_if_missing = create_new;
    opts.error_if_exists = create_new;

    rrdb_service_impl::checkpoint_info last_checkpoint;
    if (create_new)
    {
        auto& dir = data_dir();
        if (!utils::filesystem::remove_path(dir))
        {
            derror("%s: remove old directory %s failed", replica_name(), dir.c_str());
            return ERR_FILE_OPERATION_FAILED;
        }
        dwarn("%s: cleared replica dir %s", replica_name(), dir.c_str());
        if (!utils::filesystem::create_directory(dir))
        {
            derror("%s: create new directory %s failed", replica_name(), dir.c_str());
            return ERR_FILE_OPERATION_FAILED;
        }
    }
    else
    {
        last_checkpoint = parse_checkpoints();
        gc_checkpoints();
    }

    auto path = utils::filesystem::path_combine(data_dir(), "rdb");
    auto status = rocksdb::DB::Open(opts, path, &_db);
    if (status.ok())
    {
        // load from checkpoints
        // set last_durable_decree/last_durable_seq/last_commit_decree to the last checkpoint
        _last_durable_decree = last_checkpoint.d;
        _last_durable_seq = last_checkpoint.seq;
        init_last_commit_decree(last_checkpoint.d);

        // however,
        // because rocksdb may persist the state on disk even without explicit flush(),
        // it is possible that the _last_seq is larger than _last_durable_seq
        // i.e., _last_durable_seq is not the ground-truth,
        // in this case, the db enters a catch-up mode, where the write ops are ignored
        // but only _last_durable_seq is increased until it equals to _last_seq.
        _last_seq = _db->GetLatestSequenceNumber();
        if (_last_seq > _last_durable_seq)
        {
            _is_catchup = true;
        }

        ddebug("%s: open app succeed, last_committed/durable_decree = <%" PRId64 ", %" PRId64 ">, "
                "last_seq = %" PRIu64 ", last_durable_seq = %" PRIu64 ", is_catch_up = %s",
                replica_name(), last_committed_decree(), last_durable_decree(),
                _last_seq, _last_durable_seq, _is_catchup ? "true" : "false"
              );

        _is_open = true;
        return ERR_OK;
    }
    else
    {
        derror("%s: open app failed, status = %s", replica_name(), status.ToString().c_str());
        return ERR_LOCAL_APP_FAILURE;
    }
}

::dsn::error_code rrdb_service_impl::close(bool clear_state)
{
    if (!_is_open)
    {
        dassert(_db == nullptr, "");
        dassert(!clear_state, "should not be here if do clear");
        return ERR_OK;
    }

    _is_open = false;
    delete _db;
    _db = nullptr;

    _batch.Clear();
    _batch_repliers.clear();

    _last_seq = 0;
    _is_catchup = false;
    _last_durable_seq = 0;
    _is_checkpointing = false;
    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        _checkpoints.clear();
    }

    reset_states();

    if (clear_state)
    {
        if (!utils::filesystem::remove_path(data_dir()))
        {
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    return ERR_OK;
}

::dsn::error_code rrdb_service_impl::checkpoint()
{
    if (!_is_open || _is_catchup|| _is_checkpointing)
        return ERR_WRONG_TIMING;

    if (_last_durable_decree == last_committed_decree())
        return ERR_NO_NEED_OPERATE;

    _is_checkpointing = true;

    rocksdb::Checkpoint* chkpt = nullptr;
    auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
    if (!status.ok())
    {
        derror("%s: create Checkpoint object failed, status = %s", replica_name(), status.ToString().c_str());
        _is_checkpointing = false;
        return ERR_LOCAL_APP_FAILURE;
    }

    checkpoint_info ci;
    ci.d = last_committed_decree();
    ci.seq = _last_seq;
    check_last_seq();

    auto dir = chkpt_get_dir_name(ci.d, ci.seq);
    auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
    if (utils::filesystem::directory_exists(chkpt_dir))
    {
        dwarn("%s: checkpoint directory %s already exist, remove it first", replica_name(), chkpt_dir.c_str());
        if (!utils::filesystem::remove_path(chkpt_dir))
        {
            derror("%s: remove old checkpoint directory %s failed", replica_name(), chkpt_dir.c_str());
            delete chkpt;
            chkpt = nullptr;
            _is_checkpointing = false;
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    status = chkpt->CreateCheckpoint(chkpt_dir);
    if (!status.ok())
    {
        // sometimes may fail, try again
        dwarn("%s: create checkpoint failed, status = %s, try again", replica_name(), status.ToString().c_str());
        status = chkpt->CreateCheckpoint(chkpt_dir);
    }

    // destroy Checkpoint object
    delete chkpt;
    chkpt = nullptr;

    if (!status.ok())
    {
        derror("%s: create checkpoint failed, status = %s", replica_name(), status.ToString().c_str());
        utils::filesystem::remove_path(chkpt_dir);
        _is_checkpointing = false;
        return ERR_LOCAL_APP_FAILURE;
    }

    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        _checkpoints.push_back(ci);
        std::sort(_checkpoints.begin(), _checkpoints.end());
        _last_durable_decree = _checkpoints.back().d;
    }

    ddebug("%s: create checkpoint %s succeed", replica_name(), chkpt_dir.c_str());

    gc_checkpoints();

    _is_checkpointing = false;
    return ERR_OK;
}

// Must be thread safe.
::dsn::error_code rrdb_service_impl::checkpoint_async()
{
    if (!_is_open || _is_catchup || _is_checkpointing)
        return ERR_WRONG_TIMING;

    if (_last_durable_decree == last_committed_decree())
        return ERR_NO_NEED_OPERATE;

    _is_checkpointing = true;

    rocksdb::Checkpoint* chkpt = nullptr;
    auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
    if (!status.ok())
    {
        derror("%s: create Checkpoint object failed, status = %s", replica_name(), status.ToString().c_str());
        _is_checkpointing = false;
        return ERR_LOCAL_APP_FAILURE;
    }

    rocksdb::SequenceNumber sequence = 0;
    uint64_t decree = 0;
    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        if (!_checkpoints.empty())
        {
            auto& ci = _checkpoints.back();
            sequence = ci.seq;
            decree = ci.d;
        }
    }

    char buf[256];
    sprintf(buf, "checkpoint.tmp.%" PRIu64 "", dsn_now_us());
    std::string tmp_dir = utils::filesystem::path_combine(data_dir(), buf);
    if (utils::filesystem::directory_exists(tmp_dir))
    {
        dwarn("%s: tmp directory %s already exist, remove it first", replica_name(), tmp_dir.c_str());
        if (!utils::filesystem::remove_path(tmp_dir))
        {
            derror("%s: remove old tmp directory %s failed", replica_name(), tmp_dir.c_str());
            _is_checkpointing = false;
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    status = chkpt->CreateCheckpointQuick(tmp_dir, &sequence, &decree);
    delete chkpt;
    chkpt = nullptr;
    if (!status.ok())
    {
        dwarn("%s: async create checkpoint failed, status = %s", replica_name(), status.ToString().c_str());
        utils::filesystem::remove_path(tmp_dir);
        _is_checkpointing = false;
        return status.IsTryAgain() ? ERR_TRY_AGAIN : ERR_LOCAL_APP_FAILURE;
    }

    checkpoint_info ci;
    ci.seq = sequence;
    ci.d = decree;
    auto dir = chkpt_get_dir_name(ci.d, ci.seq);
    auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
    if (utils::filesystem::directory_exists(chkpt_dir))
    {
        dwarn("%s: checkpoint directory %s already exist, remove it first", replica_name(), chkpt_dir.c_str());
        if (!utils::filesystem::remove_path(chkpt_dir))
        {
            derror("%s: remove old checkpoint directory %s failed", replica_name(), chkpt_dir.c_str());
            utils::filesystem::remove_path(tmp_dir);
            _is_checkpointing = false;
            return ERR_FILE_OPERATION_FAILED;
        }
    }

    if (!utils::filesystem::rename_path(tmp_dir, chkpt_dir))
    {
        derror("%s: rename checkpoint directory from %s to %s failed", replica_name(), tmp_dir.c_str(), chkpt_dir.c_str());
        utils::filesystem::remove_path(tmp_dir);
        _is_checkpointing = false;
        return ERR_FILE_OPERATION_FAILED;
    }

    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        _checkpoints.push_back(ci);
        std::sort(_checkpoints.begin(), _checkpoints.end());
        _last_durable_decree = _checkpoints.back().d;
    }

    ddebug("%s: async create checkpoint %s succeed", replica_name(), chkpt_dir.c_str());

    gc_checkpoints();

    _is_checkpointing = false;
    return ERR_OK;
}

::dsn::error_code rrdb_service_impl::get_checkpoint(
        ::dsn::replication::decree start, 
        const blob& learn_req, 
        /*out*/ ::dsn::replication::learn_state& state)
{
    dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

    checkpoint_info ci;
    {
        utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
        if (_checkpoints.size() > 0)
            ci = *_checkpoints.rbegin();
    }

    if (ci.d == 0)
    {
        derror("%s: no checkpoint found", replica_name());
        return ERR_OBJECT_NOT_FOUND;
    }

    auto dir = chkpt_get_dir_name(ci.d, ci.seq);
    auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
    auto succ = utils::filesystem::get_subfiles(chkpt_dir, state.files, true);
    if (!succ)
    {
        derror("%s: list files in checkpoint dir %s failed", replica_name(), chkpt_dir.c_str());
        return ERR_FILE_OPERATION_FAILED;
    }

    // attach checkpoint info
    binary_writer writer;
    writer.write_pod(ci);
    state.meta.push_back(writer.get_buffer());

    state.from_decree_excluded = 0;
    state.to_decree_included = ci.d;

    ddebug("%s: get checkpoint succeed, from_decree_excluded = 0, to_decree_included = %" PRId64, replica_name(), state.to_decree_included);
    return ERR_OK;
}

::dsn::error_code rrdb_service_impl::apply_checkpoint(
        ::dsn::replication::learn_state& state,
        ::dsn::replication::chkpt_apply_mode mode)
{
    ::dsn::error_code err;
    checkpoint_info ci;

    dassert(state.meta.size() >= 1, "the learned state does not have checkpint meta info");
    binary_reader reader(state.meta[0]);
    reader.read_pod(ci);

    if (mode == dsn::replication::CHKPT_COPY)
    {
        dassert(state.to_decree_included > last_durable_decree()
                && ci.d == state.to_decree_included,
                "");

        auto dir = chkpt_get_dir_name(ci.d, ci.seq);
        auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
        auto old_dir = utils::filesystem::remove_file_name(state.files[0]);
        auto succ = utils::filesystem::rename_path(old_dir, chkpt_dir);

        if (succ)
        {
            utils::auto_lock<utils::ex_lock_nr> l(_checkpoints_lock);
            _checkpoints.push_back(ci);
            _last_durable_decree = ci.d;
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
        err = close(true);
        if (err != ERR_OK)
        {
            derror("%s: close rocksdb %s failed, err = %s", replica_name(), data_dir().c_str(), err.to_string());
            return err;
        }
    }

    // clear data dir
    if (!utils::filesystem::remove_path(data_dir()))
    {
        derror("%s: clear data directory %s failed", replica_name(), data_dir().c_str());
        return ERR_FILE_OPERATION_FAILED;
    }

    // reopen the db with the new checkpoint files
    if (state.files.size() > 0)
    {
        // create data dir
        if (!utils::filesystem::create_directory(data_dir()))
        {
            derror("%s: create data directory %s failed", replica_name(), data_dir().c_str());
            return ERR_FILE_OPERATION_FAILED;
        }

        // move learned files from learn_dir to data_dir/rdb
        std::string learn_dir = utils::filesystem::remove_file_name(*state.files.rbegin());
        std::string new_dir = utils::filesystem::path_combine(data_dir(), "rdb");
        if (!utils::filesystem::rename_path(learn_dir, new_dir))
        {
            derror("%s: rename %s to %s failed", replica_name(), learn_dir.c_str(), new_dir.c_str());
            return ERR_FILE_OPERATION_FAILED;
        }

        err = open(false);
    }
    else
    {
        dwarn("%s: apply empty checkpoint, create new rocksdb", replica_name());
        err = open(true);
    }

    if (err != ERR_OK)
    {
        derror("%s: open rocksdb %s failed, err = %s", replica_name(), data_dir().c_str(), err.to_string());
        return err;
    }

    // because there is no checkpoint in the new db
    dassert(_is_open, "");
    dassert(last_committed_decree() == 0, "");
    dassert(last_durable_decree() == 0, "");
    dassert(ci.seq == _last_seq,
            "seq numbers from loaded data and attached meta info do not match: %" PRId64 " vs %" PRId64 "",
            ci.seq,
            _last_seq
           );

    // set last_commit_decree to last checkpoint
    init_last_commit_decree(ci.d);
    _is_catchup = false;

    // checkpoint immediately if need
    if (last_committed_decree() != last_durable_decree())
    {
        err = checkpoint();
        if (err != ERR_OK)
        {
            derror("%s: checkpoint failed, err = %s", data_dir().c_str(), err.to_string());
            return err;
        }
        dassert(last_committed_decree() == last_durable_decree(),
                "commit and durable decree mismatch after checkpoint: %" PRId64 " vs %" PRId64,
                last_committed_decree(),
                last_durable_decree()
               );
    }

    ddebug("%s: apply checkpoint succeed, last_committed_decree = %" PRId64, replica_name(), last_committed_decree());
    return ERR_OK;
}
}
}
