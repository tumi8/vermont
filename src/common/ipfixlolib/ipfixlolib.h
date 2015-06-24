/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */

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

/*! \mainpage ipfixlolib documentation

ipfixlolib is a library that implements an IPFIX Exporter as defined in RFC 5101. It supports the following transport protocols

- UDP
- SCTP
- DTLS over UDP
- DTLS over SCTP

See ipfixlolib.h for details on how to use this library.

 */

/*! \file ipfixlolib.h
    \brief Interface to ipfixlolib
    
    ipfixlolib implements an IPFIX Exporter. It supports the following
    protocols
     - UDP
     - SCTP
     - DTLS over UDP
     - DTLS over SCTP

    ipfixlolib should be used in the following way:

    - Applications start out by creating an exporter object using
    ipfix_init_exporter(). The exporter object is the abstraction for a common
    set of active Templates, a common send buffer, an observation domain id, a
    sequence number and further information. Most of the functions described
    below require a previously initialized exporter as the first argument.
    - Multiple Collectors can be added to the exporter by using
    ipfix_add_collector(). The transport protocol, the Collector's IP address
    and port as well as other configuration details will be passed to
    ipfix_add_collector(). ipfixlolib will start the connection setup procedure
    immediately which includes triggering the DTLS handshake if one of the DTLS
    variants has been chosen as the transport protocol.
    - ipfix_start_template(), ipfix_put_template_field() and
    ipfix_end_template() can be used at any time throughout the life cycle
    of an exporter to define new Templates. These Template definitions will not
    be sent immediately, but when ipfix_send() is called the next time. If UDP
    is used as the transport protocol, the library makes sure that active
    Templates are retransmitted on a regular basis as required by RFC 5101. It
    should be noted that the library supports only one Template per Template
    Set even though RFC 5101 allows multiple Templates per Set. Templates can
    only be defined on a per Exporter basis i.e. <em>all</em> Templates will be
    send to <em>all</em> Collectors. You cannot define a Template that is valid
    only for a specific Collector.
    - ipfix_remove_template_set() removes previously defined Templates that are
    no longer needed. A corresponding withdrawal message is sent to all
    Collectors using SCTP.
    - The functions ipfix_start_data_set(), ipfix_put_data_field(),
    ipfix_end_data_set() are used to append Data Sets to the send buffer. Again,
    there is only a single send buffer per exporter. As soon as ipfix_send() is
    called, all Data Sets will be sent to all Collectors in parallel.
    - Depending on the transport protocols used and the network infrastructure
    (e.g., the Maximum Transmission Unit (MTU)), IPFIX Messages might be
    limited to a certain length. As a result, the application has to ensure
    that it does not overshoot this length by appending too much data to the
    send buffer. ipfix_get_remaining_space() should be used to query how many
    bytes still may be added. It should be noted that there is a 4 bytes
    overhead per Data Set i.e. calling ipfix_start_data_set() adds 4 extra
    bytes to the send buffer. The macro IPFIX_OVERHEAD_PER_SET which is equal to
    4 should be used instead of hard wiring the value 4.
    - ipfix_send() transmits all Templates that have been previously defined
    but not yet transmitted. This function also retransmits Templates to UDP
    Collectors on a regular basis as required by RFC 5101. In addition, all
    Data Sets waiting in the send buffer are transmitted. The length of this
    buffer is reset to zero afterwards.
    - ipfix_remove_collector() can be used at any time to remove a Collector
    that has been previously added with ipfix_add_collector(). This includes
    closing the transport connection.
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
#ifdef SUPPORT_DTLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include "common/openssl/OpenSSL.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Supported export protocols */
typedef enum export_protocol_version {
	NFV9_PROTOCOL =9,
	IPFIX_PROTOCOL =10
} export_protocol_version;

/*
 * enterprise flag in IE ID field
 */
#define IPFIX_ENTERPRISE_BIT (1 << 15)

/*
 * Amount of iovec, the header consumes.
 * Change only, if you change the internal code!
 */
#define HEADER_USED_IOVEC_COUNT 1

/*
 * maximum number of collectors at a time.
 * can be specified by user
 */
#define IPFIX_MAX_COLLECTORS 16

/*
 * maximum number of templates at a time;
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

/*
 * maximum number of sets per IPFIX packet
 * TODO: This value is deliberately chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SETS_PER_PACKET 128

/*
 * maximum size of a sendbuffer
 * TODO: This value is deliberately chosen, adapt it if you need it or make it dynamic.
 */
#define IPFIX_MAX_SENDBUFSIZE (32 * 1024)

