#!/bin/bash

#measure queue implementation
if [ ! -d queueImp ]; then
	mkdir queueImp
fi

#scheduler core settings
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 10000000 --output "queueImp/1-10-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 10000000 --output "queueImp/1-100-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 10000000 --output "queueImp/1-1000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 10000000 --output "queueImp/1-10000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 10000000 --output "queueImp/1-10000000-11-out.txt"

./comptest --rep 10 --comp 1 --qtype 2 --qsize 10 --qops 10000000 --output "queueImp/2-10-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 2 --qsize 100 --qops 10000000 --output "queueImp/2-100-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 2 --qsize 1000 --qops 10000000 --output "queueImp/2-1000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 2 --qsize 10000 --qops 10000000 --output "queueImp/2-10000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 2 --qsize 10000000 --qops 10000000 --output "queueImp/2-10000000-11-out.txt"

./comptest --rep 10 --comp 1 --qtype 3 --qsize 10 --qops 10000000 --output "queueImp/3-10-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 3 --qsize 100 --qops 10000000 --output "queueImp/3-100-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 3 --qsize 1000 --qops 10000000 --output "queueImp/3-1000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 3 --qsize 10000 --qops 10000000 --output "queueImp/3-10000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 3 --qsize 10000000 --qops 10000000 --output "queueImp/3-10000000-11-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 10000000 --output "queueImp/4-10-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 10000000 --output "queueImp/4-100-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 10000000 --output "queueImp/4-1000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 10000000 --output "queueImp/4-10000-11-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 10000000 --output "queueImp/4-10000000-11-out.txt"


#good core settings
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 10000000 --output "queueImp/1-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 10000000 --output "queueImp/1-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 10000000 --output "queueImp/1-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 10000000 --output "queueImp/1-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 10000000 --output "queueImp/1-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 10 --qops 10000000 --output "queueImp/2-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 100 --qops 10000000 --output "queueImp/2-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 1000 --qops 10000000 --output "queueImp/2-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 10000 --qops 10000000 --output "queueImp/2-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 10000000 --qops 10000000 --output "queueImp/2-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 10 --qops 10000000 --output "queueImp/3-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 100 --qops 10000000 --output "queueImp/3-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 1000 --qops 10000000 --output "queueImp/3-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 10000 --qops 10000000 --output "queueImp/3-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 10000000 --qops 10000000 --output "queueImp/3-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 10000000 --output "queueImp/4-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 10000000 --output "queueImp/4-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 10000000 --output "queueImp/4-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 10000000 --output "queueImp/4-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 10000000 --output "queueImp/4-10000000-11-out-goodcpu.txt"

#bad core settings
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 10000000 --output "queueImp/1-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 10000000 --output "queueImp/1-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 10000000 --output "queueImp/1-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 10000000 --output "queueImp/1-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 10000000 --output "queueImp/1-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 10 --qops 10000000 --output "queueImp/2-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 100 --qops 10000000 --output "queueImp/2-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 1000 --qops 10000000 --output "queueImp/2-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 10000 --qops 10000000 --output "queueImp/2-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 2 --qsize 10000000 --qops 10000000 --output "queueImp/2-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 10 --qops 10000000 --output "queueImp/3-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 100 --qops 10000000 --output "queueImp/3-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 1000 --qops 10000000 --output "queueImp/3-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 10000 --qops 10000000 --output "queueImp/3-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 3 --qsize 10000000 --qops 10000000 --output "queueImp/3-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 10000000 --output "queueImp/4-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 10000000 --output "queueImp/4-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 10000000 --output "queueImp/4-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 10000000 --output "queueImp/4-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 10000000 --output "queueImp/4-10000000-11-out-badcpu.txt"

