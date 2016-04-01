/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
/*
 This file is part of IPFIXLOLIB
 EXAMPLE CODE
 Published under GPL v2
 Ronny T. Lampert, 2005-01
 Daniel Mentz, 2010-04

 based upon the original IPFIXLOLIB
 by Jan Petranek, University of Tuebingen
 2004-11-18
 jan@petranek.de
 */

#include <stdio.h>
#include <getopt.h>
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/encoding.h"
#include "common/msg.h"

#define MY_OBSERVATION_DOMAIN_ID 70538
#define DEFAULT_COLLECTOR_IP_ADDRESS "127.0.0.1"

struct config {
    const char *coll_ip4_addr;
    int coll_port;
    enum ipfix_transport_protocol transport_protocol;
    uint16_t mtu;
    const char *peer_fqdn;
    const char *certificate_chain_file;
    const char *private_key_file;
    const char *ca_file;
    const char *ca_path;
};

struct config myconfig = {
    .coll_ip4_addr = DEFAULT_COLLECTOR_IP_ADDRESS,
    .coll_port = 0,
    .transport_protocol = UDP,
    .mtu = 0,
    .peer_fqdn = 0,
    .certificate_chain_file = 0,
    .private_key_file = 0,
    .ca_file = 0,
    .ca_path = 0
};

/*
 Data we want to transmit.
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

meter_data my_meter_data[10000];
unsigned int my_meter_data_next_free = 0;


int parse_command_line_arguments(int argc, char **argv);
int add_collector(ipfix_exporter *exporter);
int get_sample_data1(meter_data *mdat);
int get_sample_data2(meter_data *mdat);
int set_config_on_exporter(ipfix_exporter *exporter);
void print_usage(char *argv0);


/*
 You can add up to IPFIX_MAX_COLLECTORS collectors.
*/
int add_collector(ipfix_exporter *exporter) {
    int ret;

    void *aux_config = NULL;
    ipfix_aux_config_udp acu = {
	.mtu = myconfig.mtu
    };
    ipfix_aux_config_dtls_over_udp acdou = {
	.udp = { .mtu = myconfig.mtu},
	.dtls = { .peer_fqdn = myconfig.peer_fqdn}
    };
    ipfix_aux_config_dtls_over_sctp acdos = {
	.dtls = { .peer_fqdn = myconfig.peer_fqdn}
    };
    if (myconfig.transport_protocol == UDP) {
	aux_config = &acu;
    } else if (myconfig.transport_protocol == DTLS_OVER_UDP) {
	aux_config = &acdou;
    } else if (myconfig.transport_protocol == DTLS_OVER_SCTP) {
	aux_config = &acdos;
    }
    /* The type of the last parameter to ipfix_add_collector() depends
     * on the transport protocol that has been chose. */
    if ((ret = ipfix_add_collector(exporter, myconfig.coll_ip4_addr,
		    myconfig.coll_port, myconfig.transport_protocol, aux_config))) {
	fprintf(stderr, "ipfix_add_collector() failed.\n");
	return -1;
    }
    printf("Added collector.\n");
    /* ipfix_add_collector() never blocks but returns immediately.  As a
     * consequence, it can only kick of the DTLS handshake, but it can't
     * wait until it completed (or failed).  ipfix_beat() has to be called
     * regularly to allow ipfixlolib to proceed with the DTLS handshake.
     * ipfix_beat() returns 0 if there are no more pending handshakes. */
    while(ipfix_beat(exporter))
	usleep(10000);
    return 0;

}

meter_data *alloc_meter_data(void) {
    if (my_meter_data_next_free >= sizeof(my_meter_data) / sizeof(my_meter_data[0])) {
	fprintf(stderr,"No more memory to store metering data.\n");
	abort();
    }
    return &my_meter_data[my_meter_data_next_free++];
}


/* generate constant data for testing / example purposes. It is important to
 * understand that all of this data has to be in *network byte order*.
 * ipfixlolib will not perform this transformation for you.  */
int get_sample_data1(meter_data *mdat) {
    mdat->ip_src_addr = htonl(0x01020304);	// 1.2.3.4
    mdat->ip_dst_addr = htonl(0x02040608);	// 2.4.6.8
    mdat->src_port = htons(12);
    mdat->dst_port = htons(13);
    mdat->byte_count = htonll(1567490);
    mdat->packet_count = htonll(42);

    return 0;
}

/* generate some other data. Same caveat applies as to get_sample_data1:
 *
 * The data has to be in network byte order. */
