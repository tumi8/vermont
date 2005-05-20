/*
 This file is part of IPFIXLOLIB
 EXAMPLE CODE
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

/*
 Data we want to collect / transmit.
 NOTE: User-data must be in network byte order for interoperability
 */
typedef struct {
	uint32_t ip_src_addr;
	uint32_t ip_dst_addr;
	uint16_t src_port;
	uint16_t dst_port;
	uint64_t byte_count;
	uint64_t packet_count;
} meter_data;


/* generate constant data for testing / example purposes */
int meter_process_gimme_data(meter_data *mdat)
{
	mdat->ip_src_addr = htonl(0x01020304);	// 1.2.3.4
	mdat->ip_dst_addr = htonl(0x02040608);	// 2.4.6.8
	mdat->src_port = htons(12);
	mdat->dst_port = htons(13);
	//mdat->byte_count = htonll(1567490);
	//mdat->packet_count = htonll(42);

	return 0;
}


int main(int argc, char **argv)
{
	int ret =0;

	char *collector_ip;
	int collector_port;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <server_ip_address> <server_port>\nWill use UDP.\n", argv[0]);
		exit(1);
	}

	collector_ip=argv[1];
        collector_port=atoi(argv[2]);

        /*
	 Initialize an exporter

	 sourceID: the source ID the exported stream gets
	 exporter: an ipfix_exporter * to be initialized
	 */
	ipfix_exporter *my_exporter;
	ret=ipfix_init_exporter(MY_SOURCE_ID, &my_exporter);

	if (ret != 0) {
		fprintf(stderr, "ipfix_init_exporter failed!\n");
		exit(-1);
	}

	/*
	 Add a collector (the one who gets the data) to the exporting process

	 exporter: a previous init()ed exporter
	 coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
	 coll_port: port number of the collector
	 proto: transport protocol to use, TCP/UDP/SCTP

         You can add up to IPFIX_MAX_COLLECTORS collectors.
	 */
	ret = ipfix_add_collector(my_exporter, collector_ip, collector_port, UDP);
	printf("ipfix_add_collector returned %i\n", ret);

	/*
	 You have to define a template set with the ID 12345 before the exporting process starts

	 The template shall contain the following fields:
	 # |  IPFIX name of field      |  IPFIX field ID | length of associated datatype
	 -------------------------------------------------------------------------------
	 1 |  sourceAddressV4          |   8             | 4
	 2 |  destinationAddressV4     |   12            | 4
	 3 |  transportSourcePort      |   7             | 2
	 4 |  transportDestinationPort |   11            | 2
	 5 |  deltaOctetCount          |   1             | 8
	 6 |  deltaPacketCount         |   2             | 8

	 FIXME - ???
	 As none of these fields is vendor specific, the length of the template fields is 6*4 bytes.
         FIXME
	 */  
	uint16_t my_template_id = 12345;
	uint16_t my_n_template_id = htons(my_template_id);

	/*
	 Now start the adding of fields.

	 exporter: the exporter
	 template_id: an ID for this template
	 field_count: # of entries/fields
	 */
	ret=ipfix_start_template_set(my_exporter, my_template_id, 6);

	/*
	 Add fields to the exporter.

	 exporter: the exporter
	 template_id: the template ID chosen beforehand
	 length: sizeof() datatype
	 type: the IPFIX field ID for this entry
	 enterprise: FIXME ???
	 */
	ret=ipfix_put_template_field(my_exporter, my_template_id, 8, 4, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 12, 4, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 7, 2, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 11, 2, 0);
	//ret=ipfix_put_template_field(my_exporter, my_template_id, 1, 8, 0);
	//ret=ipfix_put_template_field(my_exporter, my_template_id, 2, 8, 0);

        /* Finalize the template */
	ret=ipfix_end_template_set(my_exporter, my_template_id);


        /*
	 Main exporting loop

	 What you basically do is

	 1) get data
	 2) start a data set
	 3) add the fields
         4) send data set
	 */
	int exporting = 1;

	while(exporting) {

		int i;
		meter_data my_meter_data;

                /* you may loop and add one or more data-sets */
		for (i = 0; i < 2; i++) {

                        /* get data - must be in Network Byte Order for interoperability */
			meter_process_gimme_data(&my_meter_data);

			/* start a data-set */
			ret=ipfix_start_data_set(my_exporter, &my_n_template_id);
			
			if (ret != 0 ) {
				// do not try use ipfix_put_data_field or  ipfix_put_end_field,
				// if  ret=ipfix_start_data_set has failed!

				fprintf(stderr, "ipfix_start_data_set failed!\n");	
			} else {
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n",
				       (*(*my_exporter).data_sendbuffer).current);

				/*
				  now fill the pre-defined data fields

				  NOTE: supplied data is NOT copied and has to
				  stay valid until the ipfix_send() below!
				*/
				ipfix_put_data_field(my_exporter, &(my_meter_data.ip_src_addr), 4);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);
				ipfix_put_data_field(my_exporter, &(my_meter_data.ip_dst_addr), 4);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data.src_port), 2);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data.dst_port), 2);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);
#if 0
				ipfix_put_data_field(my_exporter, &(my_meter_data.byte_count), 8);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data.packet_count), 8);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);
#endif
				/* FIXME ??? */
				/* Note: the user may specify a large buffer, as in the template example */
				/* The user MAY (probably should) add more records, before he calls send */
				/* FIXME ??? */

				/* finish the data-set */
				ret=ipfix_end_data_set(my_exporter);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				if (ret != 0)
					fprintf(stderr, "ipfix_end_data_set failed!\n");
			}
		}

		/*
		 send the data-set(s)
		 template sending is handled entirely by the library, too.

		 NOTE: ALL DATA added via ipfix_put_data_field has to
		 stay valid until ipfix_send() returns.
		 */
		ret=ipfix_send(my_exporter);
		if (ret != 0)
			fprintf(stderr, "ipfix_send failed!\n");

		exporting = 0;
	}

	/* if you no longer need the exporter: free resources */
	ret=ipfix_remove_collector(my_exporter, collector_ip, collector_port);
	ipfix_deinit_exporter(my_exporter);

	printf("bravo\n");
	exit(0);
}