#multi producers
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 1000000 --qprod 2 --output "queueImp/1-10-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 1000000 --qprod 3 --output "queueImp/1-10-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 1000000 --qprod 5 --output "queueImp/1-10-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 1000000 --qprod 2 --output "queueImp/1-100-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 1000000 --qprod 3 --output "queueImp/1-100-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 1000000 --qprod 5 --output "queueImp/1-100-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 1000000 --qprod 2 --output "queueImp/1-1000-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 1000000 --qprod 3 --output "queueImp/1-1000-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 1000000 --qprod 5 --output "queueImp/1-1000-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 1000000 --qprod 2 --output "queueImp/1-10000-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 1000000 --qprod 3 --output "queueImp/1-10000-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 1000000 --qprod 5 --output "queueImp/1-10000-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 1000000 --qprod 2 --output "queueImp/1-10000000-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 1000000 --qprod 3 --output "queueImp/1-10000000-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 1000000 --qprod 5 --output "queueImp/1-10000000-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 1000000 --qprod 2 --output "queueImp/4-10-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 1000000 --qprod 3 --output "queueImp/4-10-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 1000000 --qprod 5 --output "queueImp/4-10-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 1000000 --qprod 2 --output "queueImp/4-100-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 1000000 --qprod 3 --output "queueImp/4-100-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 1000000 --qprod 5 --output "queueImp/4-100-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 1000000 --qprod 2 --output "queueImp/4-1000-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 1000000 --qprod 3 --output "queueImp/4-1000-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 1000000 --qprod 5 --output "queueImp/4-1000-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 1000000 --qprod 2 --output "queueImp/4-10000-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 1000000 --qprod 3 --output "queueImp/4-10000-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 1000000 --qprod 5 --output "queueImp/4-10000-51-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 1000000 --qprod 2 --output "queueImp/4-10000000-21-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 1000000 --qprod 3 --output "queueImp/4-10000000-31-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 1000000 --qprod 5 --output "queueImp/4-10000000-51-out.txt"

#multi producers/consumers
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/1-10-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/1-10-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/1-10-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/1-100-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/1-100-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 100 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/1-100-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/1-1000-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/1-1000-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 1000 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/1-1000-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/1-10000-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/1-10000-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/1-10000-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/1-10000000-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/1-10000000-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 1 --qsize 10000000 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/1-10000000-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/4-10-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/4-10-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/4-10-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/4-100-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/4-100-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 100 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/4-100-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/4-1000-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/4-1000-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 1000 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/4-1000-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/4-10000-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/4-10000-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/4-10000-55-out.txt"

./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 1000000 --qprod 2 --qcons 2 --output "queueImp/4-10000000-22-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 1000000 --qprod 3 --qcons 3 --output "queueImp/4-10000000-33-out.txt"
./comptest --rep 10 --comp 1 --qtype 4 --qsize 10000000 --qops 1000000 --qprod 5 --qcons 5 --output "queueImp/4-10000000-55-out.txt"


#measure ConcurrentQueue
if [ ! -d concurrentQueue ]; then
	mkdir concurrentQueue
fi

#scheduler core setting
#old concurrentQueue
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10 --qops 10000000 --output "concurrentQueue/11-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 100 --qops 10000000 --output "concurrentQueue/11-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 1000 --qops 10000000 --output "concurrentQueue/11-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000 --qops 10000000 --output "concurrentQueue/11-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000000 --qops 10000000 --output "concurrentQueue/11-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 12 --qsize 10 --qops 10000000 --output "concurrentQueue/12-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 12 --qsize 100 --qops 10000000 --output "concurrentQueue/12-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 12 --qsize 1000 --qops 10000000 --output "concurrentQueue/12-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 12 --qsize 10000 --qops 10000000 --output "concurrentQueue/12-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 12 --qsize 10000000 --qops 10000000 --output "concurrentQueue/12-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 13 --qsize 10 --qops 10000000 --output "concurrentQueue/13-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 13 --qsize 100 --qops 10000000 --output "concurrentQueue/13-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 13 --qsize 1000 --qops 10000000 --output "concurrentQueue/13-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 13 --qsize 10000 --qops 10000000 --output "concurrentQueue/13-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 13 --qsize 10000000 --qops 10000000 --output "concurrentQueue/13-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 14 --qsize 10 --qops 10000000 --output "concurrentQueue/14-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 100 --qops 10000000 --output "concurrentQueue/14-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 1000 --qops 10000000 --output "concurrentQueue/14-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000 --qops 10000000 --output "concurrentQueue/14-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000000 --qops 10000000 --output "concurrentQueue/14-10000000-11-out.txt"

