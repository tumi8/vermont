
#ifndef IPFIXLOLIB_H
#define IPFIXLOLIB_H
/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX

 Changes by Gerhard Münz, 2006-02-01
   Changed and debugged sendbuffer structure and Co
   Added new function for canceling data sets and deleting fields

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
#ifdef SUPPORT_SCTP
#include <netinet/sctp.h>
#endif

//#include "encoding.h"
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
#define IPFIX_DEFAULT_TEMPLATE_TIMER 20
/*
 * Default time, until a new SCTP retransmission attempt
 * takes place
 * 5 minutes = 400 seconds
 * can be specified by user
*/
#define IPFIX_DEFAULT_SCTP_RECONNECT_TIMER 300
/*
 * Default reliability for sending IPFIX-data-records
 * 0 = reliable
 * can be specified by user
 */
#define IPFIX_DEFAULT_SCTP_DATA_LIFETIME 0

#define TRUE 1
#define FALSE 0

/*
 * maximum number of sets per IPFIX packet
 * TODO: This value is delibaretely chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SETS_PER_PACKET 4

/*
 * maximum size of a sendbuffer
 * TODO: This value is delibaretely chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SENDBUFSIZE 1024

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
/*
#define ipfix_put_field2sendbuffer(SENDBUF, POINTER, LENGTH) { \
  if ((SENDBUF->current + 1) >= IPFIX_MAX_SENDBUFSIZE ) { \
    msg(MSG_ERROR, "IPFIX: Sendbuffer too small to handle  %i entries!\n", SENDBUF->current ); \
    errno = -1; \
  } \
  (SENDBUF->entries[ SENDBUF->current ]).iov_base = POINTER; \
  (SENDBUF->entries[ SENDBUF->current ]).iov_len =  LENGTH; \
  SENDBUF->current++; \
  (SENDBUF->set_manager).data_length+= LENGTH; \
}
*/

