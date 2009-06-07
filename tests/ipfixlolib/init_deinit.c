/*
 This file is part of IPFIXLOLIB
 TEST CODE
 Published under GPL v2
 Ronny T. Lampert, 2005-01

 based upon the original IPFIXLOLIB
 by Jan Petranek, University of Tuebingen
 2004-11-18
 jan@petranek.de
 */

#include <stdio.h>
#include "ipfixlolib.h"

#define TEST_INIT_LOOP
#define TEST_ITERATIONS 1024
#define MY_SOURCE_ID 70538

int main(int argc, char *argv[])
{
	int ret;
	int i;
	ipfix_exporter *my_exporter;

#ifdef TEST_INIT_LOOP
	for (i = 0; i < TEST_ITERATIONS; i++) {
#endif
		ret = ipfix_init_exporter(MY_SOURCE_ID, &my_exporter);
		if (ret != 0) {
			fprintf(stderr, "ipfix_init_exporter failed!\n");
			exit(-1);
		}

		ipfix_deinit_exporter(my_exporter);

#ifdef TEST_INIT_LOOP
	}
#endif
	printf("bravo\n");
	exit(0);
}
