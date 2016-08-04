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
#include <vector>
#include <sys/time.h>
#include <unistd.h>

#include <dsn/service_api_c.h>

#include <rrdb_client.h>
using namespace ::dsn::apps;

irrdb_client* client = nullptr;
int total_threads;
//should be enough for threads, coz atomics are not allowed to copy or move, we init the vector statically.
std::vector< std::atomic_llong > set_nexts(1000);

const char* set_next_key = "set_next";
const char* hash_key_prefix = "kill_test_hash_key_";
const char* sort_key_prefix = "kill_test_sort_key_";
const char* value_prefix = "kill_test_value_";

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "kill.test.main"

// return time in us.
long get_time()
{    
    struct timeval tv;    
    gettimeofday(&tv,NULL);    
    return tv.tv_sec * 1000000 + tv.tv_usec;    
}    

void do_set(int thread_index)
{
    char buf[1024];
    long last_time = get_time();
    int try_count = 0;
    long long id = set_nexts[thread_index].load(std::memory_order_relaxed);
    std::string hash_key;
    std::string sort_key;
    std::string value;
    while (true) {
        if (try_count == 0) {
            sprintf(buf, "%s%lld", hash_key_prefix, id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, id);
            value.assign(buf);
        }
        irrdb_client::internal_info info;
        int ret = client->set(hash_key, sort_key, value, 5000, 0, &info);
        if (ret == RRDB_ERR_OK) {
            long cur_time = get_time();
            ddebug("kill_test: SetThread@%d: set succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, decree=%lld, server=%s)",
                    thread_index, id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.decree, info.server.c_str());
            last_time = cur_time;
            try_count = 0;
            id += total_threads;
            set_nexts[thread_index].store(id, std::memory_order_release);
        }
        else {
            derror("kill_test: SetThread@%d: set failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, decree=%lld, server=%s)",
                    thread_index, id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.decree, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
}


long long thread_set_next(int thread_index)
{
    return set_nexts[thread_index].load(std::memory_order_acquire)/total_threads;
}

// for each round:
// - first randomly get from range [0, thread_set_next - ROUND_COUNT) for RANDOM_COUNT times
// - then loop from range [thread_set_next - ROUND_COUNT, thread_set_next)
#define ROUND_COUNT 100000
#define RANDOM_COUNT 100000
void do_get(int thread_index)
{
    char buf[1024];
    long last_time = get_time();
    int try_count = 0;
    int random_count = RANDOM_COUNT;
    long long random_end = 0;
    long long next = 0;
    long long id = LLONG_MAX;
    std::string hash_key;
    std::string sort_key;
    std::string value;

    while (true) {
        if (try_count == 0) {
            if (random_count < RANDOM_COUNT) {
                random_count++;
                if (random_count < RANDOM_COUNT) {
                    id = std::rand() % random_end;
                }
                else {
                    id = random_end;
                }
            }
            else if (id < next-1) {
                id++;
            }
            else {
                next = thread_set_next(thread_index);
                if (id < next-1) {
                    id++;
                }
                else {
                    if (next > 0 && id != LLONG_MAX) {
                        ddebug("kill_test: GetThread@%d: finish get round: set_next=%lld", thread_index, next);
                    }

                    sleep(1);

                    if (next == 0) {
                        continue;
                    }

                    next = thread_set_next(thread_index);
                    if (next >= ROUND_COUNT + RANDOM_COUNT * 2) {
                        random_count = 0;
                        random_end = next - ROUND_COUNT;
                        id = 0;
                    }
                    else {
                        random_count = RANDOM_COUNT;
                        random_end = 0;
                        id = 0;
                    }
                    if (next > 0) {
                        ddebug("kill_test: GetThread@%d: start get round: set_next=%lld, random_end=%lld", thread_index, next, random_end);
                    }
                }
            }

            long long real_id = id*total_threads + thread_index;
            sprintf(buf, "%s%lld", hash_key_prefix, real_id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, real_id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, real_id);
            value.assign(buf);
        }
        std::string get_value;
        irrdb_client::internal_info info;
        int ret = client->get(hash_key, sort_key, get_value, 5000, &info);
        if (ret == RRDB_ERR_OK || ret == RRDB_ERR_NOT_FOUND) {
            long cur_time = get_time();
            if (ret == RRDB_ERR_NOT_FOUND) {
                dfatal("kill_test: GetThread%d: get not found: id=%lld, try=%d, time=%ld (gpid=%d.%d, server=%s)",
                    thread_index, id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.server.c_str());
                exit(-1);
            }
            else if (value == get_value) {
                dinfo("kill_test: GetThread%d: get succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, server=%s)",
                    thread_index, id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.server.c_str());
            }
            else {
                dfatal("kill_test: GetThread%d: get mismatched: id=%lld, try=%d, time=%ld, expect_value=%s, real_value=%s (gpid=%d.%d, server=%s)",
                    thread_index, id, try_count, (cur_time - last_time), value.c_str(), get_value.c_str(), info.app_id, info.partition_index, info.server.c_str());
                exit(-1);
            }
            last_time = cur_time;
            try_count = 0;
        }
        else {
            derror("kill_test: GetThread%d: get failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, server=%s)",
                thread_index, id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
}

long long get_min_set_next()
{
    long long result = set_nexts[0].load(std::memory_order_acquire);
    for (unsigned int i=0; i<total_threads; ++i) {
        long long next_value = set_nexts[i].load(std::memory_order_acquire);
        if (result > next_value)
            result = next_value;
    }
    return result;
}

void do_mark()
{
    char buf[1024];
    long last_time = get_time();
    long long id = 0;
    std::string value;
    while (true) {
        sleep(1);
        long long next = get_min_set_next();
        if (id == next) {
            continue;
        }
        id = next;
        sprintf(buf, "%lld", id);
        value.assign(buf);
        int ret = client->set(set_next_key, "", value);
        if (ret == RRDB_ERR_OK) {
            long cur_time = get_time();
            ddebug("kill_test: MarkThread: update set_next succeed: set_next=%lld, time=%ld", id, (cur_time - last_time));
            last_time = cur_time;
        }
        else {
            derror("kill_test: MarkThread: update set_next failed: set_next=%lld, ret=%d, error=%s", id, ret, client->get_error_string(ret));
        }
    }
}

void initialize_set_next(long long min_db_set)
{
    int start = min_db_set - min_db_set%total_threads;
    for (int i=0; i<total_threads; ++i)
        set_nexts[i].store(start+i);
}

int main(int argc, const char* argv[])
{
    if (argc != 4) {
        derror("USAGE: %s <config-file> <app-name> <rw-thread-count>", argv[0]);
        return -1;
    }
    total_threads = atoi(argv[3]);
    if (total_threads <= 0 ) {
        derror("USAGE: %s <config-file> <app-name> <rw-thread-count>", argv[0]);
        return -1;
    }
    const char* config_file = argv[1];
    if (!rrdb_client_factory::initialize(config_file)) {
        derror("kill_test: MainThread: init pegasus failed");
        return -1;
    }

    const char* app_name = argv[2];
    client = rrdb_client_factory::get_client("mycluster", app_name);
    ddebug("kill_test: MainThread: app_name=%s", app_name);

    while (true)
    {
        std::string set_next_value;
        int ret = client->get(set_next_key, "", set_next_value);
        if (ret == RRDB_ERR_OK) {
            long long i = atoll(set_next_value.c_str());
            if (i == 0 && !set_next_value.empty()) {
                derror("kill_test: MainThread: read \"%s\" failed: value_str=%s", set_next_key, set_next_value.c_str());
                return -1;
            }
            ddebug("kill_test: MainThread: read \"%s\" succeed: value=%lld", set_next_key, i);
            initialize_set_next(i);
            break;
        }
        else if (ret == RRDB_ERR_NOT_FOUND) {
            ddebug("kill_test: MainThread: read \"%s\" not found, init set_next to 0", set_next_key);
            initialize_set_next(0);
            break;
        }
        else {
            derror("kill_test: MainThread: read \"%s\" failed: error=%s", set_next_key, client->get_error_string(ret));
        }
    }

    std::srand(std::time(0));

    ddebug("kill_test: MainThread: start %d worker threads for both read/write", total_threads);
    std::vector<std::thread> worker_threads;

    for (int i=0; i<total_threads; ++i) {
        worker_threads.push_back( std::thread(do_set, i) );
        worker_threads.push_back( std::thread(do_get, i) );
    }
    worker_threads.push_back( std::thread(do_mark) );

    for (unsigned int i=0; i<worker_threads.size(); ++i)
        worker_threads[i].join();
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