/*
 * maximum size of an IPFIX packet
 */
#define IPFIX_MAX_PACKETSIZE (1<<16)

/* MTU considerations apply to UDP and DTLS over UDP only. */

/* The MTU is set by the user. Path MTU discovery is turned off. */
#define IPFIX_MTU_FIXED 0
/* Path MTU discovery is turned on. */
#define IPFIX_MTU_DISCOVER 1

/*
 * Stevens: The maximum size of an IPv4 datagram is 65535 bytes, including
 * the IPv4 header. This is because of the 16-bit total length field.
 */
#define IPFIX_MTU_MAX UINT16_MAX


/* Use a very conservative default MTU so that it even works with IPSec over PPPoE */
#define IPFIX_MTU_CONSERVATIVE_DEFAULT 1400

#ifdef IP_MTU_DISCOVER
/* basically Linux */
#define IPFIX_MTU_DEFAULT IPFIX_MTU_MAX
#define IPFIX_MTU_MODE_DEFAULT IPFIX_MTU_DISCOVER
#else
/* non-Linux, mostly FreeBSD */
#define IPFIX_MTU_DEFAULT IPFIX_MTU_CONSERVATIVE_DEFAULT
#define IPFIX_MTU_MODE_DEFAULT IPFIX_MTU_FIXED
#endif

#ifdef SUPPORT_DTLS
#define IPFIX_DTLS_MAX_RECORD_LENGTH 16384
#endif

/* Struct containing an ipfix-header */
/*     Header Format (see RFC 5101)

3.1. Message Header Format


   The format of the IPFIX Message Header is shown in Figure F.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Version Number          |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Export Time                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Sequence Number                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Observation Domain ID                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Figure F: IPFIX Message Header Format

   Message Header Field Descriptions:

   Version

      Version of Flow Record format exported in this message.  The value
      of this field is 0x000a for the current version, incrementing by
      one the version used in the NetFlow services export version 9
      [RFC3954].

   Length

      Total length of the IPFIX Message, measured in octets, including
      Message Header and Set(s).

   Export Time

      Time, in seconds, since 0000 UTC Jan 1, 1970, at which the IPFIX
      Message Header leaves the Exporter.

   Sequence Number

      Incremental sequence counter modulo 2^32 of all IPFIX Data Records
      sent on this PR-SCTP stream from the current Observation Domain by
      the Exporting Process.  Check the specific meaning of this field
      in the subsections of Section 10 when UDP or TCP is selected as
      the transport protocol.  This value SHOULD be used by the
      Collecting Process to identify whether any IPFIX Data Records have
      been missed.  Template and Options Template Records do not
      increase the Sequence Number.

   Observation Domain ID

      A 32-bit identifier of the Observation Domain that is locally
      unique to the Exporting Process.  The Exporting Process uses the
      Observation Domain ID to uniquely identify to the Collecting
      Process the Observation Domain that metered the Flows.  It is
      RECOMMENDED that this identifier also be unique per IPFIX Device.
      Collecting Processes SHOULD use the Transport Session and the
      Observation Domain ID field to separate different export streams
      originating from the same Exporting Process.  The Observation
      Domain ID SHOULD be 0 when no specific Observation Domain ID is
      relevant for the entire IPFIX Message, for example, when exporting
      the Exporting Process Statistics, or in case of a hierarchy of
      Collectors when aggregated Data Records are exported.
      */

typedef struct {
	uint16_t version;
	uint16_t length;
	uint32_t export_time;
	uint32_t sequence_number;
	uint32_t observation_domain_id;
} ipfix_header;

/*  Set Header:
    
      0                   1                   2                   3 
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
     |          Set ID               |          Length               | 
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 
*/
/* Note that this ipfix_set_header struct is only used for data sets.
 * The header of template sets is built up in a char array.
 * (See ipfix_start_datatemplate)
 */

typedef struct {
    uint16_t set_id;
    uint16_t length;
} ipfix_set_header;

/*! \brief Overhead in bytes of one IPFIX Data Set. */
#define IPFIX_OVERHEAD_PER_SET 4