int get_sample_data2(meter_data *mdat) {
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

    if (parse_command_line_arguments(argc,argv)) {
	exit(EXIT_FAILURE);
    }

    /* Initialize the exporter.
     * MY_OBSERVATION_ID is the Observation ID that will be sent
     * with every IPFIX Message. */
    ipfix_exporter *my_exporter;
    ret=ipfix_init_exporter(IPFIX_PROTOCOL, MY_OBSERVATION_DOMAIN_ID, &my_exporter);
    if (set_config_on_exporter(my_exporter))
	exit(EXIT_FAILURE);

    if (ret) {
	fprintf(stderr, "ipfix_init_exporter failed!\n");
	exit(EXIT_FAILURE);
    }
    printf("Initialized exporter.\n");

    if (add_collector(my_exporter)) exit(EXIT_FAILURE);

    printf("++ Defining Templates ++.\n");
    /*
     * Prior to sending Data Records we have to define a Template that describes the 
     * encoding of these Data Records.
     * We choose 12345 as the Template ID.

     The template shall contain the following fields:
    # |  IPFIX name of field      |  IPFIX field ID | length of associated data type
    -------------------------------------------------------------------------------
    1 |  sourceAddressV4          |   8             | 4
    2 |  destinationAddressV4     |   12            | 4
    3 |  transportSourcePort      |   7             | 2
    4 |  transportDestinationPort |   11            | 2
    5 |  deltaOctetCount          |   1             | 8
    6 |  deltaPacketCount         |   2             | 8

    */  
    uint16_t my_template1_id = 12345;
    uint16_t my_n_template1_id = htons(my_template1_id); /* Template ID in network byte order */
    uint16_t my_template1_length = 4+4+2+2+8+8; /* cumulative length of all Template fields */

    /*
       Now start the adding of fields.

    exporter: the exporter
    template_id: an ID for this template
    field_count: # of entries/fields
    */
    printf("Starting template with id %d.\n",my_template1_id);
    ret = ipfix_start_template(my_exporter, my_template1_id, 6);

    /*
       Add fields to the exporter.

    exporter: the exporter
    template_id: the template ID chosen beforehand
    type: the IPFIX field ID for this entry
    length: sizeof() data type
    enterprise: FIXME ???
    */
    printf("Putting data fields.\n");
    ret = ipfix_put_template_field(my_exporter, my_template1_id, 8,  4, 0);
    ret = ipfix_put_template_field(my_exporter, my_template1_id, 12, 4, 0);
    ret = ipfix_put_template_field(my_exporter, my_template1_id, 7,  2, 0);
    ret = ipfix_put_template_field(my_exporter, my_template1_id, 11, 2, 0);
    ret = ipfix_put_template_field(my_exporter, my_template1_id, 1,  8, 0);
    ret = ipfix_put_template_field(my_exporter, my_template1_id, 2,  8, 0);

    /* Finalize the template */
    printf("Ending template.\n");
    ret = ipfix_end_template(my_exporter, my_template1_id);


    /*
     * We decide to define another Template with ID 6789.
     The template shall contain the following fields:
    # |  IPFIX name of field      |  IPFIX field ID | length of associated data type
    -------------------------------------------------------------------------------
    1 |  sourceAddressV4          |   8             | 4
    2 |  destinationAddressV4     |   12            | 4
    3 |  transportSourcePort      |   7             | 2
    4 |  transportDestinationPort |   11            | 2
    */

    uint16_t my_template2_id = 6789;
    uint16_t my_n_template2_id = htons(my_template2_id); /* Same Template ID in network byte order */
    /* uint16_t my_template2_length = 4+4+2+2; */ /* cumulative length of all Template fields */

    /*
       Now start the adding of fields.

    exporter: the exporter
    template_id: an ID for this template
    field_count: # of entries/fields
    */
    printf("Starting template with id %d.\n",my_template2_id);
    ret = ipfix_start_template(my_exporter, my_template2_id, 4);

    /*
       Add fields to the exporter.

    exporter: the exporter
    template_id: the template ID chosen beforehand
    type: the IPFIX field ID for this entry
    length: sizeof() data type
    enterprise: FIXME ???
    */
    printf("Putting data fields.\n");
    ret = ipfix_put_template_field(my_exporter, my_template2_id, 8,  4, 0);
    ret = ipfix_put_template_field(my_exporter, my_template2_id, 12, 4, 0);
    ret = ipfix_put_template_field(my_exporter, my_template2_id, 7,  2, 0);
    ret = ipfix_put_template_field(my_exporter, my_template2_id, 11, 2, 0);

    /* Finalize the template */
    printf("Ending template.\n");
    ret = ipfix_end_template(my_exporter, my_template2_id);

    printf("++ Sending Data ++.\n");
    /*
       Main exporting loop

       What you basically do is

       1) get data
       2) start a data set
       3) add the fields
       4) finish data set
       5) send data
       */

    meter_data *my_md;
    int sp;
    int num_records = 0; /* Number of records in the current Data Set. We have
			    to pass this value when we call
			    ipfix_end_data_set().  If num_records == 0, then
			    there's no open Data Set. We have to call
			    ipfix_start_data_set() first in this situation. */
    int records_per_set = 50; /* We don't want to store more than this number
				 of records in one Data Set. */
    int total_records = 1000; /* Total number of records we want to send */
    int i;

    for(i=0;i<total_records;i++) {
	/* How many bytes still fit in the IPFIX Message? */
	sp = ipfix_get_remaining_space(my_exporter);
	printf("Remaining available space in IPFIX Message: %d\n", sp);

	/* We don't want to have more than records_per_set Records in one Data Set */
	if (num_records == records_per_set) {
	    printf("There are now %d Records in this Data Set. Let's close this and start a new one.\n",num_records);
	    printf("ipfix_end_data_set()\n");
	    ret = ipfix_end_data_set(my_exporter,num_records);
	    if (ret != 0)
		fprintf(stderr, "ipfix_end_data_set failed!\n");
	    /* Reset num_records to 0 */
	    num_records = 0;
	}
	/* There's an open Data Set (i.e. num_records > 0), but there's *not*
	 * enough space left to fit another Data Record. */
	if (num_records > 0 &&
		(sp=ipfix_get_remaining_space(my_exporter)) < my_template1_length) {
	    printf("Remaining space: %d < %d. Too small to fit another Data Record\n",
		    sp,my_template1_length);
	    /* Close Data Set */
	    printf("ipfix_end_data_set()\n");
	    ret = ipfix_end_data_set(my_exporter,num_records);
	    num_records = 0;
	    if (ret != 0)
		fprintf(stderr, "ipfix_end_data_set failed!\n");
	    /* Send off data. As a result, the send buffer will be empty again. */
	    printf("ipfix_send()\n");
	    ret = ipfix_send(my_exporter);
	    if (ret != 0)
		fprintf(stderr, "ipfix_send failed!\n");
	    /* Now, we can recycle the memory in which we stored the data. */
	    my_meter_data_next_free = 0;
	    /* Now, there should be plenty of space available. */
	    sp = ipfix_get_remaining_space(my_exporter);
	    printf("Remaining available space in IPFIX Message after ipfix_send(): %d\n", sp);
	}
	/* If there's no open Data Set */
	if (num_records == 0) {
	    /* Make sure that at least the set header plus
	     * one data record fits inside. */
	    sp = ipfix_get_remaining_space(my_exporter);
	    if (sp < IPFIX_OVERHEAD_PER_SET + my_template1_length) {
		/* If not, clear the send buffer by sending out the data that
		 * sits in the send buffer. */
		printf("Can't start data set. No more space in IPFIX Message. Space left %d\n",sp);
		printf("ipfix_send()\n");
		ret = ipfix_send(my_exporter);
		if (ret != 0)
		    fprintf(stderr, "ipfix_send failed!\n");
		/* Now, we can recycle the memory in which we stored the data. */
		my_meter_data_next_free = 0;
		/* Now we should have plenty of remaining space. */
		sp = ipfix_get_remaining_space(my_exporter);
		printf("Remaining available space in IPFIX Message after ipfix_send(): %d\n", sp);
		if (sp < IPFIX_OVERHEAD_PER_SET + my_template1_length) {
		    /* If the buffer is still too small so that not even
		     * a single Data Record fits in, then something is
		     * severely wrong */
		    printf("Can't start data set. No more space in IPFIX Message even though we just called ipfix_send()\n");
		    goto out;
		}
	    }
	    /* start a data-set.
	     * NOTE: The Template ID has to be passed in *network byte order*.
	     * This is in contrast to the corresponding parameter of
	     * ipfix_start_template()
	     * */
	    printf("ipfix_start_data_set()\n");
	    ret = ipfix_start_data_set(my_exporter, my_n_template1_id);
	    if (ret != 0 ) {
		// do not try use ipfix_put_data_field or  ipfix_put_end_field,
		// if  ret=ipfix_start_data_set has failed!
		fprintf(stderr, "ipfix_start_data_set failed!\n");	
		goto out;
	    }
	    /* Remaining space should now be 4 bytes less than before because the
	     * Data Set header is 4 bytes long. */
	    sp = ipfix_get_remaining_space(my_exporter);
	    printf("Remaining available space in IPFIX Message after ipfix_start_data_set(): %d\n", sp);
	}

	my_md = alloc_meter_data();
	/* get data - must be in Network Byte Order for interoperability */
	if (num_records % 2)
	    get_sample_data2(my_md);
	else
	    get_sample_data1(my_md);

	/*
	   now fill the pre-defined data fields

	NOTE: supplied data is NOT copied and has to
	stay valid until the ipfix_send() below!

	NOTE: It's the user's responsibility to ensure that
	the added data is conform to the indicated template.
	*/
	printf("Passing flow data to ipfixlolib.\n");
	ipfix_put_data_field(my_exporter, &my_md->ip_src_addr, 4);
	ipfix_put_data_field(my_exporter, &my_md->ip_dst_addr, 4);
	ipfix_put_data_field(my_exporter, &my_md->src_port, 2);
	ipfix_put_data_field(my_exporter, &my_md->dst_port, 2);
	ipfix_put_data_field(my_exporter, &my_md->byte_count, 8);
	ipfix_put_data_field(my_exporter, &my_md->packet_count, 8);

	num_records++;

    }
    printf("ipfix_end_data_set()\n");
    ret = ipfix_end_data_set(my_exporter,num_records);
    num_records = 0;
    if (ret != 0)
	fprintf(stderr, "ipfix_end_data_set failed!\n");
    printf("ipfix_send()\n");
    ret = ipfix_send(my_exporter);
    if (ret != 0)
	fprintf(stderr, "ipfix_send failed!\n");
    /* Now, we can recycle the memory in which we stored the data. */
    my_meter_data_next_free = 0;


    /* start a data-set for second template*/
    ret=ipfix_start_data_set(my_exporter, my_n_template2_id);

    if (ret != 0 ) {
	// do not try use ipfix_put_data_field or  ipfix_put_end_field,
	// if  ret=ipfix_start_data_set has failed!

	fprintf(stderr, "ipfix_start_data_set failed!\n");	
	goto out;
    }

    my_md = alloc_meter_data();
    /* get data - must be in Network Byte Order for interoperability */
    get_sample_data1(my_md);

    /*
       now fill the pre-defined data fields
    */
    ipfix_put_data_field(my_exporter, &my_md->ip_src_addr, 4);
    /* We changed our mind and want to stop, so call ipfix_cancel_data_set 
       instead of ipfix_end_data_set */
    ret=ipfix_cancel_data_set(my_exporter);

    if (ret != 0)
	fprintf(stderr, "ipfix_end_data_set failed!\n");


    /* start again for second template*/
    ret=ipfix_start_data_set(my_exporter, my_n_template2_id);

    if (ret != 0 ) {
	// do not try use ipfix_put_data_field or  ipfix_put_end_field,
	// if  ret=ipfix_start_data_set has failed!

	fprintf(stderr, "ipfix_start_data_set failed!\n");	
	goto out;
    }
    my_md = alloc_meter_data();
    /* get data - must be in Network Byte Order for interoperability */
    get_sample_data1(my_md);

    /*
       now fill the pre-defined data fields
       */
    ipfix_put_data_field(my_exporter, &my_md->ip_src_addr, 4);
    ipfix_put_data_field(my_exporter, &my_md->ip_dst_addr, 4);

    /* Set marker in order to go back */
    ipfix_set_data_field_marker(my_exporter);

    ipfix_put_data_field(my_exporter, &my_md->src_port, 2);

    /* Go back to the marker */
    ipfix_delete_data_fields_upto_marker(my_exporter);

    /* It was just a joke, so let's put the data field again */
    ipfix_put_data_field(my_exporter, &my_md->src_port, 2);
    ipfix_put_data_field(my_exporter, &my_md->dst_port, 2);


    /* finish the data set 
    remark: the main task of ipfix_end_data_set is to calculate the
    length of the data set */
    ret = ipfix_end_data_set(my_exporter,1);

    if (ret != 0)
	fprintf(stderr, "ipfix_end_data_set failed!\n");

    /*
    send the data-set(s) template sending is handled entirely by the
    library, too.

    NOTE: ALL DATA added via ipfix_put_data_field has to stay valid
    until ipfix_send() returns.
    */
    ret = ipfix_send(my_exporter);
    if (ret != 0)
	fprintf(stderr, "ipfix_send failed!\n");
    /* Now, we can recycle the memory in which we stored the data. */
    my_meter_data_next_free = 0;
out:

    /* if you no longer need the exporter: free resources */
    ret=ipfix_remove_collector(my_exporter, myconfig.coll_ip4_addr, myconfig.coll_port);
    ipfix_deinit_exporter(my_exporter);

    printf("Done.\n");
    exit(EXIT_SUCCESS);
}

