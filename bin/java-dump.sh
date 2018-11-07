#!/bin/bash
set -e
set -x

CUR_DIR=`pwd`
ATTACH_JAR=attach-main.jar
PERF_MAP_DIR=$(dirname $(readlink -f $0))/..
ATTACH_JAR_PATH=$PERF_MAP_DIR/out/$ATTACH_JAR

if [ -z "$JAVA_HOME" ]; then
  JAVA_HOME=/usr/lib/jvm/default-java
fi

[ -d "$JAVA_HOME" ] || JAVA_HOME=/etc/alternatives/java_sdk
[ -d "$JAVA_HOME" ] || (echo "JAVA_HOME directory at '$JAVA_HOME' does not exist." && false)

echo $* >> output.log
(cd $PERF_MAP_DIR/out && sudo -u johannes java -cp $ATTACH_JAR_PATH:$JAVA_HOME/lib/tools.jar net.virtualvoid.perf.AttachOnce $*)