/* Struct containing an nfv9-header */
/* Header Format (See RFC 3954)
5.1.  Header Format

   The Packet Header format is specified as:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Version Number          |            Count              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           sysUpTime                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           UNIX Secs                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Sequence Number                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Source ID                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Packet Header Field Descriptions

   Version
         Version of Flow Record format exported in this packet.  The
         value of this field is 9 for the current version.

   Count
         The total number of records in the Export Packet, which is the
         sum of Options FlowSet records, Template FlowSet records, and
         Data FlowSet records.

   sysUpTime
         Time in milliseconds since this device was first booted.

   UNIX Secs
         Time in seconds since 0000 UTC 1970, at which the Export Packet
         leaves the Exporter.

   Sequence Number
         Incremental sequence counter of all Export Packets sent from
         the current Observation Domain by the Exporter.  This value
         MUST be cumulative, and SHOULD be used by the Collector to
         identify whether any Export Packets have been missed.

   Source ID
         A 32-bit value that identifies the Exporter Observation Domain.
         NetFlow Collectors SHOULD use the combination of the source IP
         address and the Source ID field to separate different export
         streams originating from the same Exporter.
*/

typedef struct {
	uint16_t version;
	uint16_t count;
	uint32_t sysUpTime;
	uint32_t unix_secs;
	uint32_t sequence_number;
	uint32_t source_id;
} nfv9_header;


/*! \brief The transport protocol used to transmit IPFIX data
 */
enum ipfix_transport_protocol {
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT 
	RAWDIR, 
#endif 
	DATAFILE,
	SCTP, /*!< SCTP, most favorable */
	UDP, /*!< UDP, available on all platforms, may result in MTU issues */
	TCP, /*!< TCP, currently unsupported by ipfixlolib */
	DTLS_OVER_UDP, /*!< DTLS over UDP, requires OpenSSL */
	DTLS_OVER_SCTP /*!< DTLS over SCTP, requires OpenSSL w/ SCTP patches from sctp.fh-muenster.de and recent version of FreeBSD */
};

typedef struct {
    uint16_t mtu; /*!< Maximum transmission unit (MTU).
		     If set to 0, PMTU discovery will be used.
		     (Only available on the Linux platform)
		     Applies to UDP and DTLS over UDP only. */
} ipfix_aux_config_udp;

typedef struct {
    const char *peer_fqdn; /*!< The Fully Qualified Domain Name (FQDN) of the
			     peer. If set, the peer i.e. the Collector
			     <em>must</em> present a certificate of which
			     either the subject's Common Name (CN) or one of
			     the subject alternative names matches the FQDN.
			     There is no support for wildcard matching. For the
			     certificate verification to work, the user must
			     also call <tt>ipfix_set_ca_locations()</tt> in
			     advance to specify the locations of the root CA
			     certificates.
			   
			     If set to NULL, anonymous cipher suites will be
			     added to the list of permissible cipher suites.
			     The identity of the peer will not be verified
			     then.*/
} ipfix_aux_config_dtls;

typedef struct {
    ipfix_aux_config_dtls dtls; /*!< DTLS specific configuration */
    ipfix_aux_config_udp udp; /*!< UDP specific configuration */
    unsigned max_connection_lifetime; /*!< Time in seconds after which the DTLS
				 connection is replaced by a new one.
				 This mechanism aims to overcome the
				 dead peer problem.*/
} ipfix_aux_config_dtls_over_udp;

typedef struct {
    ipfix_aux_config_dtls dtls; /*!< DTLS specific configuration */
} ipfix_aux_config_dtls_over_sctp;

/*
 * These indicate, if a field is committed (i.e. can be used)
 * unused or unclean (i.e. data is not complete yet)
 * T_SENT (Template was sent) and T_WITHDRAWN (Template destroyed) 
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
/* The life cycles of connections of type DTLS over UDP
 * and plain UDP are as follows:
 *
 * DTLS over UDP:
 *  - state == C_UNUSED
 *  - Successful calls to socket() and connect()
 *  - state <= C_NEW
 *  - DTLS handshake is taking place
 *  - DTLS handshake succeeded.
 *  - Templates are sent
 *  - state <= C_CONNECTED
 *
 * UDP:
 *  - state == C_UNUSED
 *  - Successful calls to socket() and connect()
 *  - state <= C_NEW
 *  - Templates are sent
 *  - state <= C_CONNECTED
 */
enum collector_state {C_UNUSED, C_NEW, C_DISCONNECTED, C_CONNECTED};


/*
 * Manages a record set
 * stores the record set header
 *
 * Please note that this struct is only used for data sets.
 *
 */
typedef struct{
	/* index of the current set.
	 * If no set is open, then .set_counter specifies the next free entry
	 * in .set_header_store */
	/* The maximum is IPFIX_MAX_SETS_PER_PACKET.
	 * set_counter serves as an index into set_header_store. */
	unsigned set_counter;

	/* buffer to store set headers */
	ipfix_set_header set_header_store[IPFIX_MAX_SETS_PER_PACKET];

	/* set length = sum of field length */
	/* This refers to the current data set only */
	unsigned data_length;
} ipfix_set_manager;


