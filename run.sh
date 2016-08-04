#!/bin/bash

ROOT=`pwd`

if [ -z "$DSN_ROOT" ]; then
    echo "ERROR: DSN_ROOT not defined"
    exit -1
fi

if [ ! -d "$DSN_ROOT" ]; then
    echo "ERROR: DSN_ROOT directory not exist: ${DSN_ROOT}"
    exit -1
fi

function usage()
{
    echo "usage: run.sh <command> [<args>]"
    echo
    echo "Command list:"
    echo "   help                      print the help info"
    echo "   build                     build the system"
    echo
    echo "   start_zk                  start local single zookeeper server"
    echo "   stop_zk                   stop local zookeeper server"
    echo "   clear_zk                  stop local zookeeper server and clear data"
    echo
    echo "   start_onebox              start rrdb onebox"
    echo "   stop_onebox               stop rrdb onebox"
    echo "   list_onebox               list rrdb onebox"
    echo "   clear_onebox              clear rrdb onebox"
    echo
    echo "   start_onebox_instance     start rrdb onebox instance"
    echo "   stop_onebox_instance      stop rrdb onebox instance"
    echo "   restart_onebox_instance   restart rrdb onebox instance"
    echo
    echo "   start_kill_test           statr rrdb kill test"
    echo "   stop_kill_test            stop rrdb kill test"
    echo "   clear_kill_test           clear rrdb kill test"
    echo
    echo "   bench                     benchmark test"
    echo "   shell                     run rrdb_cluster shell"
    echo
    echo "Command 'run.sh <command> -h' will print help for subcommands."
}

#####################
## build
#####################
function usage_build()
{
    echo "Options for subcommand 'build':"
    echo "   -h|--help         print the help info"
    echo "   -t|--type         build type: debug|release, default is debug"
    echo "   -s|--serialize    serialize type: dsn|thrift|proto, default is thrift"
    echo "   -c|--clear        clear the environment before building"
    echo "   -cc|--half-clear  only clear the environment of replication before building"
    echo "   -b|--boost_dir <dir>"
    echo "                     specify customized boost directory,"
    echo "                     if not set, then use the system boost"
    echo "   -w|--warning_all  open all warnings when build, default no"
    echo "   -g|--enable_gcov  generate gcov code coverage report, default no"
    echo "   -v|--verbose      build in verbose mode, default no"
}
function run_build()
{
    BUILD_TYPE="debug"
    SERIALIZE_TYPE="thrift"
    CLEAR=NO
    PART_CLEAR=NO
    BOOST_DIR=""
    WARNING_ALL=NO
    ENABLE_GCOV=NO
    RUN_VERBOSE=NO
    TEST_MODULE=""
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_build
                exit 0
                ;;
            -t|--type)
                BUILD_TYPE="$2"
                shift
                ;;
            -s|--serialize)
                SERIALIZE_TYPE="$2"
                shift
                ;;
            -c|--clear)
                CLEAR=YES
                ;;
            -cc|--part_clear)
                PART_CLEAR=YES
                ;;
            -b|--boost_dir)
                BOOST_DIR="$2"
                shift
                ;;
            -w|--warning_all)
                WARNING_ALL=YES
                ;;
            -g|--enable_gcov)
                ENABLE_GCOV=YES
                ;;
            -v|--verbose)
                RUN_VERBOSE=YES
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_build
                exit -1
                ;;
        esac
        shift
    done
    if [ "$BUILD_TYPE" != "debug" -a "$BUILD_TYPE" != "release" ]; then
        echo "ERROR: invalid build type \"$BUILD_TYPE\""
        echo
        usage_build
        exit -1
    fi
    if [ "$SERIALIZE_TYPE" != "dsn" -a "$SERIALIZE_TYPE" != "thrift" -a "$SERIALIZE_TYPE" != "protobuf" ]; then
        echo "ERROR: invalid serialize type \"$SERIALIZE_TYPE\""
        echo
        usage_build
        exit -1
    fi
    cd replication; BUILD_TYPE="$BUILD_TYPE" SERIALIZE_TYPE="$SERIALIZE_TYPE" CLEAR="$CLEAR" PART_CLEAR="$PART_CLEAR" \
        BOOST_DIR="$BOOST_DIR" WARNING_ALL="$WARNING_ALL" ENABLE_GCOV="$ENABLE_GCOV" \
        RUN_VERBOSE="$RUN_VERBOSE" TEST_MODULE="$TEST_MODULE" ./build.sh
}

