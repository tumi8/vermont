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

#define MY_SOURCE_ID 70538
#define TEST_INIT_LOOP 1
#define TEST_TEMPLATE_LOOP 1
#define TEST_ITERATIONS 1024

int main(int argc, char *argv[])
{

	int ret;
	int i;
	ipfix_exporter *my_exporter;

#ifdef TEST_INIT_LOOP
	for (i = 0; i < TEST_ITERATIONS; i++) {
		int j;
#endif
		ret = ipfix_init_exporter(MY_SOURCE_ID, &my_exporter);
		if (ret != 0) {
			fprintf(stderr, "ipfix_init_exporter failed!\n");
			exit(-1);
		}

		ret=ipfix_add_collector(my_exporter, "127.0.0.1", 4711, UDP);
		if (ret != 0) {
			fprintf(stderr, "ipfix_add_collector failed!\n");
			exit(-1);
		}

#ifdef TEST_TEMPLATE_LOOP
		for (j = 0; j < TEST_ITERATIONS; j++) {
#endif
			uint16_t my_template_id=707;
                        ret=0;

			ret|=ipfix_start_template_set(my_exporter, my_template_id, 1);
			ret|=ipfix_put_template_field(my_exporter, my_template_id, 2, 8, 0);
			ret|=ipfix_end_template_set(my_exporter, my_template_id);

			if (ret != 0) {
				fprintf(stderr, "ipfix_add_collector failed!\n");
				exit(-1);
			}
#ifdef TEST_TEMPLATE_LOOP
		}
#endif
		ret=ipfix_send(my_exporter);
		if (ret != 0) {
			fprintf(stderr, "ipfix_send failed!\n");
			exit(-1);
		}

		ret=ipfix_remove_collector(my_exporter, "127.0.0.1", 4711);
		if (ret != 0) {
			fprintf(stderr, "ipfix_remove_collector failed!\n");
			exit(-1);
		}

		ipfix_deinit_exporter(my_exporter);

#ifdef TEST_INIT_LOOP
	}
#endif
	printf("bravo\n");
	exit(0);
}
