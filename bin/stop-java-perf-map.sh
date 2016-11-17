#!/bin/bash
set -e
#set -x

CUR_DIR=`pwd`
PID=$1
ATTACH_JAR=attach-main.jar
PERF_MAP_DIR=$(dirname $(readlink -f $0))/..
ATTACH_JAR_PATH=$PERF_MAP_DIR/out/$ATTACH_JAR
PERF_MAP_FILE=/tmp/perf-$PID.map

if [ -z "$JAVA_HOME" ]; then
  JAVA_HOME=/usr/lib/jvm/default-java
fi

[ -d "$JAVA_HOME" ] || JAVA_HOME=/etc/alternatives/java_sdk
[ -d "$JAVA_HOME" ] || (echo "JAVA_HOME directory at '$JAVA_HOME' does not exist." && false)

(cd $PERF_MAP_DIR/out && java -cp $ATTACH_JAR_PATH:$JAVA_HOME/lib/tools.jar net.virtualvoid.perf.AttachOnce $PID stop)

RAW=${PERF_MAP_FILE}.raw
mv $PERF_MAP_FILE $RAW
cat $RAW | java -cp $PERF_MAP_DIR/out/merge-perf-map.jar MergePerfMap >$PERF_MAP_FILE