#####################
## start_zk
#####################
function usage_start_zk()
{
    echo "Options for subcommand 'start_zk':"
    echo "   -h|--help         print the help info"
    echo "   -d|--install_dir <dir>"
    echo "                     zookeeper install directory,"
    echo "                     if not set, then default is './.zk_install'"
    echo "   -p|--port <port>  listen port of zookeeper, default is 22181"
    echo "   -g|--git          git source to download zookeeper: github|xiaomi, default is github"
}
function run_start_zk()
{
    INSTALL_DIR=`pwd`/.zk_install
    PORT=22181
    GIT_SOURCE=github
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_start_zk
                exit 0
                ;;
            -d|--install_dir)
                INSTALL_DIR=$2
                shift
                ;;
            -p|--port)
                PORT=$2
                shift
                ;;
            -g|--git)
                GIT_SOURCE=$2
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_start_zk
                exit -1
                ;;
        esac
        shift
    done
    INSTALL_DIR="$INSTALL_DIR" PORT="$PORT" GIT_SOURCE="$GIT_SOURCE" ./replication/start_zk.sh
}

#####################
## stop_zk
#####################
function usage_stop_zk()
{
    echo "Options for subcommand 'stop_zk':"
    echo "   -h|--help         print the help info"
    echo "   -d|--install_dir <dir>"
    echo "                     zookeeper install directory,"
    echo "                     if not set, then default is './.zk_install'"
}
function run_stop_zk()
{
    INSTALL_DIR=`pwd`/.zk_install
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_stop_zk
                exit 0
                ;;
            -d|--install_dir)
                INSTALL_DIR=$2
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_stop_zk
                exit -1
                ;;
        esac
        shift
    done
    INSTALL_DIR="$INSTALL_DIR" ./replication/stop_zk.sh
}

#####################
## clear_zk
#####################
function usage_clear_zk()
{
    echo "Options for subcommand 'clear_zk':"
    echo "   -h|--help         print the help info"
    echo "   -d|--install_dir <dir>"
    echo "                     zookeeper install directory,"
    echo "                     if not set, then default is './.zk_install'"
}
function run_clear_zk()
{
    INSTALL_DIR=`pwd`/.zk_install
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_clear_zk
                exit 0
                ;;
            -d|--install_dir)
                INSTALL_DIR=$2
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_clear__zk
                exit -1
                ;;
        esac
        shift
    done
    INSTALL_DIR="$INSTALL_DIR" ./replication/clear_zk.sh
}

#####################
## start_onebox
#####################
function usage_start_onebox()
{
    echo "Options for subcommand 'start_onebox':"
    echo "   -h|--help         print the help info"
    echo "   -m|--meta_count <num>"
    echo "                     meta server count, default is 3"
    echo "   -r|--replica_count <num>"
    echo "                     replica server count, default is 3"
    echo "   -a|--app_name <str>"
    echo "                     default app name, default is rrdb.instance0"
    echo "   -p|--partition_count <num>"
    echo "                     default app partition count, default is 1"
}

function run_start_onebox()
{
    META_COUNT=3
    REPLICA_COUNT=3
    APP_NAME=rrdb.instance0
    PARTITION_COUNT=1
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_start_onebox
                exit 0
                ;;
            -m|--meta_count)
                META_COUNT="$2"
                shift
                ;;
            -r|--replica_count)
                REPLICA_COUNT="$2"
                shift
                ;;
            -a|--app_name)
                APP_NAME="$2"
                shift
                ;;
            -p|--partition_count)
                PARTITION_COUNT="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_start_onebox
                exit -1
                ;;
        esac
        shift
    done
    if [ ! -f ${DSN_ROOT}/bin/rrdb/rrdb ]; then
        echo "ERROR: file ${DSN_ROOT}/bin/rrdb/rrdb not exist"
        exit -1
    fi
    run_start_zk
    sed "s/@LOCAL_IP@/`hostname -i`/g;s/@META_COUNT@/${META_COUNT}/g;s/@REPLICA_COUNT@/${REPLICA_COUNT}/g;s/@APP_NAME@/${APP_NAME}/g;s/@PARTITION_COUNT@/${PARTITION_COUNT}/g" \
        ${ROOT}/replication/config-server.ini >${ROOT}/config-server.ini
    echo "starting server"
    mkdir -p onebox
    cd onebox
    for i in $(seq ${META_COUNT})
    do
        mkdir -p meta$i;
        cd meta$i
        ln -s -f ${DSN_ROOT}/bin/rrdb/rrdb rrdb
        ln -s -f ${ROOT}/config-server.ini config.ini
        echo "cd `pwd` && ./rrdb config.ini -app_list meta@$i &>result &"
        ./rrdb config.ini -app_list meta@$i &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
    done
    for j in $(seq ${REPLICA_COUNT})
    do
        mkdir -p replica$j
        cd replica$j
        ln -s -f ${DSN_ROOT}/bin/rrdb/rrdb rrdb
        ln -s -f ${ROOT}/config-server.ini config.ini
        echo "cd `pwd` && ./rrdb config.ini -app_list replica@$j &>result &"
        ./rrdb config.ini -app_list replica@$j &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
    done
}

