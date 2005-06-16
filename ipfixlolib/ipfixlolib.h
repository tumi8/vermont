#ifndef IPFIXLOLIB_H
#define IPFIXLOLIB_H
/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX

 Changes by Christoph Sommer, 2005-04-14
   Modified ipfixlolib-nothread Rev. 80
   Added support for DataTemplates (SetID 4)

 Changes by Ronny T. Lampert, 2005-01

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "encoding.h"
#include "ipfix_names.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * version number of the ipfix-protocol
 */
#define IPFIX_VERSION_NUMBER 0x000a

/*
 * amount of iovec, the header consumes
 * change only, if you change the internal code!
 */
#define HEADER_USED_IOVEC_COUNT 1

/*
 * length of a header in bytes
 */
#define IPFIX_HEADER_LENGTH 16

/*
 * maximum number of collectors at a time.
 * can be specified by user
 */
#define IPFIX_MAX_COLLECTORS 16

/*
 * maximum number of templates at a time
 * can be specified by user
 */
#define IPFIX_MAX_TEMPLATES 16

/*
 * Default time, until templates are re-sent again:
 * 30 seconds:
 * can be specified by user
 */
#define IPFIX_DEFAULT_TEMPLATE_TIMER 30

#define TRUE 1
#define FALSE 0

/*
 * length of a set header, i.e. the set ID and the length field
 * i.e. 4 bytes
 */
#define IPFIX_MAX_SET_HEADER_LENGTH 4

/*
 * maximum size of a sendbuffer
 * TODO: This value is delibaretely chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SENDBUFSIZE 4048

/*
 * This macro appends data to the sendbuffer. If the sendbuffer is too small,
 * it will print an error message and set errno to -1.
 */

/* #define ipfix_add_data2sendbuffer(SENDBUF, POINTER, LENGTH) { \ */
/*   if ((*SENDBUF).current >= (*SENDBUF).length-2 ) { \ */
/*     fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*SENDBUF).current ); \ */
/*     errno = -1; \ */
/*   } \ */
/*   ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_base = POINTER; \ */
/*   ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_len =  LENGTH; \ */
/*   (*SENDBUF).current++; \ */
/* }   */

#define ipfix_put_field2sendbuffer(SENDBUF, POINTER, LENGTH) { \
  if ((*SENDBUF).current >= (*SENDBUF).length-2 ) { \
    fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*SENDBUF).current ); \
    errno = -1; \
  } \
  ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_base = POINTER; \
  ((*SENDBUF).entries[ (*SENDBUF).current ]).iov_len =  LENGTH; \
  (*SENDBUF).current++; \
  (*(*SENDBUF).set_manager).data_length+= LENGTH; \
}

/* BUGFIX: After the makro found an error condition, it skips accessing data. */
#define ipfix_put_data_field(EXPORTER, POINTER, LENGTH) {		\
		if ((*(*EXPORTER).data_sendbuffer).current >= (*(*EXPORTER).data_sendbuffer).length ) { \
			fprintf (stderr, "Error: Sendbuffer too small to handle %i entries!\n", (*(*EXPORTER).data_sendbuffer).current ); \
			errno = -1;					\
		} else {						\
			((*(*EXPORTER).data_sendbuffer).entries[ (*(*EXPORTER).data_sendbuffer).current ]).iov_base = POINTER; \
			((*(*EXPORTER).data_sendbuffer).entries[ (*(*EXPORTER).data_sendbuffer).current ]).iov_len =  LENGTH; \
			(*(*EXPORTER).data_sendbuffer).current++;	\
			(*(*(*EXPORTER).data_sendbuffer).set_manager).data_length+= LENGTH; \
		} \
}

/* Struct containing an ipfix-header */
/*     Header Format (see draft-ietf-ipfix-protocol-03.txt) */

/*     0                   1                   2                   3 */
/*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 */

/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |       Version Number          |            Length             | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |                           Export Time                         | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |                       Sequence Number                         | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    |                          Source ID                            | */
/*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
/*    Message Header Field Descriptions */

/*    Version */
/*            Version of Flow Record format exported in this message. The */
/*            value of this field is 0x000a for the current version. */

/*    Length */
/*            Total Length is the length of the IPFIX message, measured in */
/*            octets, including message Header and FlowSet(s). */

/*    Export Time */
/*            Time in seconds since 0000 UTC 1970, at which the Export */
/*            Packet leaves the Exporter. */

/*    Sequence Number */
/*            Incremental sequence counter of all IPFIX Messages sent from */
/*            the current Observation Domain by the Exporting Process. */
/*            This value MUST SHOULD be used by the Collecting Process to */
/*            identify whether any IPFIX Messages have been missed. */