int parse_command_line_arguments(int argc, char **argv) {
    int transport = UDP;
    int debug_level = MSG_ERROR;
    enum opts { mtu=1, peer_fqdn,cert,key,CAfile,CApath,collector,port };
    struct option long_options[] = {
	{"udp", no_argument, &transport, UDP},
	{"sctp", no_argument, &transport, SCTP},
	{"dtls_over_udp", no_argument, &transport, DTLS_OVER_UDP},
	{"dtls_over_sctp", no_argument, &transport, DTLS_OVER_SCTP},
	{"mtu",required_argument,0,mtu},
	{"peer_fqdn",required_argument,0,peer_fqdn},
	{"cert",required_argument,0,cert},
	{"key",required_argument,0,key},
	{"CAfile",required_argument,0,CAfile},
	{"CApath",required_argument,0,CApath},
	{"collector",required_argument,0,collector},
	{"port",required_argument,0,port},
	{"help",no_argument,0,'h'},
	{0, 0, 0, 0}
    };
    while (1) {
	int c;
	int option_index = 0;
	long l;
	char *endptr;
	struct in_addr tmpaddr;

	c = getopt_long (argc, argv, "dh", long_options, &option_index);
	if (c == -1) break;
	switch (c) {
	    case 0:
		break;
	    case 'h':
		print_usage(argv[0]);
		exit(EXIT_SUCCESS);
	    case 'd':
		debug_level++;
		break;
	    case mtu:
		l = strtol(optarg,&endptr,0);
		if (*endptr != '\0' || l < 576 || l > UINT16_MAX) {
		    fprintf(stderr,"bad mtu value\n");
		    return -1;
		}
		myconfig.mtu = l;
		break;
	    case peer_fqdn:
		myconfig.peer_fqdn = optarg;
		break;
	    case cert:
		myconfig.certificate_chain_file = optarg;
		break;
	    case key:
		myconfig.private_key_file = optarg;
		break;
	    case CAfile:
		myconfig.ca_file = optarg;
		break;
	    case CApath:
		myconfig.ca_path = optarg;
		break;
	    case collector:
		if (inet_pton(AF_INET,optarg,&tmpaddr) != 1) {
		    fprintf(stderr,"bad IP address\n");
		    return -1;
		}
		myconfig.coll_ip4_addr = optarg;
		break;
	    case port:
		l = strtol(optarg,&endptr,0);
		if (*endptr != '\0' || l < 0 || l > UINT16_MAX) {
		    fprintf(stderr,"bad port number\n");
		    return -1;
		}
		myconfig.coll_port = l;
		break;
	}
    }
    if (optind != argc) {
	fprintf(stderr,"invalid arguments\n");
	return -1;
    }
    myconfig.transport_protocol = transport;
    if (myconfig.coll_port == 0) {
	if (myconfig.transport_protocol == DTLS_OVER_UDP ||
		myconfig.transport_protocol == DTLS_OVER_SCTP)
	    myconfig.coll_port = 4740; /* default port for secure connections */
	else
	    myconfig.coll_port = 4739; /* default port */
    }
    msg_setlevel(debug_level);

    return 0;
}