/*
 * A struct buffering data of an IPFIX Message
 */
typedef struct {
	struct iovec entries[IPFIX_MAX_SENDBUFSIZE]; /* an array of iovec structs, containing data and length */
	/* usage of entries:
	   - the first HEADER_USED_IOVEC_COUNT=1 entries are reserved for the IPFIX header
	   - the remaining entries are used for
	     * set headers
	     * individual fields of the sets/records
	 * Example:
	 * entries[0]: IPFIX Message header
	 * entries[1]: Set header
	 * entries[2]: Field 1 of Data Record 1
	 * entries[3]: Field 2 of Data Record 1
	 * entries[4]: Field 1 of Data Record 2
	 * entries[5]: Field 2 of Data Record 2
	 * entries[6]: Field 1 of Data Record 3
	 * entries[7]: Field 2 of Data Record 3
	 */
	unsigned current; /* next free entry in .entries */
	unsigned committed; /* number of committed entries in .entries
			     * If ipfix_end_data_set is called, all entries
			     * that belong to that specific data set will be 
			     * committed. This includes the set header as well
			     * as all data fields.
			     * If .current == .committed, then there's no
			     * open data set. Otherwise, there is an open
			     * data set. */
	unsigned marker; /* marker that allows to delete recently added entries */
	unsigned committed_data_length; /* length of the contained data (in bytes)
					 * not including the IPFIX message header.
					 * This value only includes data sets for which
					 * ipfix_end_data_set has been called i.e.
					 * it does not include data sets that are still
					 * "open". */
	ipfix_header ipfix_message_header;
	nfv9_header nfv9_message_header;
	unsigned record_count; /* Total number of records in this message */
	                       /* For Templates this appears to equal 'current'
				  as 'entries[]' contain one record per entry
				  compared to Data records more granular use */
	ipfix_set_manager set_manager; /* Only relevant when sendbuffer used
					  for data. Not relevant if used for
					  template sets. */
} ipfix_sendbuffer;

#ifdef SUPPORT_DTLS
typedef struct {
	int socket;
	// uint16_t mtu;
	SSL *ssl;
	time_t last_reconnect_attempt_time;
} ipfix_dtls_connection;
#endif

/*
 * A collector receiving messages from this exporter
 */
typedef struct {
	char ipv4address[16];
	uint16_t port_number;
	enum ipfix_transport_protocol protocol;
	int data_socket; // socket data and templates are sent to
	/* data_socket is NOT used for DTLS connections */
	struct sockaddr_in addr;
	uint32_t last_reconnect_attempt_time; // applies only to SCTP and DTLS at the moment
	enum collector_state state;
	uint32_t messages_sent; /* number of messages that should have been sent */
	char *basename;  /**< for protocol==DATAFILE, this variable contains the basename for the filename */
	int fh; /**< for protocol==DATAFILE, this variable contains the file handle */
	int filenum; /**< for protocol==DATAFILE, this variable contains the current filenumber: 'filename = basename + filenum'*/
	uint64_t bytes_written; /**< for protocol==DATAFILE, this variable contains the current filesize */
	uint32_t maxfilesize; /**< for protocol==DATAFILE, this variable contains the maximum filesize given in KiB*/
	int mtu_mode; /* Either IPFIX_MTU_FIXED or IPFIX_MTU_DISCOVER */
	uint16_t mtu; /* Maximum transmission unit.
			 Applies to UDP and DTLS over UDP only. */
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	char* packet_directory_path; /*!< if protocol==RAWDIR: path to a directory to store packets in. Ignored otherwise. */
#endif
#ifdef SUPPORT_DTLS
	/* Time in seconds after which a DTLS connection
	 * will be replaced by a new one. */
	unsigned dtls_max_connection_lifetime;
	unsigned dtls_connect_timeout;
	ipfix_dtls_connection dtls_main;
	ipfix_dtls_connection dtls_replacement;
	time_t connect_time; /* point in time when the connection setup
				succeeded. We need this to calculate the
				age of a connection. If DTLS is used,
				a connection rollover is performed when
				a connection reaches a certain age.*/
	const char *peer_fqdn;
#endif
} ipfix_receiving_collector;

/*
 * A template, mostly in binary form
 */