/*    Source ID */
/*            A 32-bit value that identifies the Exporter Process */
/*            Observation Domain. Collecting Process SHOULD use the */
/*            combination of the source IP address and the Source ID field */
/*            to separate different export streams originating from the */
/*            same Exporting Process. */

typedef struct {
	uint16_t version;
	uint16_t length;
	uint32_t export_time;
	uint32_t sequence_number;
	uint32_t source_id;
} ipfix_header;


enum ipfix_transport_protocol {UDP, TCP, SCTP};

/*
 * These indicate, if a field is commited (i.e. can be used)
 * unused or unclean (i.e. data is not complete yet)
 *
 */
enum ipfix_validity {UNUSED, UNCLEAN, COMMITED};

/*
 * Manages a record set
 * stores the record set header
 *
 */
typedef struct{
	struct iovec *header_iovec;

	/* buffer to store the header */
	char *set_header_store;

	/* total capacity of the header */
	int set_header_capacity;

	/* used length of the header store */
	int set_header_length;

	int data_length;
        /* do we need this?? */
	/* uint16_t* set_id;
         */
} ipfix_set_manager;


/*
 * A struct containing an array lot of io_vec
 * plus the index of the current (= last occupied) and maximum position.
 * Also has a buffer to store a header.
 * Note: The buffer is placed here, so it can be allocated once with the sendbuffer
 */
typedef struct {
	struct iovec *entries; /* an array of iovec structs, containing data and length */
	int current; /* last accessed field */
	int commited; /* last commited field (i.e. end data set has been called) */
	int length; /* the length of the sendbuffer */
	char *header_store; /* memory to store the header */
	int commited_data_length; /* length of the contained data (in bytes) */
	//  int uncommited_data_length; /* length of data not yet commited */
	ipfix_set_manager *set_manager;
} ipfix_sendbuffer;


/*
 * A collector receiving messages from this exporter
 */
typedef struct {
	int valid; // indicates, wheter this collector is valid. .
	char ipv4address[16];
	int port_number;
	enum ipfix_transport_protocol protocol;
	// warning! To use SCTP, we will need several ports!
	int data_socket; // socket data is sent to
	int template_socket; // socket, templates are sent to
} ipfix_receiving_collector;

/*
 * A template, mostly in binary form
 */
typedef struct{
	enum ipfix_validity valid; // indicates, wheter this template is valid.
	uint16_t template_id;
	uint16_t field_count;
	int fields_length;
	int max_fields_length;
	char *template_fields;
} ipfix_lo_template;

/*
 * Each exporting process is associated with a sequence number and a source ID
 * The exporting process keeps track of the sequence number.
 */
typedef struct {
	uint32_t sequence_number;
	uint32_t source_id;
	ipfix_sendbuffer *template_sendbuffer;
	ipfix_sendbuffer *data_sendbuffer;
	int collector_num; // number of currently listening collectors
	int collector_max_num; // maximum available collector
	ipfix_receiving_collector *collector_arr; // array of collectors

	// we also need some timer / counter to indicate,
	// if we should send the templates too.
	uint32_t last_template_transmission_time;
	// time, after templates are transmitted again
	uint32_t template_transmission_timer;

	int ipfix_lo_template_maxsize;
	int ipfix_lo_template_current_count;
	ipfix_lo_template *template_arr;

} ipfix_exporter;


/* generated by genproto */
int ipfix_init_exporter(uint32_t source_id, ipfix_exporter **exporter);
int ipfix_deinit_exporter(ipfix_exporter *exporter);

int ipfix_add_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port, enum ipfix_transport_protocol proto);
int ipfix_remove_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port);

int ipfix_start_template_set(ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count);
int ipfix_start_optionstemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length);
int ipfix_start_datatemplate_set(ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count, uint16_t fixedfield_count);
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_end_template_set(ipfix_exporter *exporter, uint16_t template_id );
/* gerhard: use ipfix_remove_template_set
int ipfix_remove_template(ipfix_exporter *exporter, uint16_t template_id);
*/
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t *template_id);
int ipfix_end_data_set(ipfix_exporter *exporter);
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length);
int ipfix_deinit_template_set(ipfix_exporter *exporter, ipfix_lo_template* templ);
int ipfix_remove_template_set(ipfix_exporter *exporter, uint16_t template_id);

int ipfix_send(ipfix_exporter *exporter);

int ipfix_enterprise_flag_set(uint16_t id);


#ifdef __cplusplus
}
#endif

#endif
