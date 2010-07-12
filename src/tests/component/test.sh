#!/bin/bash
./comptest -r 5 -o q1_1.txt -q 1 --qops 10000000 --qsize 1
./comptest -r 5 -o q2_1.txt -q 2 --qops 10000000 --qsize 1
./comptest -r 5 -o q3_1.txt -q 3 --qops 10000000 --qsize 1
./comptest -r 5 -o q4_1.txt -q 4 --qops 10000000 --qsize 1
./comptest -r 5 -o q5_1.txt -q 5 --qops 10000000 --qsize 1

./comptest -r 5 -o q1_2.txt -q 1 --qops 100000000 --qsize 100
./comptest -r 5 -o q2_2.txt -q 2 --qops 100000000 --qsize 100
./comptest -r 5 -o q3_2.txt -q 3 --qops 100000000 --qsize 100
./comptest -r 5 -o q4_2.txt -q 4 --qops 100000000 --qsize 100
./comptest -r 5 -o q5_2.txt -q 5 --qops 100000000 --qsize 100

./comptest -r 5 -o q1_3.txt -q 1 --qops 100000000 --qsize 10000
./comptest -r 5 -o q2_3.txt -q 2 --qops 100000000 --qsize 10000
./comptest -r 5 -o q3_3.txt -q 3 --qops 100000000 --qsize 10000
./comptest -r 5 -o q4_3.txt -q 4 --qops 100000000 --qsize 10000
./comptest -r 5 -o q5_3.txt -q 5 --qops 100000000 --qsize 10000

./comptest -r 5 -o q1_4.txt -q 1 --qops 100000000 --qsize 100000000
./comptest -r 5 -o q2_4.txt -q 2 --qops 100000000 --qsize 100000000
./comptest -r 5 -o q3_4.txt -q 3 --qops 100000000 --qsize 100000000
./comptest -r 5 -o q4_4.txt -q 4 --qops 100000000 --qsize 100000000
./comptest -r 5 -o q5_4.txt -q 5 --qops 100000000 --qsize 100000000

./comptest -r 5 -o q1_5.txt -q 1 --qops 10000000 --qsize 10000 --qprod 2
./comptest -r 5 -o q3_5.txt -q 3 --qops 10000000 --qsize 10000 --qprod 2
./comptest -r 5 -o q1_7.txt -q 1 --qops 10000000 --qsize 10000 --qprod 5
./comptest -r 5 -o q3_7.txt -q 3 --qops 10000000 --qsize 10000 --qprod 5
