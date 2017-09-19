#!/bin/bash
set -e
#set -x

CUR_DIR=`pwd`
PID=$1
OPTIONS=$2
ATTACH_JAR=attach-main.jar
PERF_MAP_DIR="$(cd "$(dirname "$0")" && pwd -P)"/..
ATTACH_JAR_PATH=$PERF_MAP_DIR/out/$ATTACH_JAR
PERF_MAP_FILE=/tmp/perf-$PID.map
if [[ `uname` == 'Linux' ]]; then
  LINUX=1;
else
  LINUX=2;
fi

if [[ "$LINUX" == "1" ]]; then
  if [ ! -d /proc/$PID ]; then
    echo "PID $PID not found"
    exit 1
  fi
  TARGET_UID=$(awk '/^Uid:/{print $2}' /proc/$PID/status)
  TARGET_GID=$(awk '/^Gid:/{print $2}' /proc/$PID/status)
fi

if [ -z "$JAVA_HOME" ]; then
  if [[ "$LINUX" == "1" ]]; then
    JAVA_HOME=/usr/lib/jvm/default-java

    [ -d "$JAVA_HOME" ] || JAVA_HOME=/etc/alternatives/java_sdk
  else
    JAVA_HOME=`/usr/libexec/java_home -v 1.8`
  fi
fi
[ -d "$JAVA_HOME" ] || (echo "JAVA_HOME directory at '$JAVA_HOME' does not exist." && false)


if [[ "$LINUX" == "1" ]]; then
  sudo rm $PERF_MAP_FILE -f
  (cd $PERF_MAP_DIR/out && sudo -u \#$TARGET_UID -g \#$TARGET_GID $JAVA_HOME/bin/java -cp $ATTACH_JAR_PATH:$JAVA_HOME/lib/tools.jar net.virtualvoid.perf.AttachOnce $PID "$OPTIONS")
  sudo chown root:root $PERF_MAP_FILE
else
  rm -f $PERF_MAP_FILE
  (cd $PERF_MAP_DIR/out && $JAVA_HOME/bin/java -cp $ATTACH_JAR_PATH:$JAVA_HOME/lib/tools.jar net.virtualvoid.perf.AttachOnce $PID "$OPTIONS")
fi