#concurrentQueueCond
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10 --qops 10000000 --output "concurrentQueue/21-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 100 --qops 10000000 --output "concurrentQueue/21-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 1000 --qops 10000000 --output "concurrentQueue/21-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000 --qops 10000000 --output "concurrentQueue/21-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000000 --qops 10000000 --output "concurrentQueue/21-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 22 --qsize 10 --qops 10000000 --output "concurrentQueue/22-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 22 --qsize 100 --qops 10000000 --output "concurrentQueue/22-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 22 --qsize 1000 --qops 10000000 --output "concurrentQueue/22-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 22 --qsize 10000 --qops 10000000 --output "concurrentQueue/22-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 22 --qsize 10000000 --qops 10000000 --output "concurrentQueue/22-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 23 --qsize 10 --qops 10000000 --output "concurrentQueue/23-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 23 --qsize 100 --qops 10000000 --output "concurrentQueue/23-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 23 --qsize 1000 --qops 10000000 --output "concurrentQueue/23-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 23 --qsize 10000 --qops 10000000 --output "concurrentQueue/23-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 23 --qsize 10000000 --qops 10000000 --output "concurrentQueue/23-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 24 --qsize 10 --qops 10000000 --output "concurrentQueue/24-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 100 --qops 10000000 --output "concurrentQueue/24-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 1000 --qops 10000000 --output "concurrentQueue/24-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000 --qops 10000000 --output "concurrentQueue/24-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000000 --qops 10000000 --output "concurrentQueue/24-10000000-11-out.txt"

#concurrentQueueSpinLock
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10 --qops 10000000 --output "concurrentQueue/31-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 100 --qops 10000000 --output "concurrentQueue/31-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 1000 --qops 10000000 --output "concurrentQueue/31-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000 --qops 10000000 --output "concurrentQueue/31-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000000 --qops 10000000 --output "concurrentQueue/31-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 32 --qsize 10 --qops 10000000 --output "concurrentQueue/32-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 32 --qsize 100 --qops 10000000 --output "concurrentQueue/32-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 32 --qsize 1000 --qops 10000000 --output "concurrentQueue/32-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 32 --qsize 10000 --qops 10000000 --output "concurrentQueue/32-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 32 --qsize 10000000 --qops 10000000 --output "concurrentQueue/32-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 33 --qsize 10 --qops 10000000 --output "concurrentQueue/33-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 33 --qsize 100 --qops 10000000 --output "concurrentQueue/33-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 33 --qsize 1000 --qops 10000000 --output "concurrentQueue/33-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 33 --qsize 10000 --qops 10000000 --output "concurrentQueue/33-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 33 --qsize 10000000 --qops 10000000 --output "concurrentQueue/33-10000000-11-out.txt"

./comptest --rep 10 --comp 2 --qtype 34 --qsize 10 --qops 10000000 --output "concurrentQueue/34-10-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 100 --qops 10000000 --output "concurrentQueue/34-100-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 1000 --qops 10000000 --output "concurrentQueue/34-1000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000 --qops 10000000 --output "concurrentQueue/34-10000-11-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000000 --qops 10000000 --output "concurrentQueue/34-10000000-11-out.txt"

#good cpu settings
#old concurrentQueue
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 10 --qops 10000000 --output "concurrentQueue/11-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 100 --qops 10000000 --output "concurrentQueue/11-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 1000 --qops 10000000 --output "concurrentQueue/11-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000 --qops 10000000 --output "concurrentQueue/11-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000000 --qops 10000000 --output "concurrentQueue/11-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 10 --qops 10000000 --output "concurrentQueue/12-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 100 --qops 10000000 --output "concurrentQueue/12-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 1000 --qops 10000000 --output "concurrentQueue/12-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 10000 --qops 10000000 --output "concurrentQueue/12-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 10000000 --qops 10000000 --output "concurrentQueue/12-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 10 --qops 10000000 --output "concurrentQueue/13-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 100 --qops 10000000 --output "concurrentQueue/13-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 1000 --qops 10000000 --output "concurrentQueue/13-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 10000 --qops 10000000 --output "concurrentQueue/13-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 10000000 --qops 10000000 --output "concurrentQueue/13-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 10 --qops 10000000 --output "concurrentQueue/14-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 100 --qops 10000000 --output "concurrentQueue/14-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 1000 --qops 10000000 --output "concurrentQueue/14-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000 --qops 10000000 --output "concurrentQueue/14-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000000 --qops 10000000 --output "concurrentQueue/14-10000000-11-out-goodcpu.txt"

