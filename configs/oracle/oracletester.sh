#!/bin/sh

# handle cleanup 
trap cleanup 2

cleanup() {
  echo "Caught Signal ... cleaning up."
  rm -rf /tmp/temp_out.$$
  if[ $XPPID ] && kill $XPPID
  $WRPID && kill $WRPID
  $TPID && kill $TPID
  echo "Done cleanup ... quitting."
  exit 1
}

# Vars
VMT='../../vermont'
EXC='oracleudpexporter.xml'
WRC='oraclewriter.xml'

# Start exporter
echo "Start exporter ..."
$VMT -f $EXC >> /tmp/temp_out.$$ 2>&1 &
XPPID=$(pidof $VMT)
echo "done (pid $XPPID)"


# Start writer
echo "Start writer ..."
$VMT -f $WRC >> /tmp/temp_out.$$ 2>&1 &
WRPID=$(pidof -o $XPPID $VMT)
echo "done (pid $WRPID)"

# Show the output
echo "Printing output ..."
tail -f /tmp/temp_out.$$ 
TPID=$(pidof tail)




