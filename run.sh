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
    echo "   help           print the help info"
    echo "   build          build the system"
    echo "   start_onebox   start rrdb onebox"
    echo "   stop_onebox    stop rrdb onebox"
    echo "   list_onebox    list rrdb onebox"
    echo "   clear_onebox   clear_rrdb onebox"
    echo "   bench          benchmark test"
    echo "   shell          run rrdb_cluster shell"
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
## start_onebox
#####################
function usage_start_onebox()
{
    echo "Options for subcommand 'start_onebox':"
    echo "   -h|--help         print the help info"
    echo "   -m|--meta_count <num>"
    echo "                     meta server count"
    echo "   -r|--replica_count <num>"
    echo "                     replica server count"
}

function run_start_onebox()
{
    META_COUNT=3
    REPLICA_COUNT=3
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
    sed "s/@LOCAL_IP@/`hostname -i`/g;s/@META_COUNT@/${META_COUNT}/g;s/@REPLICA_COUNT@/${REPLICA_COUNT}/g" \
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
    ps -ef | grep rrdb | grep 'app_list meta@' | grep -v grep | awk '{print $2}' | xargs kill &>/dev/null
    ps -ef | grep rrdb | grep 'app_list replica@' | grep -v grep | awk '{print $2}' | xargs kill &>/dev/null
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
    ps -ef | grep rrdb | grep app_list | grep -v grep
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
    if [ -d onebox ]; then
        rm -rf onebox config-server.ini config-client.ini &>/dev/null
    fi
}

#####################
## bench
#####################
function usage_bench()
{
    echo "Options for subcommand 'bench':"
    echo "   -h|--help            print the help info"
    echo "   -c|--config <path>   config file path, default './config-client.ini'"
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
    CONFIG=${ROOT}/config-client.ini
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
        sed "s/@LOCAL_IP@/`hostname -i`/g" ${ROOT}/replication/config-client.ini >${CONFIG}
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
    echo "   -c|--config <path>   config file path, default './config-client.ini'"
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