#concurrentQueueCond
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 10 --qops 10000000 --output "concurrentQueue/21-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 100 --qops 10000000 --output "concurrentQueue/21-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 1000 --qops 10000000 --output "concurrentQueue/21-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000 --qops 10000000 --output "concurrentQueue/21-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000000 --qops 10000000 --output "concurrentQueue/21-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 10 --qops 10000000 --output "concurrentQueue/22-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 100 --qops 10000000 --output "concurrentQueue/22-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 1000 --qops 10000000 --output "concurrentQueue/22-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 10000 --qops 10000000 --output "concurrentQueue/22-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 10000000 --qops 10000000 --output "concurrentQueue/22-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 10 --qops 10000000 --output "concurrentQueue/23-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 100 --qops 10000000 --output "concurrentQueue/23-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 1000 --qops 10000000 --output "concurrentQueue/23-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 10000 --qops 10000000 --output "concurrentQueue/23-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 10000000 --qops 10000000 --output "concurrentQueue/23-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 10 --qops 10000000 --output "concurrentQueue/24-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 100 --qops 10000000 --output "concurrentQueue/24-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 1000 --qops 10000000 --output "concurrentQueue/24-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000 --qops 10000000 --output "concurrentQueue/24-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000000 --qops 10000000 --output "concurrentQueue/24-10000000-11-out-goodcpu.txt"

#concurrentQueueSpinLock
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 10 --qops 10000000 --output "concurrentQueue/31-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 100 --qops 10000000 --output "concurrentQueue/31-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 1000 --qops 10000000 --output "concurrentQueue/31-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000 --qops 10000000 --output "concurrentQueue/31-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000000 --qops 10000000 --output "concurrentQueue/31-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 10 --qops 10000000 --output "concurrentQueue/32-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 100 --qops 10000000 --output "concurrentQueue/32-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 1000 --qops 10000000 --output "concurrentQueue/32-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 10000 --qops 10000000 --output "concurrentQueue/32-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 10000000 --qops 10000000 --output "concurrentQueue/32-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 10 --qops 10000000 --output "concurrentQueue/33-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 100 --qops 10000000 --output "concurrentQueue/33-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 1000 --qops 10000000 --output "concurrentQueue/33-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 10000 --qops 10000000 --output "concurrentQueue/33-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 10000000 --qops 10000000 --output "concurrentQueue/33-10000000-11-out-goodcpu.txt"

taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 10 --qops 10000000 --output "concurrentQueue/34-10-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 100 --qops 10000000 --output "concurrentQueue/34-100-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 1000 --qops 10000000 --output "concurrentQueue/34-1000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000 --qops 10000000 --output "concurrentQueue/34-10000-11-out-goodcpu.txt"
taskset -c 0,1 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000000 --qops 10000000 --output "concurrentQueue/34-10000000-11-out-goodcpu.txt"

#bad cpu settings
#old concurrentQueue
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 10 --qops 10000000 --output "concurrentQueue/11-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 100 --qops 10000000 --output "concurrentQueue/11-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 1000 --qops 10000000 --output "concurrentQueue/11-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000 --qops 10000000 --output "concurrentQueue/11-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000000 --qops 10000000 --output "concurrentQueue/11-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 10 --qops 10000000 --output "concurrentQueue/12-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 100 --qops 10000000 --output "concurrentQueue/12-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 1000 --qops 10000000 --output "concurrentQueue/12-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 10000 --qops 10000000 --output "concurrentQueue/12-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 12 --qsize 10000000 --qops 10000000 --output "concurrentQueue/12-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 10 --qops 10000000 --output "concurrentQueue/13-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 100 --qops 10000000 --output "concurrentQueue/13-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 1000 --qops 10000000 --output "concurrentQueue/13-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 10000 --qops 10000000 --output "concurrentQueue/13-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 13 --qsize 10000000 --qops 10000000 --output "concurrentQueue/13-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 10 --qops 10000000 --output "concurrentQueue/14-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 100 --qops 10000000 --output "concurrentQueue/14-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 1000 --qops 10000000 --output "concurrentQueue/14-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000 --qops 10000000 --output "concurrentQueue/14-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000000 --qops 10000000 --output "concurrentQueue/14-10000000-11-out-badcpu.txt"

#concurrentQueueCond
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 10 --qops 10000000 --output "concurrentQueue/21-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 100 --qops 10000000 --output "concurrentQueue/21-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 1000 --qops 10000000 --output "concurrentQueue/21-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000 --qops 10000000 --output "concurrentQueue/21-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000000 --qops 10000000 --output "concurrentQueue/21-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 10 --qops 10000000 --output "concurrentQueue/22-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 100 --qops 10000000 --output "concurrentQueue/22-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 1000 --qops 10000000 --output "concurrentQueue/22-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 10000 --qops 10000000 --output "concurrentQueue/22-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 22 --qsize 10000000 --qops 10000000 --output "concurrentQueue/22-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 10 --qops 10000000 --output "concurrentQueue/23-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 100 --qops 10000000 --output "concurrentQueue/23-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 1000 --qops 10000000 --output "concurrentQueue/23-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 10000 --qops 10000000 --output "concurrentQueue/23-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 23 --qsize 10000000 --qops 10000000 --output "concurrentQueue/23-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 10 --qops 10000000 --output "concurrentQueue/24-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 100 --qops 10000000 --output "concurrentQueue/24-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 1000 --qops 10000000 --output "concurrentQueue/24-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000 --qops 10000000 --output "concurrentQueue/24-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000000 --qops 10000000 --output "concurrentQueue/24-10000000-11-out-badcpu.txt"