typedef struct{
	enum template_state state; // indicates, whether this template is valid.
	uint16_t template_id;
	uint16_t field_count;	// the number of fields the user announced
				// when calling ipfix_start_template
	uint16_t fixedfield_count;
				// the number of fixed-value fields the
				// user announced when calling ipfix_start_datatemplate
	uint16_t fields_added;	// make sure the user adds the exact the same number
				// of fields he told us to add when calling
				// ipfix_start_template()
				// Make sure fields_added == field_count + fixedfield_count
				// when the user calls ipfix_end_template()
	int fields_length;	// This also includes the length of the Set Header
				// It's basically the number of bytes written
				// into template_fields so far.

	int max_fields_length;	// size of the malloced char array
				// template_fields points to.

	char *template_fields;	// Points to a byte array that contains the Set Header and the
				// Template Record Header as it goes out on the wire.
				// Note that the type ipfix_set_header is *not* used
				// to build Set Headers for Template Sets.
} ipfix_lo_template;

/*
 * Each exporting process is associated with a sequence number and a source ID
 * The exporting process keeps track of the sequence number.
 */
typedef struct {
	export_protocol_version export_protocol;
	uint32_t sequence_number; // total number of data records 
	uint32_t sn_increment; // to be added to sequence number before sending data records
	uint32_t observation_domain_id;
	struct timeval start_time; // Time exporter system was started
	uint16_t max_message_size; /* Maximum size of an IPFIX message.
		       * This is the maximum size that all collectors allow.
		       * If a new collector is added that only allows
		       * smaller IPFIX messages, this value has to be
		       * updated.
		       * Only observed when sending messages
		       * containing data sets. IPFIX messages
		       * containing template sets might get
		       * longer than that. That's a TODO */
	ipfix_sendbuffer *template_sendbuffer;
	ipfix_sendbuffer *sctp_template_sendbuffer;
	ipfix_sendbuffer *data_sendbuffer;
	int collector_max_num; // maximum available collector
	ipfix_receiving_collector *collector_arr; // array of (collector_max_num) collectors

	// we also need some timer / counter to indicate,
	// if we should send the templates too.
	// This applies only to UDP and DTLS over UDP.
	// It contains the return value from time(NULL) which is the time since
	// the  Epoch (00:00:00 UTC, January 1, 1970), measured in seconds.
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
	ipfix_lo_template *template_arr;
#ifdef SUPPORT_DTLS
	SSL_CTX *ssl_ctx;
	const char *certificate_chain_file;
	const char *private_key_file;
	const char *ca_file;
	const char *ca_path;
#endif
} ipfix_exporter;


int ipfix_beat(ipfix_exporter *exporter);
/* generated by genproto */
int ipfix_init_exporter(export_protocol_version export_protocol, uint32_t observation_domain_id, ipfix_exporter **exporter);
int ipfix_deinit_exporter(ipfix_exporter **exporter_p);

int ipfix_add_collector(ipfix_exporter *exporter, const char *coll_ip4_addr, uint16_t coll_port, enum ipfix_transport_protocol proto, void *aux_config);
int ipfix_remove_collector(ipfix_exporter *exporter, const char *coll_ip4_addr, uint16_t coll_port);
int ipfix_start_template(ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count);
int ipfix_start_optionstemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length);
int ipfix_start_datatemplate(ipfix_exporter *exporter, uint16_t template_id, uint16_t preceding, uint16_t field_count, uint16_t fixedfield_count);
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id);
int ipfix_end_template(ipfix_exporter *exporter, uint16_t template_id );
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t template_id);
uint16_t ipfix_get_remaining_space(ipfix_exporter *exporter);
int ipfix_put_data_field(ipfix_exporter *exporter,void *data, unsigned length);
int ipfix_end_data_set(ipfix_exporter *exporter, uint16_t number_of_records);
int ipfix_cancel_data_set(ipfix_exporter *exporter);
int ipfix_set_data_field_marker(ipfix_exporter *exporter);
int ipfix_delete_data_fields_upto_marker(ipfix_exporter *exporter);
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length);
int ipfix_remove_template(ipfix_exporter *exporter, uint16_t template_id);
int ipfix_send(ipfix_exporter *exporter);
int ipfix_set_template_transmission_timer(ipfix_exporter *exporter, uint32_t timer); 	 
int ipfix_set_sctp_lifetime(ipfix_exporter *exporter, uint32_t lifetime);
int ipfix_set_sctp_reconnect_timer(ipfix_exporter *exporter, uint32_t timer);

int ipfix_set_dtls_certificate(ipfix_exporter *exporter, const char *certificate_chain_file, const char *private_key_file);
int ipfix_set_ca_locations(ipfix_exporter *exporter, const char *ca_file, const char *ca_path);

#ifdef __cplusplus
}
#endif

#endif