int set_config_on_exporter(ipfix_exporter *exporter) {
    int ret = 0;
    ret = ipfix_set_ca_locations(exporter, myconfig.ca_file, myconfig.ca_path);
    if (ret) return ret;
    if (myconfig.certificate_chain_file)
	ret = ipfix_set_dtls_certificate(exporter, myconfig.certificate_chain_file, myconfig.private_key_file);
    return ret;
}

void print_usage(char *argv0) {
    printf(
"Usage: %s [OPTIONS]\n"
"  --collector IPADDR  connect to collector at IP address IPADDR.\n"
"                      Default is 127.0.0.1\n"
"  --port P            use port P instead of standard port number\n"
"                      which is 4739 or 4740 if DTLS is used.\n"
"  --udp               use UDP as transport protocol (default)\n"
"  --sctp              use SCTP as transport protocol\n"
"  --dtls_over_udp     use DTLS over UDP as transport protocol\n"
"  --dtls_over_sctp    use DTLS over SCTP as transport protocol\n"
"  --mtu V             set maximum transmission unit (MTU) to V\n"
"The following options apply to DTLS only\n"
"  --peer_fqdn N       peer must present certificate which contains\n"
"                      the FQDN N.\n"
"  --cert F            X.509 certificate chain in PEM format is stored in F\n"
"  --key F             private key for certificate is stored in F\n"
"  --CAfile F          search F for CA certificates\n"
"  --CApath D          search directory D for CA certificates\n"
,
argv0
);
}