#concurrentQueueSpinLock
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 10 --qops 10000000 --output "concurrentQueue/31-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 100 --qops 10000000 --output "concurrentQueue/31-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 1000 --qops 10000000 --output "concurrentQueue/31-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000 --qops 10000000 --output "concurrentQueue/31-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000000 --qops 10000000 --output "concurrentQueue/31-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 10 --qops 10000000 --output "concurrentQueue/32-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 100 --qops 10000000 --output "concurrentQueue/32-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 1000 --qops 10000000 --output "concurrentQueue/32-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 10000 --qops 10000000 --output "concurrentQueue/32-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 32 --qsize 10000000 --qops 10000000 --output "concurrentQueue/32-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 10 --qops 10000000 --output "concurrentQueue/33-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 100 --qops 10000000 --output "concurrentQueue/33-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 1000 --qops 10000000 --output "concurrentQueue/33-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 10000 --qops 10000000 --output "concurrentQueue/33-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 33 --qsize 10000000 --qops 10000000 --output "concurrentQueue/33-10000000-11-out-badcpu.txt"

taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 10 --qops 10000000 --output "concurrentQueue/34-10-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 100 --qops 10000000 --output "concurrentQueue/34-100-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 1000 --qops 10000000 --output "concurrentQueue/34-1000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000 --qops 10000000 --output "concurrentQueue/34-10000-11-out-badcpu.txt"
taskset -c 0,2 ./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000000 --qops 10000000 --output "concurrentQueue/34-10000000-11-out-badcpu.txt"

#multi producers old concurrentQueue
./comptest --rep 10 --comp 2 --qtype 11 --qsize 100 --qops 1000000 --qprod 2 --output "concurrentQueue/11-100-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 100 --qops 1000000 --qprod 3 --output "concurrentQueue/11-100-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 100 --qops 1000000 --qprod 5 --output "concurrentQueue/11-100-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 11 --qsize 1000 --qops 1000000 --qprod 2 --output "concurrentQueue/11-1000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 1000 --qops 1000000 --qprod 3 --output "concurrentQueue/11-1000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 1000 --qops 1000000 --qprod 5 --output "concurrentQueue/11-1000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000 --qops 1000000 --qprod 2 --output "concurrentQueue/11-10000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000 --qops 1000000 --qprod 3 --output "concurrentQueue/11-10000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000 --qops 1000000 --qprod 5 --output "concurrentQueue/11-10000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000000 --qops 1000000 --qprod 2 --output "concurrentQueue/11-10000000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000000 --qops 1000000 --qprod 3 --output "concurrentQueue/11-10000000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 11 --qsize 10000000 --qops 1000000 --qprod 5 --output "concurrentQueue/11-10000000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 14 --qsize 100 --qops 1000000 --qprod 2 --output "concurrentQueue/14-100-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 100 --qops 1000000 --qprod 3 --output "concurrentQueue/14-100-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 100 --qops 1000000 --qprod 5 --output "concurrentQueue/14-100-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 14 --qsize 1000 --qops 1000000 --qprod 2 --output "concurrentQueue/14-1000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 1000 --qops 1000000 --qprod 3 --output "concurrentQueue/14-1000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 1000 --qops 1000000 --qprod 5 --output "concurrentQueue/14-1000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000 --qops 1000000 --qprod 2 --output "concurrentQueue/14-10000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000 --qops 1000000 --qprod 3 --output "concurrentQueue/14-10000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000 --qops 1000000 --qprod 5 --output "concurrentQueue/14-10000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000000 --qops 1000000 --qprod 2 --output "concurrentQueue/14-10000000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000000 --qops 1000000 --qprod 3 --output "concurrentQueue/14-10000000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 14 --qsize 10000000 --qops 1000000 --qprod 5 --output "concurrentQueue/14-10000000-51-out.txt"