#####################
## stop_onebox
#####################
function usage_stop_onebox()
{
    echo "Options for subcommand 'stop_onebox':"
    echo "   -h|--help         print the help info"
}

function run_stop_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_stop_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_stop_onebox
                exit -1
                ;;
        esac
        shift
    done
    ps -ef | grep rrdb | grep 'app_list meta@' | awk '{print $2}' | xargs kill &>/dev/null
    ps -ef | grep rrdb | grep 'app_list replica@' | awk '{print $2}' | xargs kill &>/dev/null
}

#####################
## list_onebox
#####################
function usage_list_onebox()
{
    echo "Options for subcommand 'list_onebox':"
    echo "   -h|--help         print the help info"
}

function run_list_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_list_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_list_onebox
                exit -1
                ;;
        esac
        shift
    done
    ps -ef | grep rrdb | grep app_list
}

#####################
## clear_onebox
#####################
function usage_clear_onebox()
{
    echo "Options for subcommand 'clear_onebox':"
    echo "   -h|--help         print the help info"
}

function run_clear_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_clear_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_clear_onebox
                exit -1
                ;;
        esac
        shift
    done
    run_stop_onebox
    run_clear_zk
    rm -rf onebox *.log *.data config-*.ini &>/dev/null
}

#####################
## start_onebox_instance
#####################
function usage_start_onebox_instance()
{
    echo "Options for subcommand 'start_onebox_instance':"
    echo "   -h|--help         print the help info"
    echo "   -m|--meta_id <num>"
    echo "                     meta server id"
    echo "   -r|--replica_id <num>"
    echo "                     replica server id"
}

function run_start_onebox_instance()
{
    META_ID=0
    REPLICA_ID=0
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_start_onebox_instance
                exit 0
                ;;
            -m|--meta_id)
                META_ID="$2"
                shift
                ;;
            -r|--replica_id)
                REPLICA_ID="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_start_onebox_instance
                exit -1
                ;;
        esac
        shift
    done
    if [ $META_ID = "0" -a $REPLICA_ID = "0" ]; then
        echo "ERROR: no meta_id or replica_id set"
        exit -1
    fi
    if [ $META_ID != "0" -a $REPLICA_ID != "0" ]; then
        echo "ERROR: meta_id and replica_id can only set one"
        exit -1
    fi
    if [ $META_ID != "0" ]; then
        dir=onebox/meta$META_ID
        if [ ! -d $dir ]; then
            echo "ERROR: invalid meta_id"
            exit -1
        fi
        if ps -ef | grep rrdb | grep app_list | grep meta@$META_ID ; then
            echo "INFO: meta@$META_ID already running"
            exit -1
        fi
        cd $dir
        echo "cd `pwd` && ./rrdb config.ini -app_list meta@$META_ID &>result &"
        ./rrdb config.ini -app_list meta@$META_ID &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
        echo "INFO: meta@$META started"
    fi
    if [ $REPLICA_ID != "0" ]; then
        dir=onebox/replica$REPLICA_ID
        if [ ! -d $dir ]; then
            echo "ERROR: invalid replica_id"
            exit -1
        fi
        if ps -ef | grep rrdb | grep app_list | grep replica@$REPLICA_ID ; then
            echo "INFO: replica@$REPLICA_ID already running"
            exit -1
        fi
        cd $dir
        echo "cd `pwd` && ./rrdb config.ini -app_list replica@$REPLICA_ID &>result &"
        ./rrdb config.ini -app_list replica@$REPLICA_ID &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
        echo "INFO: replica@$REPLICA_ID started"
    fi
}

