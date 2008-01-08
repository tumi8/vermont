#!/bin/bash

while `true`; do
	(
		flock -s 200
		xsltproc sensor_output.xml ../sensor_output > sensor_output.html
		cat ../sensor_output | perl gen_dot.pl | dot -Tpng -o modules.png
	) 200> ../sensor_output.lock
	sleep 1
done