#multi producers concurrentQueueCond
./comptest --rep 10 --comp 2 --qtype 21 --qsize 100 --qops 1000000 --qprod 2 --output "concurrentQueue/21-100-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 100 --qops 1000000 --qprod 3 --output "concurrentQueue/21-100-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 100 --qops 1000000 --qprod 5 --output "concurrentQueue/21-100-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 21 --qsize 1000 --qops 1000000 --qprod 2 --output "concurrentQueue/21-1000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 1000 --qops 1000000 --qprod 3 --output "concurrentQueue/21-1000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 1000 --qops 1000000 --qprod 5 --output "concurrentQueue/21-1000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000 --qops 1000000 --qprod 2 --output "concurrentQueue/21-10000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000 --qops 1000000 --qprod 3 --output "concurrentQueue/21-10000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000 --qops 1000000 --qprod 5 --output "concurrentQueue/21-10000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000000 --qops 1000000 --qprod 2 --output "concurrentQueue/21-10000000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000000 --qops 1000000 --qprod 3 --output "concurrentQueue/21-10000000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 21 --qsize 10000000 --qops 1000000 --qprod 5 --output "concurrentQueue/21-10000000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 24 --qsize 100 --qops 1000000 --qprod 2 --output "concurrentQueue/24-100-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 100 --qops 1000000 --qprod 3 --output "concurrentQueue/24-100-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 100 --qops 1000000 --qprod 5 --output "concurrentQueue/24-100-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 24 --qsize 1000 --qops 1000000 --qprod 2 --output "concurrentQueue/24-1000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 1000 --qops 1000000 --qprod 3 --output "concurrentQueue/24-1000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 1000 --qops 1000000 --qprod 5 --output "concurrentQueue/24-1000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000 --qops 1000000 --qprod 2 --output "concurrentQueue/24-10000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000 --qops 1000000 --qprod 3 --output "concurrentQueue/24-10000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000 --qops 1000000 --qprod 5 --output "concurrentQueue/24-10000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000000 --qops 1000000 --qprod 2 --output "concurrentQueue/24-10000000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000000 --qops 1000000 --qprod 3 --output "concurrentQueue/24-10000000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 24 --qsize 10000000 --qops 1000000 --qprod 5 --output "concurrentQueue/24-10000000-51-out.txt"

#multi producers old concurrentQueueSpinlock
./comptest --rep 10 --comp 2 --qtype 31 --qsize 100 --qops 1000000 --qprod 2 --output "concurrentQueue/31-100-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 100 --qops 1000000 --qprod 3 --output "concurrentQueue/31-100-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 100 --qops 1000000 --qprod 5 --output "concurrentQueue/31-100-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 31 --qsize 1000 --qops 1000000 --qprod 2 --output "concurrentQueue/31-1000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 1000 --qops 1000000 --qprod 3 --output "concurrentQueue/31-1000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 1000 --qops 1000000 --qprod 5 --output "concurrentQueue/31-1000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000 --qops 1000000 --qprod 2 --output "concurrentQueue/31-10000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000 --qops 1000000 --qprod 3 --output "concurrentQueue/31-10000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000 --qops 1000000 --qprod 5 --output "concurrentQueue/31-10000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000000 --qops 1000000 --qprod 2 --output "concurrentQueue/31-10000000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000000 --qops 1000000 --qprod 3 --output "concurrentQueue/31-10000000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 31 --qsize 10000000 --qops 1000000 --qprod 5 --output "concurrentQueue/31-10000000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 34 --qsize 100 --qops 1000000 --qprod 2 --output "concurrentQueue/34-100-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 100 --qops 1000000 --qprod 3 --output "concurrentQueue/34-100-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 100 --qops 1000000 --qprod 5 --output "concurrentQueue/34-100-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 34 --qsize 1000 --qops 1000000 --qprod 2 --output "concurrentQueue/34-1000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 1000 --qops 1000000 --qprod 3 --output "concurrentQueue/34-1000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 1000 --qops 1000000 --qprod 5 --output "concurrentQueue/34-1000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000 --qops 1000000 --qprod 2 --output "concurrentQueue/34-10000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000 --qops 1000000 --qprod 3 --output "concurrentQueue/34-10000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000 --qops 1000000 --qprod 5 --output "concurrentQueue/34-10000-51-out.txt"

./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000000 --qops 1000000 --qprod 2 --output "concurrentQueue/34-10000000-21-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000000 --qops 1000000 --qprod 3 --output "concurrentQueue/34-10000000-31-out.txt"
./comptest --rep 10 --comp 2 --qtype 34 --qsize 10000000 --qops 1000000 --qprod 5 --output "concurrentQueue/34-10000000-51-out.txt"


