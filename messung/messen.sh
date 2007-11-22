#!/bin/sh

../vermont -f messung1.xml &
pid=$!
echo $pid

sleep 5s
kill -USR1 $pid

sleep 10s
kill -INT $pid
sleep 8s
