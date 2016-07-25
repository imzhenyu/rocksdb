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

#include <dsn/service_api_c.h>

#include <rrdb_client.h>
using namespace ::dsn::apps;

irrdb_client* client = nullptr;
std::atomic_llong set_next(0);

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

void do_set()
{
    char buf[1024];
    long last_time = get_time();
    int try_count = 0;
    long long id = 0;
    std::string hash_key;
    std::string sort_key;
    std::string value;
    while (true) {
        if (try_count == 0) {
            id = set_next.load();
            sprintf(buf, "%s%lld", hash_key_prefix, id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, id);
            value.assign(buf);
        }
        irrdb_client::internal_info info;
        int ret = client->set(hash_key, sort_key, value, 5000, &info);
        if (ret == RRDB_ERR_OK) {
            long cur_time = get_time();
            ddebug("kill_test: SetThread: set succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, decree=%lld, server=%s)",
                    id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.decree, info.server.c_str());
            last_time = cur_time;
            try_count = 0;
            set_next++;
        }
        else {
            derror("kill_test: SetThread: set failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, decree=%lld, server=%s)",
                    id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.decree, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
}

/*
// for each round:
// - loop from range [0, set_next)
void do_get()
{
    char buf[1024];
    long last_time = get_time();
    int try_count = 0;
    long long id = 0;
    std::string hash_key;
    std::string sort_key;
    std::string value;
    while (true) {
        if (try_count == 0) {
            long long next = set_next.load();
            if (id >= next) {
                if (next > 0) {
                    ddebug("kill_test: GetThread: finish get round: set_next=%lld", next);
                }
                id = 0;
                sleep(1);
                continue;
            }
            if (id == 0) {
                ddebug("kill_test: GetThread: start get round");
            }
            sprintf(buf, "%s%lld", hash_key_prefix, id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, id);
            value.assign(buf);
        }
        irrdb_client::internal_info info;
        std::string get_value;
        int ret = client->get(hash_key, sort_key, get_value, 5000, &info);
        if (ret == RRDB_ERR_OK || ret == RRDB_ERR_NOT_FOUND) {
            long cur_time = get_time();
            if (ret == RRDB_ERR_NOT_FOUND) {
                derror("kill_test: GetThread: get not found: id=%lld, try=%d, time=%ld (gpid=%d.%d, ballot=%lld, server=%s)",
                    id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.ballot, info.server.c_str());
            }
            else if (value == get_value) {
                dinfo("kill_test: GetThread: get succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, ballot=%lld, server=%s)",
                    id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.ballot, info.server.c_str());
            }
            else {
                derror("kill_test: GetThread: get mismatched: id=%lld, try=%d, time=%ld, expect_value=%s, real_value=%s (gpid=%d.%d, ballot=%lld, server=%s)",
                    id, try_count, (cur_time - last_time), value.c_str(), get_value.c_str(), info.app_id, info.partition_index, info.ballot, info.server.c_str());
            }
            last_time = cur_time;
            try_count = 0;
            id++;
        }
        else {
            derror("kill_test: GetThread: get failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, ballot=%lld, server=%s)",
                id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.ballot, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
}
*/

// for each round:
// - first randomly get from range [0, set_next - ROUND_COUNT) for RANDOM_COUNT times
// - then loop from range [set_next - ROUND_COUNT, set_next)
#define ROUND_COUNT 100000
#define RANDOM_COUNT 100000
void do_get()
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
    std::srand(std::time(0));
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
            else if (id < next - 1) {
                id++;
            }
            else {
                next = set_next.load();
                if (id < next - 1) {
                    id++;
                }
                else {
                    if (next > 0 && id != LLONG_MAX) {
                        ddebug("kill_test: GetThread: finish get round: set_next=%lld", next);
                    }

                    sleep(1);

                    if (next == 0) {
                        continue;
                    }

                    next = set_next.load();
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
                        ddebug("kill_test: GetThread: start get round: set_next=%lld, random_end=%lld", next, random_end);
                    }
                }
            }
            sprintf(buf, "%s%lld", hash_key_prefix, id);
            hash_key.assign(buf);
            sprintf(buf, "%s%lld", sort_key_prefix, id);
            sort_key.assign(buf);
            sprintf(buf, "%s%lld", value_prefix, id);
            value.assign(buf);
        }
        std::string get_value;
        irrdb_client::internal_info info;
        int ret = client->get(hash_key, sort_key, get_value, 5000, &info);
        if (ret == RRDB_ERR_OK || ret == RRDB_ERR_NOT_FOUND) {
            long cur_time = get_time();
            if (ret == RRDB_ERR_NOT_FOUND) {
                dfatal("kill_test: GetThread: get not found: id=%lld, try=%d, time=%ld (gpid=%d.%d, server=%s)",
                    id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.server.c_str());
                exit(-1);
            }
            else if (value == get_value) {
                dinfo("kill_test: GetThread: get succeed: id=%lld, try=%d, time=%ld (gpid=%d.%d, server=%s)",
                    id, try_count, (cur_time - last_time), info.app_id, info.partition_index, info.server.c_str());
            }
            else {
                dfatal("kill_test: GetThread: get mismatched: id=%lld, try=%d, time=%ld, expect_value=%s, real_value=%s (gpid=%d.%d, server=%s)",
                    id, try_count, (cur_time - last_time), value.c_str(), get_value.c_str(), info.app_id, info.partition_index, info.server.c_str());
                exit(-1);
            }
            last_time = cur_time;
            try_count = 0;
        }
        else {
            derror("kill_test: GetThread: get failed: id=%lld, try=%d, ret=%d, error=%s (gpid=%d.%d, server=%s)",
                id, try_count, ret, client->get_error_string(ret), info.app_id, info.partition_index, info.server.c_str());
            try_count++;
            if (try_count > 3) {
                sleep(1);
            }
        }
    }
}

void do_mark()
{
    char buf[1024];
    long last_time = get_time();
    long long id = 0;
    std::string value;
    while (true) {
        sleep(1);
        long long next = set_next.load();
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

int main(int argc, const char* argv[])
{
    if (argc != 3) {
        derror("USAGE: %s <config-file> <app-name>", argv[0]);
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
            set_next.store(i);
            break;
        }
        else if (ret == RRDB_ERR_NOT_FOUND) {
            ddebug("kill_test: MainThread: read \"%s\" not found, init set_next to 0", set_next_key);
            set_next.store(0);
            break;
        }
        else {
            derror("kill_test: MainThread: read \"%s\" failed: error=%s", set_next_key, client->get_error_string(ret));
        }
    }

    std::thread set_thread(do_set);
    std::thread get_thread(do_get);
    std::thread mark_thread(do_mark);

    set_thread.join();
    get_thread.join();
    mark_thread.join();

    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
