#!/bin/bash

while `true`; do
	xsltproc sensor_output.xml ../sensor_output > sensor_output.html
	cat ../sensor_output | perl gen_dot.pl | dot -Tpng -o modules.png
	sleep 1
done
