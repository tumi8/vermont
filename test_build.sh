#!/usr/bin/env bash

job_numbers=2
if [ "$1" ]
then
	job_numbers=$1
fi

(./configure --enable-debug --enable-db --enable-pcapmmap&& make clean && make -j $job_numbers) || exit 1
(./configure --enable-debug --enable-db && make clean && make -j $job_numbers) || exit 1
(./configure --enable-db && make clean && make -j $job_numbers) || exit 1
(./configure --enable-debug && make clean && make -j $job_numbers) || exit 1
(./configure --enable-pcapmmap&& make clean && make -j $job_numbers) || exit 1
(./configure && make clean && make -j $job_numbers) || exit 1
