// Copyright (c) 2016 xiaomi.com, Inc. All Rights Reserved.

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <ctime>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include <dsn/service_api_c.h>

#include <pegasus/client.h>

using namespace pegasus;

pegasus_client* client = nullptr;
std::atomic_llong set_next(0);
int set_thread_count;
int get_thread_count;
std::vector<long long> set_thread_setting_id;

const char* set_next_key = "set_next";
const char* check_max_key = "check_max";
const char* hash_key_prefix = "kill_test_hash_key_";
const char* sort_key_prefix = "kill_test_sort_key_";
const char* value_prefix = "kill_test_value_";

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "kill.test"

// return time in us.
long get_time()
{    
    struct timeval tv;    
    gettimeofday(&tv,NULL);    
    return tv.tv_sec * 1000000 + tv.tv_usec;    
}

long long get_min_thread_setting_id()
{
    long long id = set_thread_setting_id[0];
    for (int i = 1; i < set_thread_count; ++i)
    {
        if (set_thread_setting_id[i] < id)
            id = set_thread_setting_id[i];
    }
    return id;
}

void do_set(int thread_id)
{
    char buf[1024];
    std::string hash_key;
    std::string sort_key;
    std::string value;
    long long id = 0;
    int try_count = 0;
    long last_time = get_time();
    while (true) {
        if (try_count == 0) {
            id = set_next++;
            set_thread_setting_id[thread_id] = id;
            sprintf(buf, "%s%lld", hash_key_prefix, id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, id);
            value.assign(buf);
        }
        pegasus::pegasus_client::internal_info info;
        int ret = client->set(hash_key, sort_key, value, 5000, &info);
        if (ret == PERR_OK) {
            long cur_time = get_time();
            ddebug("SetThread[%d]: set succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, decree=%lld, server=%s)",
                    thread_id, id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.decree, info.server.c_str());
            last_time = cur_time;
            try_count = 0;
        }
        else {
            derror("SetThread[%d]: set failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, decree=%lld, server=%s)",
                    thread_id, id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.decree, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
}

// for each round:
// - loop from range [start_id, end_id]
void do_get_range(int thread_id, int round_id, long long start_id, long long end_id)
{
    ddebug("GetThread[%d]: round(%d): start get range [%u,%u]", thread_id, round_id, start_id, end_id);
    char buf[1024];
    std::string hash_key;
    std::string sort_key;
    std::string value;
    long long id = start_id;
    int try_count = 0;
    long last_time = get_time();
    while (id <= end_id) {
        if (try_count == 0) {
            sprintf(buf, "%s%lld", hash_key_prefix, id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, id);
            value.assign(buf);
        }
        pegasus::pegasus_client::internal_info info;
        std::string get_value;
        int ret = client->get(hash_key, sort_key, get_value, 5000, &info);
        if (ret == PERR_OK || ret == PERR_NOT_FOUND) {
            long cur_time = get_time();
            if (ret == PERR_NOT_FOUND) {
                dfatal("GetThread[%d]: round(%d): get not found: id=%lld, try=%d, time=%ld (gpid=%d.%d, server=%s)",
                    thread_id, round_id, id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.server.c_str());
                exit(-1);
            }
            else if (value != get_value) {
                dfatal("GetThread[%d]: round(%d): get mismatched: id=%lld, try=%d, time=%ld, expect_value=%s, real_value=%s (gpid=%d.%d, server=%s)",
                    thread_id, round_id, id, try_count, (cur_time - last_time), value.c_str(), get_value.c_str(), info.app_id, info.partition_index, info.server.c_str());
                exit(-1);
            }
            else {
                dinfo("GetThread[%d]: round(%d): get succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, server=%s)",
                    thread_id, round_id, id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.server.c_str());
            }
            last_time = cur_time;
            try_count = 0;
            id++;
        }
        else {
            derror("GetThread[%d]: round(%d): get failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, server=%s)",
                thread_id, round_id, id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
    ddebug("GetThread[%d]: round(%d): finish get range [%u,%u]", thread_id, round_id, start_id, end_id);
}

void do_check(int thread_count)
{
    int round_id = 1;
    while (true)
    {
        long long range_end = get_min_thread_setting_id() - 1;
        if (range_end < thread_count)
        {
            sleep(1);
            continue;
        }
        ddebug("CheckThread: round(%d): start check round, range_end=%lld", round_id, range_end);
        long start_time = get_time();
        std::vector<std::thread> worker_threads;
        long long piece_count = range_end / thread_count;
        for (int i = 0; i < thread_count; ++i)
        {
            long long start_id = piece_count * i;
            long long end_id = (i == thread_count - 1) ? range_end : (piece_count * (i+1) - 1);
            worker_threads.emplace_back(do_get_range, i, round_id, start_id, end_id);
        }
        for (auto& t : worker_threads)
        {
            t.join();
        }
        long finish_time = get_time();
        ddebug("CheckThread: round(%d): finish check round, range_end=%lld, total_time=%ld seconds", round_id, range_end, (finish_time - start_time) / 1000000);

        // update check_max
        while (true)
        {
            char buf[1024];
            sprintf(buf, "%lld", range_end);
            int ret = client->set(check_max_key, "", buf);
            if (ret == PERR_OK) {
                ddebug("CheckThread: round(%d): update \"%s\" succeed: check_max=%lld", round_id, check_max_key, range_end);
                break;
            }
            else {
                derror("CheckThread: round(%d): update \"%s\" failed: check_max=%lld, ret=%d, error=%s", round_id, check_max_key, range_end, ret, client->get_error_string(ret));
            }
        }

        round_id++;
    }
}

void do_mark()
{
    char buf[1024];
    long last_time = get_time();
    long long old_id = 0;
    std::string value;
    while (true) {
        sleep(1);
        long long new_id = get_min_thread_setting_id();
        dassert(new_id >= old_id, "");
        if (new_id == old_id) {
            continue;
        }
        sprintf(buf, "%lld", new_id);
        value.assign(buf);
        int ret = client->set(set_next_key, "", value);
        if (ret == PERR_OK) {
            long cur_time = get_time();
            ddebug("MarkThread: update \"%s\" succeed: set_next=%lld, time=%ld", set_next_key, new_id, (cur_time - last_time));
            old_id = new_id;
        }
        else {
            derror("MarkThread: update \"%s\" failed: set_next=%lld, ret=%d, error=%s", set_next_key, new_id, ret, client->get_error_string(ret));
        }
    }
}

int main(int argc, const char* argv[])
{
    if (argc != 4) {
        derror("USAGE: %s <config-file> <app-name> <thread-count>", argv[0]);
        return -1;
    }

    const char* config_file = argv[1];
    if (!pegasus_client_factory::initialize(config_file)) {
        derror("MainThread: init pegasus failed");
        return -1;
    }

    const char* app_name = argv[2];
    client = pegasus_client_factory::get_client("mycluster", app_name);
    ddebug("MainThread: app_name=%s", app_name);

    set_thread_count = atoi(argv[3]);
    if (set_thread_count <= 0) {
        derror("MainThread: invalid thread count: %s", argv[3]);
        return -1;
    }
    ddebug("MainThread: set_thread_count=%d", set_thread_count);

    get_thread_count = set_thread_count * 4;
    ddebug("MainThread: get_thread_count=%d", get_thread_count);

    while (true)
    {
        std::string set_next_value;
        int ret = client->get(set_next_key, "", set_next_value);
        if (ret == PERR_OK) {
            long long i = atoll(set_next_value.c_str());
            if (i == 0 && !set_next_value.empty()) {
                derror("MainThread: read \"%s\" failed: value_str=%s", set_next_key, set_next_value.c_str());
                return -1;
            }
            ddebug("MainThread: read \"%s\" succeed: value=%lld", set_next_key, i);
            set_next.store(i);
            break;
        }
        else if (ret == PERR_NOT_FOUND) {
            ddebug("MainThread: read \"%s\" not found, init set_next to 0", set_next_key);
            set_next.store(0);
            break;
        }
        else {
            derror("MainThread: read \"%s\" failed: error=%s", set_next_key, client->get_error_string(ret));
        }
    }

    set_thread_setting_id.resize(set_thread_count);

    std::vector<std::thread> set_threads;
    for (int i = 0; i < set_thread_count; ++i) {
        set_threads.emplace_back(do_set, i);
    }

    std::thread mark_thread(do_mark);

    do_check(get_thread_count);

    mark_thread.join();

    for (auto& t : set_threads) {
        t.join();
    }

    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
