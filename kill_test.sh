#!/bin/bash

if [ $# -ne 5 ]
then
    echo "USAGE: $0 <meta-count> <replica-count> <app-name> <kill-type> <sleep-time>"
    echo "  kill-type: meta | replica | all"
    exit -1
fi
META_COUNT=$1
REPLICA_COUNT=$2
APP_NAME=$3
KILL_TYPE=$4
SLEEP_TIME=$5
if [ "$KILL_TYPE" != "meta" -a "$KILL_TYPE" != "replica" -a "$KILL_TYPE" != "all" ]
then
    echo "ERROR: invalid kill-type, should be: meta | replica | all"
    exit -1
fi

CUR_DIR=`pwd`
LEASE_TIMEOUT=20
UNHEALTHY_TIMEOUT=1200
KILL_META_PROB=10
SHELL_INPUT=.kill_test.shell.input
SHELL_OUTPUT=.kill_test.shell.output
echo "cluster_info" >$SHELL_INPUT
echo "nodes" >>$SHELL_INPUT
echo "app $APP_NAME -detailed" >>$SHELL_INPUT
WAIT_SECONDS=0
while true
do
  if ! ps -ef | grep "rrdb_kill_test" |  grep -v grep &>/dev/null
  then
    echo "[`date`] rrdb_kill_test process is not exist, stop cluster..."
    echo "---------------------------"
    ./run.sh shell <$SHELL_INPUT | grep '^[0-9]\|^primary_meta_server'
    echo "---------------------------"
    echo
    ./run.sh stop_onebox
    exit -1
  fi

  if [ "`find onebox -name 'core.*' | wc -l `" -gt 0 -o "`find onebox -name 'core' | wc -l `" -gt 0 ]
  then
    echo "[`date`] coredump generated, stop rrdb_kill_test and cluster..."
    echo "---------------------------"
    ./run.sh shell <$SHELL_INPUT | grep '^[0-9]\|^primary_meta_server'
    echo "---------------------------"
    echo
    sleep 2
    ps -ef | grep rrdb_kill_test | grep -v grep | awk '{print $2}' | xargs kill
    ./run.sh stop_onebox
    exit -1
  fi

  ./run.sh shell <$SHELL_INPUT &>$SHELL_OUTPUT
  UNALIVE_COUNT=`cat $SHELL_OUTPUT | grep UNALIVE | wc -l`
  PARTITION_COUNT=`cat $SHELL_OUTPUT | awk '{print $3}' | grep '/' | grep -o '^[0-9]*' | wc -l`
  UNHEALTHY_COUNT=`cat $SHELL_OUTPUT | awk '{print $3}' | grep '/' | grep -o '^[0-9]*' | grep -v '3' | wc -l`
  if [ $UNALIVE_COUNT -gt 0 -o $PARTITION_COUNT -eq 0 -o $UNHEALTHY_COUNT -gt 0 ]
  then
    if [ $WAIT_SECONDS -gt $UNHEALTHY_TIMEOUT ]
    then
      echo "[`date`] not healthy for $WAIT_SECONDS seconds, stop rrdb_kill_test and cluster..."
      echo "---------------------------"
      ./run.sh shell <$SHELL_INPUT | grep '^[0-9]\|^primary_meta_server'
      echo "---------------------------"
      echo
      ps -ef | grep rrdb_kill_test | grep -v grep | awk '{print $2}' | xargs kill
      ./run.sh stop_onebox
      exit -1
    fi
    WAIT_SECONDS=$((WAIT_SECONDS+1))
    sleep 1
    continue
  else
    echo "[`date`] healthy now, waited for $WAIT_SECONDS seconds"
    echo "---------------------------"
    cat $SHELL_OUTPUT | grep '^[0-9]\|^primary_meta_server'
    echo "---------------------------"
    echo
  fi

  echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>"
  if [ "$KILL_TYPE" = "meta" ]
  then
      TYPE=meta
  elif [ "$KILL_TYPE" = "replica" ]
  then
      TYPE=relica
  else
      R=$((RANDOM % KILL_META_PROB))
      if [ $R -eq 0 ]
      then
          TYPE=meta
      else
          TYPE=relica
      fi
  fi
  if [ "$TYPE" = "meta" ]
  then
      OPT="-m"
      TASK_ID=$((RANDOM % META_COUNT + 1))
  else
      OPT="-r"
      TASK_ID=$((RANDOM % REPLICA_COUNT + 1))
  fi
  ###
  SECONDS=$((RANDOM % SLEEP_TIME + 1))
  echo "[`date`] sleep for $SECONDS seconds to stop $TYPE #$TASK_ID"
  sleep $SECONDS
  ###
  echo "[`date`] stop $TYPE #$TASK_ID"
  ./run.sh stop_onebox_instance $OPT $TASK_ID
  ###
  SECONDS=$((RANDOM % LEASE_TIMEOUT + 1))
  echo "[`date`] sleep for $SECONDS seconds to start $TYPE #$TASK_ID"
  sleep $SECONDS
  ###
  echo "[`date`] start $TYPE #$TASK_ID"
  ./run.sh start_onebox_instance $OPT $TASK_ID
  ###
  echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<"
  echo

  echo "[`date`] first sleep for $LEASE_TIMEOUT seconds, and then wait for healthy..."
  echo
  sleep $LEASE_TIMEOUT
  WAIT_SECONDS=$LEASE_TIMEOUT
done
