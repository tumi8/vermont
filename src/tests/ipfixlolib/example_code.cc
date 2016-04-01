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
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/encoding.h" // because we need htonll()
#include "common/msg.h"

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
	mdat->byte_count = htonll(1567490);
	mdat->packet_count = htonll(42);

	return 0;
}

/* generate some other data */
int meter_process_gimme_data2(meter_data *mdat)
{
	mdat->ip_src_addr = htonl(0x01020304);	// 1.2.3.4
	mdat->ip_dst_addr = htonl(0x02040608);	// 2.4.6.8
	mdat->src_port = htons(22);
	mdat->dst_port = htons(4713);
	mdat->byte_count = htonll(3);
	mdat->packet_count = htonll(2);

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
	ret=ipfix_init_exporter(IPFIX_PROTOCOL, MY_SOURCE_ID, &my_exporter);

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
	 *aux_config: protocol dependent parameters

         You can add up to IPFIX_MAX_COLLECTORS collectors.
	 */
	ipfix_aux_config_udp aux_config;
	aux_config.mtu = 1500;
	ret = ipfix_add_collector(my_exporter, collector_ip, collector_port, UDP, &aux_config);
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
	ret=ipfix_start_template(my_exporter, my_template_id, 6);

	/*
	 Add fields to the exporter.

	 exporter: the exporter
	 template_id: the template ID chosen beforehand
	 type: the IPFIX field ID for this entry
	 length: sizeof() datatype
	 enterprise: FIXME ???
	 */
	ret=ipfix_put_template_field(my_exporter, my_template_id, 8, 4, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 12, 4, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 7, 2, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 11, 2, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 1, 8, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id, 2, 8, 0);

        /* Finalize the template */
	ret=ipfix_end_template(my_exporter, my_template_id);


	/* Add another template */
	uint16_t my_template_id2 = 6789;
	uint16_t my_n_template_id2 = htons(my_template_id2);

	/*
	 Now start the adding of fields.

	 exporter: the exporter
	 template_id: an ID for this template
	 field_count: # of entries/fields
	 */
	ret=ipfix_start_template(my_exporter, my_template_id2, 4);

	/*
	 Add fields to the exporter.

	 exporter: the exporter
	 template_id: the template ID chosen beforehand
	 type: the IPFIX field ID for this entry
	 length: sizeof() datatype
	 enterprise: FIXME ???
	 */
	ret=ipfix_put_template_field(my_exporter, my_template_id2, 8, 4, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id2, 12, 4, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id2, 7, 2, 0);
	ret=ipfix_put_template_field(my_exporter, my_template_id2, 11, 2, 0);

        /* Finalize the template */
	ret=ipfix_end_template(my_exporter, my_template_id2);

        /*
	 Main exporting loop

	 What you basically do is

	 1) get data
	 2) start a data set
	 3) add the fields
         4) send data set
	 */
	
	/* let's specify the number of data packets */
	int exporting = 2;

	while(exporting) {

		int i;
		meter_data my_meter_data[5];

                /* you may loop and add one or more data-sets */
		for (i = 0; i < 2; i++) {

			/* start a data-set */
			ret=ipfix_start_data_set(my_exporter, my_n_template_id);
			
			if (ret != 0 ) {
				// do not try use ipfix_put_data_field or  ipfix_put_end_field,
				// if  ret=ipfix_start_data_set has failed!

				fprintf(stderr, "ipfix_start_data_set failed!\n");	
			} else {
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n",
				       (*(*my_exporter).data_sendbuffer).current);

				/* get data - must be in Network Byte Order for interoperability */
				meter_process_gimme_data(&my_meter_data[i*2]);

				/*
				  now fill the pre-defined data fields

				  NOTE: supplied data is NOT copied and has to
				  stay valid until the ipfix_send() below!

				  NOTE: It's the user's responsability to ensure that
				  the added data is conform to the indicated template.
				*/
				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].ip_src_addr), 4);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].ip_dst_addr), 4);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].src_port), 2);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].dst_port), 2);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].byte_count), 8);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				ipfix_put_data_field(my_exporter, &(my_meter_data[i*2].packet_count), 8);
				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				/* more than one record can be added to a data set, so let's add another one */
				/* NOTE: we need another my_meter_data here, because the data of my_meter_data 
				   has to remain valid till ipfix_send() is called */
				meter_process_gimme_data2(&my_meter_data[i*2+1]);

				/* We can write a complete record in one go because the meter_data struct corresponds to the template.
				 * This possibility is handy and increases performance.
				 */
				ipfix_put_data_field(my_exporter, &my_meter_data[i*2+1], sizeof(meter_data));

				/* finish the data-set 
				   remarks: 
					- the main task of ipfix_end_data_set is to calculate the length of the data set 
					- the numbers of records must be given to enable the calculation of the next sequence number
				*/
				ret=ipfix_end_data_set(my_exporter, 2);
