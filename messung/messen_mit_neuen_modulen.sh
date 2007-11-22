#!/bin/sh

cp messung1.xml a.xml &&
../vermont -f a.xml &
pid=$!
echo $pid

sleep 5s
cp messung_neue_module.xml a.xml
kill -USR1 $pid

sleep 10s
kill -INT $pid
sleep 8s
