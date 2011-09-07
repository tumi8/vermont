#!/bin/sh

# handle cleanup 
trap cleanup 2

cleanup() {
  echo "Caught Signal ... cleaning up."
  rm -rf /tmp/temp_out.$$
  if [ -n "$XPPID" ]; then 
    echo "shutting down exporter ..."
    kill $XPPID
  fi
  if [ -n "$WRPID" ]; then
    echo "shutting down writer ..."
    kill $WRPID
  fi
  if [ -n "$TPID" ]; then
    echo "shutting down log viewer ..."
    kill $TPID
  fi
  echo "Done cleanup ... quitting."
  exit 1
}

# Vars
VMT='../../vermont'
EXC='udpexp.xml'
WRC='oxewriter.xml'

print_output(){
  # Show the output
  echo "\n\nOUTPUT:\n\n"
  tail -f /tmp/temp_out.$$ 
  TPID=$(pidof tail)
}

# Tests
test_exporter() {
  echo "testing exporter"
  $VMT -ddd -f $EXC >> /tmp/temp_out.$$ 2>&1 &
  XPPID=$(pidof $VMT)
  echo "done (pid $XPPID)"
  print_output
}

test_writer() {
  echo "testing writer"
  $VMT -ddd -f $WRC >> /tmp/temp_out.$$ 2>&1 &
  WRPID=$(pidof -o $XPPID $VMT)
  echo "done (pid $WRPID)"
  print_output
}

test_to_db() {
  echo "testing write to db"
}

# Run
command=`basename $0`
usage="Usage: $command -h [-t test]"

while getopts ht: o; do
case "$o" in
    h) echo $usage && exit 1;;
    t) testcase="$OPTARG";;
esac
done

case $testcase in
  exp) test_exporter;;
  wr) test_writer;;
esac