//				printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

				if (ret != 0)
					fprintf(stderr, "ipfix_end_data_set failed!\n");
			}
		}

		/* start a data-set for second template*/
		ret=ipfix_start_data_set(my_exporter, my_n_template_id2);

		if (ret != 0 ) {
		    // do not try use ipfix_put_data_field or  ipfix_put_end_field,
		    // if  ret=ipfix_start_data_set has failed!

		    fprintf(stderr, "ipfix_start_data_set failed!\n");	
		} else {
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n",
			    (*(*my_exporter).data_sendbuffer).current);

		    /* get data - must be in Network Byte Order for interoperability */
		    meter_process_gimme_data(&my_meter_data[4]);

		    /*
		       now fill the pre-defined data fields
		     */
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].ip_src_addr), 4);
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

		    /* We changed our mind and want to stop, so call ipfix_cancel_data_set 
		       instead of ipfix_end_data_set */
		    ret=ipfix_cancel_data_set(my_exporter);

		    if (ret != 0)
			fprintf(stderr, "ipfix_end_data_set failed!\n");
		}

		
		/* start again for second template*/
		ret=ipfix_start_data_set(my_exporter, my_n_template_id2);

		if (ret != 0 ) {
		    // do not try use ipfix_put_data_field or  ipfix_put_end_field,
		    // if  ret=ipfix_start_data_set has failed!

		    fprintf(stderr, "ipfix_start_data_set failed!\n");	
		} else {
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n",
			    (*(*my_exporter).data_sendbuffer).current);

		    /* get data - must be in Network Byte Order for interoperability */
		    meter_process_gimme_data(&my_meter_data[4]);

		    /*
		       now fill the pre-defined data fields
		     */
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].ip_src_addr), 4);
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].ip_dst_addr), 4);
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

		    /* Set marker in order to go back */
		    ipfix_set_data_field_marker(my_exporter);

		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].src_port), 2);
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);

		    /* Go back to the marker */
		    ipfix_delete_data_fields_upto_marker(my_exporter);

		    /* It was just a joke, so let's put the data field again */
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].src_port), 2);
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);
		    
		    ipfix_put_data_field(my_exporter, &(my_meter_data[4].dst_port), 2);
		    printf("main:   (*(*my_exporter).data_sendbuffer).current %i\n", my_exporter->data_sendbuffer->current);


		    /* finish the data-set 
remark: the main task of ipfix_end_data_set is to calculate the length of the data set */
		    ret=ipfix_end_data_set(my_exporter, 1);

		    if (ret != 0)
			fprintf(stderr, "ipfix_end_data_set failed!\n");
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

		exporting--;
	}

	/* if you no longer need the exporter: free resources */
	ret=ipfix_remove_collector(my_exporter, collector_ip, collector_port);
	ipfix_deinit_exporter(&my_exporter);

	printf("bravo\n");
	exit(0);
}