#####################
## stop_onebox_instance
#####################
function usage_stop_onebox_instance()
{
    echo "Options for subcommand 'stop_onebox_instance':"
    echo "   -h|--help         print the help info"
    echo "   -m|--meta_id <num>"
    echo "                     meta server id"
    echo "   -r|--replica_id <num>"
    echo "                     replica server id"
}

function run_stop_onebox_instance()
{
    META_ID=0
    REPLICA_ID=0
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_stop_onebox_instance
                exit 0
                ;;
            -m|--meta_id)
                META_ID="$2"
                shift
                ;;
            -r|--replica_id)
                REPLICA_ID="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_stop_onebox_instance
                exit -1
                ;;
        esac
        shift
    done
    if [ $META_ID = "0" -a $REPLICA_ID = "0" ]; then
        echo "ERROR: no meta_id or replica_id set"
        exit -1
    fi
    if [ $META_ID != "0" -a $REPLICA_ID != "0" ]; then
        echo "ERROR: meta_id and replica_id can only set one"
        exit -1
    fi
    if [ $META_ID != "0" ]; then
        dir=onebox/meta$META_ID
        if [ ! -d $dir ]; then
            echo "ERROR: invalid meta_id"
            exit -1
        fi
        if ! ps -ef | grep rrdb | grep app_list | grep meta@$META_ID ; then
            echo "INFO: meta@$META_ID is not running"
            exit -1
        fi
        ps -ef | grep rrdb | grep "app_list meta@$META_ID\>" | awk '{print $2}' | xargs kill &>/dev/null
        echo "INFO: meta@$META_ID stopped"
    fi
    if [ $REPLICA_ID != "0" ]; then
        dir=onebox/replica$REPLICA_ID
        if [ ! -d $dir ]; then
            echo "ERROR: invalid replica_id"
            exit -1
        fi
        if ! ps -ef | grep rrdb | grep app_list | grep replica@$REPLICA_ID ; then
            echo "INFO: replica@$REPLICA_ID is not running"
            exit -1
        fi
        ps -ef | grep rrdb | grep "app_list replica@$REPLICA_ID\>" | awk '{print $2}' | xargs kill &>/dev/null
        echo "INFO: replica@$REPLICA_ID stopped"
    fi
}

#####################
## restart_onebox_instance
#####################
function usage_restart_onebox_instance()
{
    echo "Options for subcommand 'restart_onebox_instance':"
    echo "   -h|--help         print the help info"
    echo "   -m|--meta_id <num>"
    echo "                     meta server id"
    echo "   -r|--replica_id <num>"
    echo "                     replica server id"
    echo "   -s|--sleep <num>"
    echo "                     sleep time in seconds between stop and start, default is 1"
}

function run_restart_onebox_instance()
{
    META_ID=0
    REPLICA_ID=0
    SLEEP=1
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_restart_onebox_instance
                exit 0
                ;;
            -m|--meta_id)
                META_ID="$2"
                shift
                ;;
            -r|--replica_id)
                REPLICA_ID="$2"
                shift
                ;;
            -s|--sleep)
                SLEEP="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_restart_onebox_instance
                exit -1
                ;;
        esac
        shift
    done
    if [ $META_ID = "0" -a $REPLICA_ID = "0" ]; then
        echo "ERROR: no meta_id or replica_id set"
        exit -1
    fi
    if [ $META_ID != "0" -a $REPLICA_ID != "0" ]; then
        echo "ERROR: meta_id and replica_id can only set one"
        exit -1
    fi
    run_stop_onebox_instance -m $META_ID -r $REPLICA_ID
    echo "sleep $SLEEP"
    sleep $SLEEP
    run_start_onebox_instance -m $META_ID -r $REPLICA_ID
}

#####################
## start_kill_test
#####################
function usage_start_kill_test()
{
    echo "Options for subcommand 'start_kill_test':"
    echo "   -h|--help         print the help info"
    echo "   -m|--meta_count <num>"
    echo "                     meta server count, default is 3"
    echo "   -r|--replica_count <num>"
    echo "                     replica server count, default is 5"
    echo "   -a|--app_name <str>"
    echo "                     app name, default is rrdb.instance0"
    echo "   -p|--partition_count <num>"
    echo "                     app partition count, default is 16"
    echo "   -t|--kill_type <str>"
    echo "                     kill type: meta | replica | all, default is all"
    echo "   -s|--sleep_time <num>"
    echo "                     max sleep time before next kill, default is 300"
    echo "                     actual sleep time will be a random value in range of [1, sleep_time]"
    echo "   -w|--worker_count <num>"
    echo "                     threads count for doing reading & writing. program will start"
    echo "                     this many workers for read and this many workers for write. Default is 2"
}

