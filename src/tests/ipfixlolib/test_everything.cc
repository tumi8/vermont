/*
 This file is part of IPFIXLOLIB
 TEST CODE
 Published under GPL v2
 Alex Melnik, 2007
 */

#include <stdio.h>
#include <assert.h>
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/msg.h"

#define MY_SOURCE_ID 70538
#define TEST_INIT_LOOP 1
#define TEST_TEMPLATE_LOOP 1
#define TEST_ITERATIONS 10


int print_usage(void){
	
	printf("How To Use Tests:\n\n");
	printf("\t--- Just what you are reading: \th \n");
#ifdef SUPPORT_SCTP
	printf("\t--- Create SCTP collector: \tc \n");
#endif
	printf("\t--- Create UDP collector: \tu \n");
	printf("\t--- Template creation: \t\tt \n");
	printf("\t--- Template creation with custom ID: \tT 777 \n");
	printf("\t--- Delete template with ID: \td 777 \n");
	printf("\t--- Create (empty) data record assigned to custom template ID: \tr 777 \n");
	printf("\t--- Sending: \t\t\ts \n");
	printf("\t--- Quit and deinit:\t\tq \n");
	
	return 0;
}

void ready(void){
	printf(":");	
}

int main(int argc, char *argv[])
{

	int ret;
#ifdef SUPPORT_SCTP
	int sctp_exists = 0;
#endif
	ipfix_exporter *my_exporter;

	ret = ipfix_init_exporter(IPFIX_PROTOCOL, MY_SOURCE_ID, &my_exporter);
	if (ret != 0) {
		fprintf(stderr, "ipfix_init_exporter failed!\n");
		exit(-1);
	}	
	
	ipfix_set_sctp_lifetime(my_exporter, 5);
	printf("SCTP datalifetime set to %d\n", my_exporter->sctp_lifetime);
	
	ready();
	
	
	
	int c;
	
	while ((c = getchar()) != 'q') {
		uint16_t my_template_id = 0;
		uint create_id = 0;
		uint delete_id = 0;
		uint16_t my_n_template_id;
		switch (c) {
		case 'T':
			create_id = 0;
			//Test custom templates
			printf("Start testing Template creation!\n");
			assert(scanf("%u",&create_id)==1);
			
			ret=0;
		
			ret|=ipfix_start_template(my_exporter, create_id, 1);
			ret|=ipfix_put_template_field(my_exporter, create_id, 2, 8, 0);
			ret|=ipfix_end_template(my_exporter, create_id);
		
			if (ret != 0) {
				fprintf(stderr, "create template failed!\n");
				exit(-1);
			}
			printf("template created with ID: %u\n", create_id);
			break;
		case 't':
			//Test templates
			printf("Start testing Template creation!\n");
			my_template_id=4711;
			ret=0;
		
			ret|=ipfix_start_template(my_exporter, my_template_id, 1);
			ret|=ipfix_put_template_field(my_exporter, my_template_id, 2, 8, 0);
			ret|=ipfix_end_template(my_exporter, my_template_id);
		
			if (ret != 0) {
				fprintf(stderr, "create template failed!\n");
				exit(-1);
			}
			printf("template created with ID: %d\n", my_template_id);
			break;
		case 's':
			//Test sending
			printf("Start sending!\n");
			
			ret=ipfix_send(my_exporter);
			if (ret != 0) {
				fprintf(stderr, "ipfix_send failed!\n");
				exit(-1);
			}
			break;
#ifdef SUPPORT_SCTP
		case 'c':
			// add SCTP collector
			ret=ipfix_add_collector(my_exporter, "127.0.0.1", 1500, SCTP, NULL);
			
			if (ret != 0) {
				fprintf(stderr, "ipfix_add_collector failed!\n");
				exit(-1);
			}
			sctp_exists = 1;
			break;
#endif
		case 'u':
			// add UDP collector
			ipfix_aux_config_udp aux_config;
			aux_config.mtu = 1500;
			ret=ipfix_add_collector(my_exporter, "127.0.0.1", 4711, UDP, &aux_config);
			if (ret != 0) {
				fprintf(stderr, "ipfix_add_collector failed!\n");
				exit(-1);
			}
			break;
		case 'd':
			//delete template
			assert(scanf("%u",&delete_id)==1);
			printf("Start testing Template destruction ID : %d!\n", delete_id);
			
			ret=ipfix_remove_template(my_exporter, delete_id);
			
			if (ret != 0) {
				fprintf(stderr, "ipfix_remove_template failed!\n");
			}
			break;
		case 'r':
			//Send an empty datarecord 
			// implemented only for testing if data is assighned to the corresponding templates correctly
			create_id = 0;
			assert(scanf("%u",&create_id)==1);
			//create and send datarecord
			my_n_template_id = htons(create_id);
			ret = ipfix_start_data_set(my_exporter, my_n_template_id);
			if (ret != 0) {
				fprintf(stderr, "ipfix_start_data_set failed!\n");
				exit(-1);
			}
			ret = ipfix_end_data_set(my_exporter, 1);
			if (ret != 0) {
				fprintf(stderr, "ipfix_end_data_set failed!\n");
				exit(-1);
			}
			ret=ipfix_send(my_exporter);
			if (ret != 0) {
				fprintf(stderr, "ipfix_send failed!\n");
				exit(-1);
			}
			printf("datarecord send with ID: %d\n", create_id);
			break;
		case 10:
			ready();
			break;
		case 'h':
			print_usage();
			break;
		default:
			print_usage();
		}
	}


	//Cleaning
	printf("Cleaning up:\n");
/*
	printf("Remove Collector!\n");
	
	ret=ipfix_remove_collector(my_exporter, "127.0.0.1", 4711);
	if (ret != 0) {
		fprintf(stderr, "ipfix_remove_collector failed!\n");
		exit(-1);
	}
*/
#ifdef SUPPORT_SCTP
	if(sctp_exists){
		
		printf("Remove  SCTP Collector!\n");
		ret=ipfix_remove_collector(my_exporter, "127.0.0.1", 1500);
		if (ret != 0) {
			fprintf(stderr, "ipfix_remove_sctp_collector failed!\n");
			exit(-1);
		}
	}
#endif
	printf("deinit exporter!\n");
	
	ipfix_deinit_exporter(&my_exporter);



	printf("Done, Have a nice day!\n");
	exit(0);
}
