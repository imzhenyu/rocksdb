#!/bin/bash

if [ $# -ne 4 ]
then
    echo "USAGE: $0 <meta-count> <replica-count> <app-name> <kill-type>"
    echo "  kill-type: meta | replica | all"
    exit -1
fi
META_COUNT=$1
REPLICA_COUNT=$2
APP_NAME=$3
KILL_TYPE=$4
if [ "$KILL_TYPE" != "meta" -a "$KILL_TYPE" != "replica" -a "$KILL_TYPE" != "all" ]
then
    echo "ERROR: invalid kill-type, should be: meta | replica | all"
    exit -1
fi

CUR_DIR=`pwd`
LEASE_TIMEOUT=20
WAIT_SECONDS=0
SHELL_INPUT=.shell.input
echo "app $APP_NAME -detailed" >$SHELL_INPUT
while true
do
  if ! ps -ef | grep "rrdb_kill_test" |  grep -v grep &>/dev/null
  then
    echo "[`date`] rrdb_kill_test process is not exist, stop cluster..."
    echo
    ./run.sh stop_onebox
    exit -1
  fi

  if [ "`find onebox -name 'core.*' | wc -l `" -gt 0 ]
  then
    echo "[`date`] coredump generated, stop rrdb_kill_test and cluster..."
    echo
    ps -ef | grep rrdb_kill_test | grep -v grep | awk '{print $2}' | xargs kill
    ./run.sh stop_onebox
    exit -1
  fi

  NUM=`./run.sh shell <$SHELL_INPUT | awk '{print $3}' | grep '/' | grep -o '^[0-9]*' | grep -v '3' | wc -l`
  if [ $NUM -ne 0 ]
  then
    if [ $WAIT_SECONDS -gt 1200 ]
    then
      echo "[`date`] not healthy for $WAIT_SECONDS seconds, stop rrdb_kill_test and cluster..."
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
    ./run.sh shell <$SHELL_INPUT | grep '^[0-9]'
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
      R=$((RANDOM % 10))
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
  SECONDS=$((RANDOM % 300 + 1))
  echo "[`date`] sleep for $SECONDS seconds to stop $TYPE #$TASK_ID"
  sleep $SECONDS
  ###
  echo "[`date`] stop replica #$TASK_ID"
  ./run.sh stop_onebox_instance $OPT $TASK_ID
  ###
  SECONDS=$((RANDOM % LEASE_TIMEOUT + 1))
  echo "[`date`] sleep for $SECONDS seconds to start $TYPE #$TASK_ID"
  sleep $SECONDS
  ###
  echo "[`date`] start replica #$TASK_ID"
  ./run.sh start_onebox_instance $OPT $TASK_ID
  ###
  echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<"
  echo

  echo "[`date`] wait for healthy..."
  echo
  sleep $LEASE_TIMEOUT
  WAIT_SECONDS=$LEASE_TIMEOUT
done