function run_start_kill_test()
{
    META_COUNT=3
    REPLICA_COUNT=5
    APP_NAME=rrdb.instance0
    PARTITION_COUNT=16
    KILL_TYPE=all
    SLEEP_TIME=300
    WORKER_COUNT=2

    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_start_kill_test
                exit 0
                ;;
            -m|--meta_count)
                META_COUNT="$2"
                shift
                ;;
            -r|--replica_count)
                REPLICA_COUNT="$2"
                shift
                ;;
            -a|--app_name)
                APP_NAME="$2"
                shift
                ;;
            -p|--partition_count)
                PARTITION_COUNT="$2"
                shift
                ;;
            -t|--kill_type)
                KILL_TYPE="$2"
                shift
                ;;
            -s|--sleep_time)
                SLEEP_TIME="$2"
                shift
                ;;
            -w|--worker_count)
                WORKER_COUNT="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_start_kill_test
                exit -1
                ;;
        esac
        shift
    done

    run_clear_kill_test
    echo

    run_start_onebox -m $META_COUNT -r $REPLICA_COUNT -a $APP_NAME -p $PARTITION_COUNT
    echo

    cd $ROOT
    CONFIG=config-kill-test.ini
    sed "s/@LOCAL_IP@/`hostname -i`/g" ${ROOT}/replication/config-kill-test.ini >$CONFIG
    ln -sf ${DSN_ROOT}/bin/rrdb_kill_test/rrdb_kill_test
    echo "./rrdb_kill_test $CONFIG $APP_NAME $WORKER_COUNT &>rrdb_kill_test.log &"
    ./rrdb_kill_test $CONFIG $APP_NAME $WORKER_COUNT &>rrdb_kill_test.log &
    sleep 0.2
    echo

    echo "./kill_test.sh $META_COUNT $REPLICA_COUNT $APP_NAME $KILL_TYPE $SLEEP_TIME &>kill_test.log &"
    ./kill_test.sh $META_COUNT $REPLICA_COUNT $APP_NAME $KILL_TYPE $SLEEP_TIME &>kill_test.log &
    echo

    echo "------------------------------"
    run_list_onebox
    ps -ef | grep rrdb_kill_test | grep -v grep
    ps -ef | grep kill_test.sh | grep -v grep
    echo "------------------------------"
    echo "Server dir: ./onebox"
    echo "Client dir: ./rrdb_kill_test.data"
    echo "Kill   log: ./kill_test.log"
    echo "------------------------------"
}

#####################
## stop_kill_test
#####################
function usage_stop_kill_test()
{
    echo "Options for subcommand 'stop_kill_test':"
    echo "   -h|--help         print the help info"
}

function run_stop_kill_test()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_stop_kill_test
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_stop_kill_test
                exit -1
                ;;
        esac
        shift
    done

    ps -ef | grep rrdb_kill_test | awk '{print $2}' | xargs kill &>/dev/null
    ps -ef | grep kill_test.sh | awk '{print $2}' | xargs kill &>/dev/null
    run_stop_onebox
}

#####################
## clear_kill_test
#####################
function usage_clear_kill_test()
{
    echo "Options for subcommand 'clear_kill_test':"
    echo "   -h|--help         print the help info"
}

function run_clear_kill_test()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_clear_kill_test
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_clear_kill_test
                exit -1
                ;;
        esac
        shift
    done
    run_stop_kill_test
    run_clear_onebox
    rm -rf *.log *.data config-*.ini &>/dev/null
}

#####################
## bench
#####################
function usage_bench()
{
    echo "Options for subcommand 'bench':"
    echo "   -h|--help            print the help info"
    echo "   -c|--config <path>   config file path, default './config-bench.ini'"
    echo "   -t|--type            benchmark type, supporting:"
    echo "                          fillseq_rrdb, fillrandom_rrdb, filluniquerandom_rrdb,"
    echo "                          readrandom_rrdb, deleteseq_rrdb, deleterandom_rrdb"
    echo "                        default is 'fillseq_rrdb,readrandom_rrdb'"
    echo "   -n <num>             number of key/value pairs, default 100000"
    echo "   --cluster_name <str> cluster name, default 'mycluster'"
    echo "   --app_name <str>     app name, default 'rrdb.instance0'"
    echo "   --thread_num <num>   number of threads, default 1"
    echo "   --key_size <num>     key size, default 16"
    echo "   --value_size <num>   value size, default 100"
    echo "   --timeout <num>      timeout in milliseconds, default 10000"
}