/* BUGFIX: After the makro found an error condition, it skips accessing data. */
#define ipfix_put_data_field(EXPORTER, POINTER, LENGTH) { \
		if (EXPORTER->data_sendbuffer->current >= IPFIX_MAX_SENDBUFSIZE) { \
		    msg(MSG_ERROR, "IPFIX: Sendbuffer too small to handle  %i entries!\n", EXPORTER->data_sendbuffer->current ); \
			errno = -1;					\
		} else {						\
			(EXPORTER->data_sendbuffer->entries[ EXPORTER->data_sendbuffer->current ]).iov_base = POINTER; \
			(EXPORTER->data_sendbuffer->entries[ EXPORTER->data_sendbuffer->current ]).iov_len =  LENGTH; \
			EXPORTER->data_sendbuffer->current++;	\
			(EXPORTER->data_sendbuffer->set_manager).data_length+= LENGTH; \
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

/*  Set Header:

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |          Set ID               |          Length               |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

typedef struct {
    uint16_t set_id;
    uint16_t length;
} ipfix_set_header;


enum ipfix_transport_protocol {
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	RAWDIR,
#endif
	DATAFILE, SCTP, UDP, TCP
	};

/*
 * These indicate, if a field is commited (i.e. can be used)
 * unused or unclean (i.e. data is not complete yet)
 * T_SENT (Template was sent) and T_WiTHDRAWN (Template destroyed)
 * are used with SCTP, since Templates are sent only once
 * T_TOBEDELETED templates will be deleted the next time when the buffer is updated
 */
enum template_state {T_UNUSED, T_UNCLEAN, T_COMMITED, T_SENT, T_WITHDRAWN, T_TOBEDELETED};


/*
 * Indicates the state of the collector. After collector is added
 * state changes to C_NEW, successful call of the socket connect() function
 * sets the state to C_CONNECTED. If connection is lost and the socket closed
 * state changes to C_DISCONNECTED and reconnection attempts can take place
*/
enum collector_state {C_UNUSED, C_NEW, C_DISCONNECTED, C_CONNECTED};


/*
 * Manages a record set
 * stores the record set header
 *
 */
typedef struct{
	/* number of the current set */
	unsigned set_counter;

	/* variable that stores the position of the current set header
	   in ipfix_sendbuffer->entries */
	struct iovec *header_iovec;

	/* buffer to store set headers */
	ipfix_set_header set_header_store[IPFIX_MAX_SETS_PER_PACKET];

	/* set length = sum of field length */
	unsigned data_length;
} ipfix_set_manager;


/*
 * A struct buffering data of an ipfix packet
 */
typedef struct {
	struct iovec entries[IPFIX_MAX_SENDBUFSIZE]; /* an array of iovec structs, containing data and length */
	/* usage of entries:
	   - the first HEADER_USED_IOVEC_COUNT=1 entries are reserved for the ipfix header
	   - the remaining entries are used for
	     * set headers
	     * individual fields of the sets/records
	 */
	unsigned current; /* last accessed entry in entries */
	unsigned committed; /* last commited entry in entries, i.e. when end_data_set was called for the last time */
	unsigned marker; /* marker that allows to delete recently added entries */
	unsigned committed_data_length; /* length of the contained data (in bytes) */
	ipfix_header packet_header;
	//  int uncommited_data_length; /* length of data not yet commited */
	ipfix_set_manager set_manager;
} ipfix_sendbuffer;


/*
 * A collector receiving messages from this exporter
 */
typedef struct {
	char ipv4address[16];
	int port_number;
	enum ipfix_transport_protocol protocol;
	int data_socket; // socket data and templates are sent to
	struct sockaddr_in addr;
	uint32_t last_reconnect_attempt_time;
	enum collector_state state;
	char* file; /**< for protocol==FILE, this variable contains the destination file name */
	int fh; /**< for protocol==FILE, this variable contains the file handle */
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	char* packet_directory_path; /**< if protocol==RAWDIR: path to a directory to store packets in. Ignored otherwise. */
	int packets_written; /**< if protcol==RAWDIR: number of packets written to packet_directory_path. Ignored otherwise. */
#endif
} ipfix_receiving_collector;

/*
 * A template, mostly in binary form
 */
typedef struct{
	enum template_state state; // indicates, whether this template is valid.
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
	uint32_t sequence_number; // total number of data records
	uint32_t sn_increment; // to be added to sequence number before sending data records
	uint32_t source_id;
	ipfix_sendbuffer *template_sendbuffer;
	ipfix_sendbuffer *sctp_template_sendbuffer;
	ipfix_sendbuffer *data_sendbuffer;
	int collector_num; // number of currently listening collectors
	int collector_max_num; // maximum available collector
	ipfix_receiving_collector *collector_arr; // array of collectors

	// we also need some timer / counter to indicate,
	// if we should send the templates too.
	uint32_t last_template_transmission_time;

	// force template send next time packets are sent (to include new template ids)
	uint32_t force_template_send;

	// time, after templates are transmitted again
	uint32_t template_transmission_timer;
	// lifetime of an SCTP data packet
	uint32_t sctp_lifetime;
	// time, after new sctp reconnection will be initiated (default = 5 min)
	// (0 ==> no reconnection -> destroy collector)
	uint32_t sctp_reconnect_timer;
	int ipfix_lo_template_maxsize;
	int ipfix_lo_template_current_count;
	ipfix_lo_template *template_arr;

} ipfix_exporter;


/* generated by genproto */
int ipfix_init_exporter(uint32_t source_id, ipfix_exporter **exporter);
int ipfix_deinit_exporter(ipfix_exporter *exporter);

int ipfix_add_collector(ipfix_exporter *exporter, const char *coll_ip4_addr, int coll_port, enum ipfix_transport_protocol proto);
int ipfix_remove_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port);

int ipfix_start_template_set(ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count);
int ipfix_start_optionstemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length);
int ipfix_start_datatemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t preceding, uint16_t field_count, uint16_t fixedfield_count);
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_end_template_set(ipfix_exporter *exporter, uint16_t template_id );
/* gerhard: use ipfix_remove_template_set
int ipfix_remove_template(ipfix_exporter *exporter, uint16_t template_id);
*/
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t template_id);
int ipfix_end_data_set(ipfix_exporter *exporter, uint16_t number_of_records);
int ipfix_cancel_data_set(ipfix_exporter *exporter);
int ipfix_set_data_field_marker(ipfix_exporter *exporter);
int ipfix_delete_data_fields_upto_marker(ipfix_exporter *exporter);
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length);
int ipfix_deinit_template_set(ipfix_exporter *exporter, ipfix_lo_template* templ);
int ipfix_remove_template_set(ipfix_exporter *exporter, uint16_t template_id);
int ipfix_send(ipfix_exporter *exporter);
int ipfix_enterprise_flag_set(uint16_t id);
// Set up time after that Templates are going to be resent
int ipfix_set_template_transmission_timer(ipfix_exporter *exporter, uint32_t timer);
// Sets a packet lifetime for SCTP data packets (lifetime > 0 : unreliable packets)
int ipfix_set_sctp_lifetime(ipfix_exporter *exporter, uint32_t lifetime);
// Set up SCTP reconnect timer, time after that a reconnection attempt is made,
// if connection to the collector was lost.
int ipfix_set_sctp_reconnect_timer(ipfix_exporter *exporter, uint32_t timer);

#ifdef __cplusplus
}
#endif

#endif