function run_bench()
{
    CONFIG=${ROOT}/config-bench.ini
    CONFIG_SPECIFIED=0
    TYPE=fillseq_rrdb,readrandom_rrdb
    NUM=100000
    CLUSTER=mycluster
    APP=rrdb.instance0
    THREAD=1
    KEY_SIZE=16
    VALUE_SIZE=100
    TIMEOUT_MS=10000
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_bench
                exit 0
                ;;
            -c|--config)
                CONFIG="$2"
                CONFIG_SPECIFIED=1
                shift
                ;;
            -t|--type)
                TYPE="$2"
                shift
                ;;
            -n)
                NUM="$2"
                shift
                ;;
            --cluster_name)
                CLUSTER="$2"
                shift
                ;;
            --app_name)
                APP="$2"
                shift
                ;;
            --thread_num)
                THREAD="$2"
                shift
                ;;
            --key_size)
                KEY_SIZE="$2"
                shift
                ;;
            --value_size)
                VALUE_SIZE="$2"
                shift
                ;;
            --timeout)
                TIMEOUT_MS="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_bench
                exit -1
                ;;
        esac
        shift
    done

    if [ ${CONFIG_SPECIFIED} -eq 0 ]; then
        sed "s/@LOCAL_IP@/`hostname -i`/g" ${ROOT}/replication/config-bench.ini >${CONFIG}
    fi

    ./rrdb_bench --rrdb_config=${CONFIG} --benchmarks=${TYPE} --rrdb_timeout_ms=${TIMEOUT_MS} \
        --key_size=${KEY_SIZE} --value_size=${VALUE_SIZE} --threads=${THREAD} --num=${NUM} \
        --rrdb_cluster_name=${CLUSTER} --rrdb_app_name=${APP} --stats_interval=1000 --histogram=1 \
        --compression_type=none --compression_ratio=1.0
}

#####################
## shell
#####################
function usage_shell()
{
    echo "Options for subcommand 'shell':"
    echo "   -h|--help            print the help info"
    echo "   -c|--config <path>   config file path, default './config-shell.ini'"
}

function run_shell()
{
    CONFIG=${ROOT}/config-shell.ini
    CONFIG_SPECIFIED=0
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_shell
                exit 0
                ;;
            -c|--config)
                CONFIG="$2"
                CONFIG_SPECIFIED=1
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_shell
                exit -1
                ;;
        esac
        shift
    done

    if [ ${CONFIG_SPECIFIED} -eq 0 ]; then
        sed "s/@LOCAL_IP@/`hostname -i`/g" ${ROOT}/replication/config-shell.ini >${CONFIG}
    fi

    ${DSN_ROOT}/bin/rrdb_cluster/rrdb_cluster ${CONFIG}
}

####################################################################

if [ $# -eq 0 ]; then
    usage
    exit 0
fi
cmd=$1
case $cmd in
    help)
        usage
        ;;
    build)
        shift
        run_build $*
        ;;
    start_zk)
        shift
        run_start_zk $*
        ;;
    stop_zk)
        shift
        run_stop_zk $*
        ;;
    clear_zk)
        shift
        run_clear_zk $*
        ;;
    start_onebox)
        shift
        run_start_onebox $*
        ;;
    stop_onebox)
        shift
        run_stop_onebox $*
        ;;
    clear_onebox)
        shift
        run_clear_onebox $*
        ;;
    list_onebox)
        shift
        run_list_onebox $*
        ;;
    start_onebox_instance)
        shift
        run_start_onebox_instance $*
        ;;
    stop_onebox_instance)
        shift
        run_stop_onebox_instance $*
        ;;
    restart_onebox_instance)
        shift
        run_restart_onebox_instance $*
        ;;
    start_kill_test)
        shift
        run_start_kill_test $*
        ;;
    stop_kill_test)
        shift
        run_stop_kill_test $*
        ;;
    clear_kill_test)
        shift
        run_clear_kill_test $*
        ;;
    bench)
        shift
        run_bench $*
        ;;
    shell)
        shift
        run_shell $*
        ;;
    *)
        echo "ERROR: unknown command $cmd"
        echo
        usage
        exit -1
esac

