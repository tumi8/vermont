/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX

 Changes by Daniel Mentz, 2009-01
   Added support for DTLS over UDP and DTLS over SCTP

 Changes by Gerhard Muenz, 2008-03
   non-blocking SCTP socket
 
 Changes by Alex Melnik, 2007-12
   Added SCTP support
   Corrected sequence number calculation
 
 Changes by Gerhard Muenz, 2006-02-01
   Changed and debugged sendbuffer structure and Co
   Added new function for canceling data sets and deleting fields

 Changes by Christoph Sommer, 2005-04-14
 Modified ipfixlolib-nothread Rev. 80
 Added support for DataTemplates (SetID 4)

 Changes by Ronny T. Lampert, 2005-01
 Changed 03-2005: Had to add a lot of casts for malloc() and friends
 because of stricter C++ checking

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */

#include "ipfixlolib.h"
#include "encoding.h"
#include "common/msg.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef __linux__
/* Copied from linux/in.h */
#define IP_MTU          14
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define bit_set(data, bits) ((data & bits) == bits)

#if defined(SUPPORT_DTLS_OVER_SCTP) && !defined(OPENSSL_SCTP)
# error OpenSSL built without SCTP support. Rebuild OpenSSL with SCTP support or turn off SUPPORT_DTLS_OVER_SCTP
#endif

#ifdef SUPPORT_DTLS
static int ensure_exporter_set_up_for_dtls(ipfix_exporter *exporter);
static void deinit_openssl_ctx(ipfix_exporter *exporter);
static int setup_dtls_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col, ipfix_dtls_connection *con);
static int dtls_send(ipfix_exporter *exporter, ipfix_receiving_collector *col, const struct iovec *iov, int iovcnt);
static int dtls_connect(ipfix_receiving_collector *col, ipfix_dtls_connection *con);
static void dtls_shutdown_and_cleanup(ipfix_dtls_connection *con);
static void dtls_fail_connection(ipfix_dtls_connection *con);
#endif
#ifdef SUPPORT_SCTP
static int init_send_sctp_socket(struct sockaddr_in serv_addr);
#ifdef SUPPORT_DTLS_OVER_SCTP
static void handle_sctp_event(BIO *bio, void *context, void *buf);
#endif
#endif
static int init_send_udp_socket(struct sockaddr_in serv_addr);
static int enable_pmtu_discovery(int s);
static int ipfix_find_template(ipfix_exporter *exporter, uint16_t template_id);
static void ipfix_update_header(ipfix_exporter *p_exporter, ipfix_receiving_collector *collector, ipfix_sendbuffer *sendbuf);
static int ipfix_init_sendbuffer(export_protocol_version export_protocol, ipfix_sendbuffer **sendbufn);
static int ipfix_reset_sendbuffer(ipfix_sendbuffer *sendbuf);
static int ipfix_deinit_sendbuffer(ipfix_sendbuffer **sendbuf);
static int ipfix_init_collector_array(ipfix_receiving_collector **col, int col_capacity);
static void remove_collector(ipfix_receiving_collector *collector);
static int ipfix_deinit_collector_array(ipfix_receiving_collector **col);
static int ipfix_init_send_socket(struct sockaddr_in serv_addr , enum ipfix_transport_protocol protocol);
static int ipfix_init_template_array(ipfix_exporter *exporter, int template_capacity);
static int ipfix_deinit_template(ipfix_lo_template* templ);
static int ipfix_deinit_template_array(ipfix_exporter *exporter);
static int ipfix_update_template_sendbuffer(ipfix_exporter *exporter);
static int ipfix_send_templates(ipfix_exporter* exporter);
static int ipfix_send_data(ipfix_exporter* exporter);
static int ipfix_new_file(ipfix_receiving_collector* recvcoll);
static void update_exporter_max_message_size(ipfix_exporter *exporter);
static int update_collector_mtu(ipfix_exporter *exporter, ipfix_receiving_collector *col);
static int get_mtu(const int s);
static int ipfix_enterprise_flag_set(uint16_t id);


#ifdef SUPPORT_DTLS
/* A separate SSL_CTX object is created for every ipfix_exporter.
 * Returns 0 on success, -1 on error
 * */
static int ensure_exporter_set_up_for_dtls(ipfix_exporter *e) {
    ensure_openssl_init();

    if (e->ssl_ctx) return 0;

    /* This SSL_CTX object will be freed in deinit_openssl_ctx() */
    if ( ! (e->ssl_ctx=SSL_CTX_new(DTLSv1_client_method())) ) {
	msg(MSG_FATAL, "Failed to create SSL context");
	msg_openssl_errors();
	return -1;
    }
    SSL_CTX_set_read_ahead(e->ssl_ctx,1);

    if ( (e->ca_file || e->ca_path) &&
	! SSL_CTX_load_verify_locations(e->ssl_ctx,e->ca_file,e->ca_path) ) {
	msg(MSG_FATAL,"SSL_CTX_load_verify_locations() failed.");
	msg_openssl_errors();
	return -1;
    }
    /* Load our own certificate */
    if (e->certificate_chain_file) {
	if (!SSL_CTX_use_certificate_chain_file(e->ssl_ctx, e->certificate_chain_file)) {
	    msg(MSG_FATAL,"Unable to load certificate chain file %s",e->certificate_chain_file);
	    msg_openssl_errors();
	    return -1;
	}
	if (!SSL_CTX_use_PrivateKey_file(e->ssl_ctx, e->private_key_file, SSL_FILETYPE_PEM)) {
	    msg(MSG_FATAL,"Unable to load private key file %s",e->private_key_file);
	    msg_openssl_errors();
	    return -1;
	}
	if (!SSL_CTX_check_private_key(e->ssl_ctx)) {
	    msg(MSG_FATAL,"Private key and certificate do not match");
	    msg_openssl_errors();
	    return -1;
	}
	DPRINTF("We successfully loaded our certificate.");
    } else {
	DPRINTF("We do NOT have a certificate.");
    }
    /* We leave the certificate_authorities list of the Certificate Request
     * empty. See RFC 4346 7.4.4. Certificate request. */
    return 0;
}

static void deinit_openssl_ctx(ipfix_exporter *e) {
    if (e->ssl_ctx) { SSL_CTX_free(e->ssl_ctx); }
    e->ssl_ctx = NULL;
}

static int dtls_verify_peer_cb(void *context, const char* dnsname) {
    const ipfix_receiving_collector *col =
	(const ipfix_receiving_collector *) context;
    return strcasecmp(col->peer_fqdn,dnsname) ? 0 : 1;
}

static int dtls_get_replacement_connection_ready(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {
    int ret;
    if (!col->dtls_replacement.ssl) {
	/* No SSL object has been created yet. Let's open a socket and
	 * setup a new SSL object. */
	DPRINTF("Setting up replacement connection.");
	if (setup_dtls_connection(exporter,col,&col->dtls_replacement)) {
	    return -1;
	}
    }
    ret = dtls_connect(col,&col->dtls_replacement);
    if (ret == 1) {
	DPRINTF("Replacement connection setup successful.");
	return 1; /* SUCCESS */
    }
    if (ret == 0) {
	if (col->dtls_connect_timeout && 
		(time(NULL) - col->dtls_replacement.last_reconnect_attempt_time > col->dtls_connect_timeout)) {
	    msg(MSG_ERROR,"DTLS replacement connection setup taking too long.");
	    dtls_fail_connection(&col->dtls_replacement);
	} else {
	    DPRINTF("Replacement connection setup still ongoing.");
	    return 0;
	}
    }
    return -1;
}

static int dtls_send_templates(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {

    if (exporter->template_sendbuffer->committed_data_length == 0)
	return 0;

    ipfix_update_header(exporter, col,
	exporter->template_sendbuffer);
    col->messages_sent++;
    DPRINTF("Sending templates over DTLS.");
    return dtls_send(exporter,col,
	exporter->template_sendbuffer->entries,
	exporter->template_sendbuffer->current);
}

static void dtls_swap_connections(ipfix_dtls_connection *a, ipfix_dtls_connection *b) {
    ipfix_dtls_connection tmp;
    memcpy(&tmp,a,sizeof(tmp));
    memcpy(a,b,sizeof(*b));
    memcpy(b,&tmp,sizeof(*b));
}


/* This function pushes the connection setup forward if we are in state C_NEW.
 * It also sets up the replacement connection if it's time to replace the 
 * current connection with a new one.
 * If we are in state C_DISCONNECT then it sets up a new main connection.
 *
 * Return values:
 * returns 0 on success
 * returns 1 if a connection setup procedure is still ongoing. Then, this
 *           function should be called again after a short period of time.
 * returns -1 on error.
 * Note that success does not mean that we are connected because the
 * connection setup might still be ongoing. Therefore you still have
 * to check the state member of ipfix_receiving_collector to determine
 * if we are connected. */
static int dtls_manage_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col) {
    int ret, rc = 0;

    if (col->state == C_CONNECTED) {
	if( col->protocol == DTLS_OVER_SCTP ||
		col->dtls_max_connection_lifetime == 0 ||
		time(NULL) - col->connect_time < col->dtls_max_connection_lifetime) 
	return 0;
	/* Alright, the connection is already very old and needs to be
	 * replaced. Let's get the replacement / backup connection ready. */
	ret = dtls_get_replacement_connection_ready(exporter, col);
	rc = 1;
	if (ret == 1) { /* Connection setup completed */
	    rc = 0;
	    DPRINTF("Swapping connections.");
	    dtls_swap_connections(&col->dtls_main,&col->dtls_replacement);
	    DPRINTF("Shutting down old DTLS connection.");
	    dtls_shutdown_and_cleanup(&col->dtls_replacement);
	    col->connect_time = time(NULL);
	    ret = dtls_send_templates(exporter, col);
	    /* We do not need to check the return value because
	     * dtls_send_templates already shuts down the DTLS connection
	     * in case of failure.
	     * It also sets state to C_DISCONNECTED in this case. */
	}
	/* We ignore all other return values of dtls_get_replacement_connection_ready() */
    }
    if (col->state == C_NEW) {
	rc = 1;
	/* Connection setup is still ongoing. Let's push it forward. */
	ret = dtls_connect(col,&col->dtls_main);
	if (ret == 1) {
	    /* SUCCESS */
	    col->state = C_CONNECTED;
	    col->connect_time = time(NULL);
	    if (update_collector_mtu(exporter, col)) {
		/* update_collector_mtu calls remove_collector
		   in case of failure which in turn sets
		   col->state to C_UNUSED. */
		return -1;
	    }
	    ret = dtls_send_templates(exporter, col);
	    /* dtls_send (inside dtls_send_templates) calls
	     * dtls_fail_connection() and sets col->state
	     * in case of failure. */
	    if (ret >= 0) return 0; else return -1;
	} else if (ret == -1) {
	    /* Failure
	     * dtls_connect() cleaned up SSL object already.
	     * Remember that the socket is now part of the DTLS connection
	     * abstraction. dtls_connect() closed the socket as well. */
	    col->state = C_DISCONNECTED;
	    return -1;
	}
    }
    if (col->state == C_DISCONNECTED) {
	/* Wait dtls_connect_timeout seconds before
	   re-initiating DTLS connection */
	if (time(NULL) < col->last_reconnect_attempt_time +
		col->dtls_connect_timeout)
	    return 1;
	rc = 1;
	if (setup_dtls_connection(exporter,col,&col->dtls_main)) {
	    /* col->state stays in C_DISCONNECTED in this case
	     * setup_dtls_connection() does not alter it. */
	    return -1;
	}
	col->state = C_NEW;
	col->last_reconnect_attempt_time = time(NULL);
    }
    return rc;
}

/* Return values:
 * -1 failure
 *  0 no failure but not yet connected. You need to call dtls_connect again
 *        next time
 *  1 yes. now we're connected. Don't call dtls_connect again. */
static int dtls_connect(ipfix_receiving_collector *col, ipfix_dtls_connection *con) {
    int ret, error;

    ret = SSL_connect(con->ssl);
    error = SSL_get_error(con->ssl,ret);
    if (error == SSL_ERROR_NONE) {
	msg_openssl_return_code(MSG_DEBUG,"SSL_connect()",ret,error);
	msg(MSG_INFO, "Successfully (re)connected to %s-over-DTLS collector.", col->protocol == DTLS_OVER_SCTP ? "SCTP" : "UDP");
	msg(MSG_INFO,"TLS Cipher: %s",SSL_get_cipher_name(con->ssl));
	DPRINTF("DTLS handshake succeeded. We are now connected.");
	if (col->peer_fqdn) { /* We need to verify the identity of our peer */
	    if (verify_ssl_peer(con->ssl,&dtls_verify_peer_cb,col)) {
		DPRINTF("Peer authentication successful.");
	    } else {
		msg(MSG_ERROR,"Peer authentication failed. Shutting down connection.");
		dtls_fail_connection(con);
		return -1;
	    }
	}
	return 1;
    } else if (error == SSL_ERROR_WANT_READ) {
	msg_openssl_return_code(MSG_DEBUG,"SSL_connect()",ret,error);
	return 0;
    } else {
	msg_openssl_return_code(MSG_ERROR,"SSL_connect()",ret,error);
	dtls_fail_connection(con);
	return -1;
    }
}

/* Return values:
 * n>0: sent n bytes
 * 0: Could not send due to OpenSSL returning SSL_ERROR_WANT_READ
 * -1: Recoverable error
 * -2: Bad Error. DTLS connection has been shutdown already.
 *	You should set state to C_DISCONNECT
 * -3: Message too long. You should query the current MTU
 *     estimate in this case and update the corresponding
 *     property of the collector.
 */

static int dtls_send_helper( ipfix_dtls_connection *con,
	const struct iovec *iov, int iovcnt) {
    int len, error, i;
    char sendbuf[IPFIX_MAX_PACKETSIZE];
    char *sendbufcur = sendbuf;
    int maxsendbuflen = sizeof(sendbuf);
    /* Collect data form iovecs */
    for (i=0;i<iovcnt;i++) {
	if (sendbufcur + iov[i].iov_len > sendbuf + maxsendbuflen) {
	    msg(MSG_FATAL, "sendbuffer for dtls_send too small.");
	    return -1;
	}
	memcpy(sendbufcur,iov[i].iov_base,iov[i].iov_len);
	sendbufcur+=iov[i].iov_len;
    }

    len = SSL_write(con->ssl, sendbuf, sendbufcur - sendbuf);
    error = SSL_get_error(con->ssl,len);
#ifdef DEBUG
    char buf[32];
    snprintf(buf,sizeof(buf),"SSL_write(%d bytes of data)",(int) (sendbufcur - sendbuf) );
    msg_openssl_return_code(MSG_DEBUG,buf,len,error);
#endif
    switch (error) {
	case SSL_ERROR_NONE:
	    if (len!=sendbufcur - sendbuf) {
		msg(MSG_FATAL, "len!=sendbuflen when calling SSL_write()");
		return -1;
	    }
	    return sendbufcur - sendbuf; /* SUCCESS */
	case SSL_ERROR_WANT_READ:
	    return 0;
	case SSL_ERROR_SYSCALL:
	    if (errno == EMSGSIZE) {
		return -3;
	    }
	    /* fall through */
	default:
	    msg_openssl_return_code(MSG_ERROR,"SSL_write()",len,error);
	    dtls_fail_connection(con);
	    return -2;
    }
}

/* Return values:
 * -1 error
 *  0 could not write because OpenSSL returned SSL_ERROR_WANT_READ
 *  n>0 number of bytes written
 */
static int dtls_send(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	const struct iovec *iov, int iovcnt) {

    int len;

    /* DTLS negotiation has to be finished before we can send data.
     * Drop out of this function if we are not yet connected. */
    if (col->state != C_CONNECTED) {
	return -1;
    }

    len = dtls_send_helper(&col->dtls_main, iov, iovcnt);
    if (len == -2) {
	col->state = C_DISCONNECTED;
	return -1;
    } else if (len == -3) {
	update_collector_mtu(exporter,col);
	return -1;
    }
    return len;
}

#ifdef SUPPORT_DTLS_OVER_SCTP
/* Return values:
 * -1 error
 *  0 could not write because OpenSSL returned SSL_ERROR_WANT_READ
 *  n>0 number of bytes written
 */
static int dtls_over_sctp_send(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	const struct iovec *iov, int iovcnt, uint32_t pr_value) {

    struct sctp_sndrcvinfo sinfo;
    memset(&sinfo, 0, sizeof(struct sctp_sndrcvinfo));
    sinfo.sinfo_timetolive = pr_value; // pr_value; FIXME
    BIO_ctrl(SSL_get_wbio(col->dtls_main.ssl), BIO_CTRL_DGRAM_SCTP_SET_SNDINFO, sizeof(struct sctp_sndrcvinfo), &sinfo);
    return dtls_send(exporter,col,iov,iovcnt);
}
#endif

static int create_dtls_socket(ipfix_receiving_collector *col) {
	int s, type, protocol;
#ifdef SUPPORT_DTLS_OVER_SCTP
	struct sctp_event_subscribe event;
	if (col->protocol == DTLS_OVER_SCTP) {
	    /* SCTP case */
	    type = SOCK_STREAM;
	    protocol = IPPROTO_SCTP;
	} else
#endif
	{
	    /* UDP case */
	    type = SOCK_DGRAM;
	    protocol = 0;
	}
        if((s = socket(PF_INET, type, protocol)) < 0 ) {
                msg(MSG_FATAL, "error opening socket, %s", strerror(errno));
                return -1;
        }

	if (col->protocol == DTLS_OVER_UDP) {
	    if(enable_pmtu_discovery(s)) {
		close(s);
		return -1;
	    }
	}
	
	// set non-blocking
	int flags;
	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK;
	if(fcntl(s, F_SETFL, flags) == -1) {
                msg(MSG_FATAL, "could not set socket non-blocking");
		close(s);
                return -1;
	}
#ifdef SUPPORT_DTLS_OVER_SCTP
	/* enable the reception of SCTP_SNDRCV information on a per
	 * message basis. */
	memset(&event, 0, sizeof(event));
	event.sctp_data_io_event = 1;
	if (setsockopt(s, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		msg(MSG_ERROR, "SCTP: setsockopt() failed to enable sctp_data_io_event, %s", strerror(errno));
		close(s);
                return -1;
	}
#endif

	return s;
}

/* returns 0 on success and -1 on failure */
static int setup_dtls_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col, ipfix_dtls_connection *con) {
    BIO *bio;
/* Resources allocated in this function. Those need to be freed in case of failure:
 * - socket
 * - SSL object
 * - BIO
 */

#ifdef DEBUG
    if (con->socket!=-1) {
	msg(MSG_FATAL,"socket != -1");
	close(con->socket);
	con->socket = -1;
    }
#endif

    /* Create socket
     create_dtls_socket() also activates PMTU Discovery. */
    if ((con->socket = create_dtls_socket(col)) < 0) {
	return -1;
    }
    DPRINTF("Created socket %d",con->socket);

    /* ensure an SSL_CTX object is set up */
    if (ensure_exporter_set_up_for_dtls(exporter)) {
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* create SSL object */
    if ( ! (con->ssl = SSL_new(exporter->ssl_ctx))) {
	msg(MSG_FATAL, "Failed to create SSL object.");
	msg_openssl_errors();
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* Set verification parameters and cipherlist */
    if (!col->peer_fqdn) {
	SSL_set_cipher_list(con->ssl,"ALL"); // This includes anonymous ciphers
	DPRINTF("We are NOT going to verify the certificates of the collectors b/c "
		"the peerFqdn option is NOT set.");
    } else {
	if ( ! ((exporter->ca_file || exporter->ca_path) &&
		    exporter->certificate_chain_file) ) {
	    msg(MSG_ERROR,"Cannot verify certificates of collectors because prerequisites not met. "
		    "Prerequisites are: 1. CApath or CAfile or both set, "
		    "2. We have a certificate including the private key");
	    SSL_free(con->ssl);con->ssl = NULL;
	    close(con->socket);con->socket = -1;
	    return -1;
	} else {
	    SSL_set_cipher_list(con->ssl,"DEFAULT");
	    SSL_set_verify(con->ssl,SSL_VERIFY_PEER |
		    SSL_VERIFY_FAIL_IF_NO_PEER_CERT,0);
	    DPRINTF("We are going to request certificates from the collectors "
		    "and we are going to verify these b/c "
		    "the peerFqdn option is set");
	}
    }
    /* create input/output abstraction for SSL object */
#ifdef SUPPORT_DTLS_OVER_SCTP
    if (col->protocol == DTLS_OVER_SCTP) {
	bio = BIO_new_dgram_sctp(con->socket,BIO_NOCLOSE);
	BIO_dgram_sctp_notification_cb(bio, &handle_sctp_event,con);
    }
    else /* DTLS_OVER_UDP */
#endif
	bio = BIO_new_dgram(con->socket,BIO_NOCLOSE);

    if ( ! bio) {
	msg(MSG_FATAL,"Failed to create datagram BIO.");
	msg_openssl_errors();
	SSL_free(con->ssl);con->ssl = NULL;
	close(con->socket);con->socket = -1;
	return -1;
    }
#ifdef SUPPORT_DTLS_OVER_SCTP
    if (col->protocol != DTLS_OVER_SCTP)
#endif
    (void)BIO_ctrl(bio,BIO_CTRL_DGRAM_MTU_DISCOVER,0,0);
    (void)BIO_ctrl_set_connected(bio,1,&col->addr); /* TODO: Explain, why are we doing this? */
    SSL_set_bio(con->ssl,bio,bio);
    // connect (non-blocking, i.e. handshake is initiated, not terminated)
    if((connect(con->socket, (struct sockaddr*)&col->addr, sizeof(col->addr) ) == -1) && (errno != EINPROGRESS)) {
	    msg(MSG_FATAL, "connect failed, %s", strerror(errno));
	    SSL_free(con->ssl);con->ssl = NULL;
	    close(con->socket);con->socket = -1;
	    return -1;
    }
    DPRINTF("Set up SSL object.");

    con->last_reconnect_attempt_time = time(NULL);

    return 0;
}

static void dtls_shutdown_and_cleanup(ipfix_dtls_connection *con) {
    int ret,error;
    if (!con->ssl) return;
    DPRINTF("Shutting down SSL connection.");
    ret = SSL_shutdown(con->ssl);
    error = SSL_get_error(con->ssl,ret);
#ifdef DEBUG
    msg_openssl_return_code(MSG_DEBUG,"SSL_shutdown()",ret,error);
#endif
    /* TODO: loop only if ret==-1 and error==WANT_READ or WANT_WRITE */
    int i = 0;
    while (ret != 1 && !(SSL_get_shutdown(con->ssl) & SSL_RECEIVED_SHUTDOWN)
	    && ( (ret == 0 && error == SSL_ERROR_SYSCALL) ||
		 (ret == -1 && (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)))) {
	fd_set readfds;
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(con->socket, &readfds);
	ret = select(con->socket + 1,&readfds,NULL,NULL,&timeout);
	DPRINTF("select returned: %d",ret);
	DPRINTF("Calling SSL_shutdown()");
	ret = SSL_shutdown(con->ssl);
	error = SSL_get_error(con->ssl,ret);
	msg_openssl_return_code(MSG_DEBUG,"SSL_shutdown()",ret,error);
	if (i++ == 3) {
	    msg(MSG_ERROR,"Too many calls to select(). Breaking out.");
	    break;
	}
    }
    /* Note: SSL_free() also frees associated sending and receiving BIOs */
    SSL_free(con->ssl);
    con->ssl = NULL;
    con->last_reconnect_attempt_time = 0;
    /* Close socket */
    if ( con->socket != -1) {
	DPRINTF("Closing socket");
	ret = close(con->socket);
	DPRINTF("close returned %d",ret);
	con->socket = -1;
    }
}

static void dtls_fail_connection(ipfix_dtls_connection *con) {
    DPRINTF("Failing DTLS connection.");
    dtls_shutdown_and_cleanup(con);
}

#endif /* SUPPORT_DTLS */

/*!
 * \brief Should be called on a regular basis to push forward DTLS connection setup procedures.
 *
 * ipfixlolib depends on the user to call this function regularly
 * if there is an ongoing connection setup procedure. This is
 * necessary because<ul>
 * <li>ipfixlolib is <em>not</em> allowed to block the calling thread,</li>
 * <li>ipfixlolib does not run in a dedicated thread and</li>
 * <li>a DTLS connection setup (i.e. handshake) may take an</li>
 * extended period of time.
 * </ul>
 *
 * \param exporter pointer to previously initialized exporter struct
 * \return 1 this function should be called again after a short period of time
 * \return 0 otherwise
 * \sa ipfix_add_collector()
 */
int ipfix_beat(ipfix_exporter *exporter) {
    int ret = 0;
#ifdef SUPPORT_DTLS
    int i;
    for (i = 0; i < exporter->collector_max_num; i++) {
	ipfix_receiving_collector *col = &exporter->collector_arr[i];
	// is the collector a valid target?
	if (col->state != C_UNUSED) {
	    if (col->protocol == DTLS_OVER_UDP ||
		    col->protocol == DTLS_OVER_SCTP) {
		if (dtls_manage_connection(exporter,col))
		    ret = 1;
	    }
	}
    }
#endif
    return ret;
}

/*
 * Initializes a UDP-socket to send data to.
 * Parameters:
 * char* serv_ip4_addr IP-Address of the recipient (e.g. "123.123.123.123")
 * serv_port the UDP port number of the server.
 * Returns: a socket to write to. -1 on failure
 */
static int init_send_udp_socket(struct sockaddr_in serv_addr){

        int s;
        // create socket
        if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                msg(MSG_FATAL, "error opening socket, %s", strerror(errno));
                return -1;
        }

        // connect to server
        if(connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr) ) < 0) {
                msg(MSG_FATAL, "connect failed, %s", strerror(errno));
                /* clean up */
                close(s);
                return -1;
        }
	if(enable_pmtu_discovery(s)) {
	    close(s);
	    return -1;
	}

	return s;
}

static int enable_pmtu_discovery(int s) {
#ifdef IP_MTU_DISCOVER
    // Linux
    const int optval = IP_PMTUDISC_DO;
    if (setsockopt(s,IPPROTO_IP,IP_MTU_DISCOVER,&optval,sizeof(optval))) {
	    msg(MSG_FATAL, "setsockopt(...,IP_MTU_DISCOVER,...) failed, %s", strerror(errno));
	    return -1;
    }
#elif IP_DONTFRAG
    // FreeBSD
    const int optval = 1;
    if (setsockopt(s,IPPROTO_IP,IP_DONTFRAG,&optval,sizeof(optval))) {
	    msg(MSG_FATAL, "setsockopt(...,IP_DONTFRAG,...) failed, %s", strerror(errno));
	    return -1;
    }
#endif
    return 0;
}

static int get_mtu(const int s) {
#ifdef IP_MTU
    int optval = 0;
    socklen_t optlen = sizeof(optval);
    if (getsockopt(s,IPPROTO_IP,IP_MTU,&optval,&optlen)) {
	msg(MSG_FATAL, "getsockopt(...,IP_MTU,...) failed, %s", strerror(errno));
	return -1;
    } else
	return optval;
#else
    /* Should not be called if PMTU discovery is unsupported. */
    return -1;
#endif
}


#ifdef SUPPORT_SCTP
/********************************************************************
** SCTP Extension Code:
*********************************************************************/
/*
 * Initializes a SCTP socket to send data to.
 * Parameters:
 * char* serv_ip4_addr IP-Address of the recipient (e.g. "123.123.123.123")
 * serv_port the SCTP port number of the server.
 * Returns: a socket to write to. -1 on failure
 */
static int init_send_sctp_socket(struct sockaddr_in serv_addr){

    int s;

    //create socket:
    DPRINTFL(MSG_VDEBUG, "Creating SCTP Socket ...");
    if((s = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0 ) {
	msg(MSG_FATAL, "error opening SCTP socket, %s", strerror(errno));
	return -1;
    }
    // set non-blocking
    int flags;
    flags = fcntl(s, F_GETFL);
    flags |= O_NONBLOCK;
    if(fcntl(s, F_SETFL, flags) == -1) {
	msg(MSG_FATAL, "could not set socket non-blocking");
	close(s);
	return -1;
    }
    struct sctp_event_subscribe event;
    memset(&event, 0, sizeof(event));
    event.sctp_association_event = 1;
    if (setsockopt(s, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
	    msg(MSG_ERROR, "SCTP: setsockopt() failed to enable sctp_data_io_event, %s", strerror(errno));
	    close(s);
	    return -1;
    }
    // connect (non-blocking, i.e. handshake is initiated, not terminated)
    if((connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr) ) == -1) && (errno != EINPROGRESS)) {
	msg(MSG_ERROR, "SCTP connect failed, %s", strerror(errno));
	close(s);
	return -1;
    }
    DPRINTFL(MSG_DEBUG, "SCTP Socket created");

    return s;
}

/*
 * modification of the original sctp_sendmsg to handle iovec structs
 * Parameters:
 * s 		: socket
 * *vector 	: iovec struct containing the buffer to send
 * v_len 	: length of the buffer
 * *to 		: address where data is going to be sent
 * tolen 	: length of the address
 * ppid, flags, stream_no, timetolive, context : sctp parameters
 */
static int sctp_sendmsgv(int s, struct iovec *vector, int v_len, struct sockaddr *to,
		socklen_t tolen, uint32_t ppid, uint32_t flags,
	     	uint16_t stream_no, uint32_t timetolive, uint32_t context){

	struct msghdr outmsg;
	char outcmsg[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
	struct cmsghdr *cmsg;
	struct sctp_sndrcvinfo *sinfo;
	char sendbuf[IPFIX_MAX_PACKETSIZE];
	struct iovec iv;

	outmsg.msg_name = to;
	outmsg.msg_namelen = tolen;

	/* Only IOV_MAX iovecs can be passed to sendmsg(). If we have more,
	 we copy all these iovecs into one buffer. */
	if (v_len <= sysconf(_SC_IOV_MAX)) {
	    outmsg.msg_iov = vector;
	    outmsg.msg_iovlen = v_len;
	} else {
	    int i;
	    int maxsendbuflen = sizeof(sendbuf);
	    char *sendbufcur = sendbuf;
	    /* Collect data form iovecs */
	    for (i=0;i<v_len;i++) {
		if (sendbufcur + vector[i].iov_len > sendbuf + maxsendbuflen) {
		    msg(MSG_FATAL, "sendbuffer too small.");
		    return -1;
		}
		memcpy(sendbufcur,vector[i].iov_base,vector[i].iov_len);
		sendbufcur+=vector[i].iov_len;
	    }
	    iv.iov_base = sendbuf;
	    iv.iov_len = sendbufcur - sendbuf;
	    outmsg.msg_iov = &iv;
	    outmsg.msg_iovlen = 1;
	}

	outmsg.msg_control = outcmsg;
	outmsg.msg_controllen = sizeof(outcmsg);
	outmsg.msg_flags = 0;

	cmsg = CMSG_FIRSTHDR(&outmsg);
	cmsg->cmsg_level = IPPROTO_SCTP;
	cmsg->cmsg_type = SCTP_SNDRCV;
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

	outmsg.msg_controllen = cmsg->cmsg_len;
	sinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
	memset(sinfo, 0, sizeof(struct sctp_sndrcvinfo));
	sinfo->sinfo_ppid = ppid;
	sinfo->sinfo_flags = flags;
	sinfo->sinfo_stream = stream_no;
	sinfo->sinfo_timetolive = timetolive;
	sinfo->sinfo_context = context;

	return sendmsg(s, &outmsg, 0);
}
#endif /*SUPPORT_SCTP*/

/********************************************************************
//END of SCTP Extension Code:
*********************************************************************/

/*
 * Initialize an exporter process
 * Allocates all memory necessary.
 * Parameters:
 * sourceID The source ID, to which the exporter will be initialized to.
 * exporter an ipfix_exporter* to be initialized
 */
/*!
 * \brief Creates and initialize a new exporter.
 * This function allocates memory for a new exporter struct and multiple buffers. 
 *
 * Make sure to call <tt>ipfix_deinit_exporter()</tt> if the exporter is not needed any more in order to free the allocated memory.
 *
 * \param observation_domain_id the Observation Domain ID that will be included in every IPFIX Message Header
 * \param exporter a pointer to a pointer to exporter struct
 * \return 0 success
 * \return -1 failure. Usually due to failed memory allocation requests.
 * \sa ipfix_deinit_exporter()
 */
int ipfix_init_exporter(export_protocol_version export_protocol, uint32_t observation_domain_id, ipfix_exporter **exporter)
{
        ipfix_exporter *tmp;
        int ret;

        if(!(tmp=(ipfix_exporter *)malloc(sizeof(ipfix_exporter)))) {
                goto out;
        }

	switch(export_protocol) {
	case NFV9_PROTOCOL:
	    break;
	case IPFIX_PROTOCOL:
	    break;
	default:
	    msg(MSG_FATAL, "Unknown protocol");
	    goto out;
	    break;
	}
	tmp->export_protocol = export_protocol;

        tmp->sequence_number = 0;
        tmp->sn_increment = 0;
        tmp->observation_domain_id=observation_domain_id;

	gettimeofday(&tmp->start_time, NULL);

	tmp->max_message_size = IPFIX_MTU_CONSERVATIVE_DEFAULT;

        tmp->collector_max_num = 0;
#ifdef SUPPORT_DTLS
	tmp->ssl_ctx = NULL;
	tmp->certificate_chain_file = NULL;
	tmp->private_key_file = NULL;
	tmp->ca_file = NULL;
	tmp->ca_path = NULL;
#endif

        // initialize the sendbuffers
        ret=ipfix_init_sendbuffer(export_protocol, &(tmp->data_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "initializing data sendbuffer failed");
                goto out1;
        }

        ret=ipfix_init_sendbuffer(export_protocol, &(tmp->template_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "initializing template sendbuffer failed");
                goto out2;
        }
	
	ret=ipfix_init_sendbuffer(export_protocol, &(tmp->sctp_template_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "initializing sctp template sendbuffer failed");
                goto out5;
        }
	
        // initialize the collectors to zero
        ret=ipfix_init_collector_array( &(tmp->collector_arr), IPFIX_MAX_COLLECTORS);
        if (ret !=0) {
                msg(MSG_FATAL, "initializing collectors failed");
                goto out3;
        }

        tmp->collector_max_num = IPFIX_MAX_COLLECTORS;

        // initialize an array to hold the templates.
        if(ipfix_init_template_array(tmp, IPFIX_MAX_TEMPLATES)) {
                goto out4;
        }
	
        // we have not transmitted any templates yet!
        tmp->last_template_transmission_time=0;
        tmp->template_transmission_timer=IPFIX_DEFAULT_TEMPLATE_TIMER;
	tmp->sctp_reconnect_timer=IPFIX_DEFAULT_SCTP_RECONNECT_TIMER;
	tmp->sctp_lifetime=IPFIX_DEFAULT_SCTP_DATA_LIFETIME;
	
        /* finally attach new exporter to the pointer we were given */
        *exporter=tmp;

        return 0;

out5:
        ipfix_deinit_sendbuffer(&(tmp->sctp_template_sendbuffer));
out4:
        ipfix_deinit_collector_array(&(tmp->collector_arr));
out3:
        ipfix_deinit_sendbuffer(&(tmp->data_sendbuffer));
out2:
        ipfix_deinit_sendbuffer(&(tmp->template_sendbuffer));
out1:
        free(tmp);
out:
        /* we have nothing to free */
        return -1;
}


/*!
 * \brief Cleanup previously initialized exporter and free memory.
 *
 * All Collectors which are associated with this Exporter are cleaned up as
 * well.
 *
 * \param exporter pointer to exporter struct 
 * \return 0 success
 * \sa ipfix_init_exporter()
 */
int ipfix_deinit_exporter(ipfix_exporter **exporter_p) {
        ipfix_exporter *exporter = *exporter_p;
        // Cleanup processes
        // close sockets etc.
        // (currently, nothing to do)

        // free all children

        // deinitialize array to hold the templates.
        ipfix_deinit_template_array(exporter);

        /*   free ( (**exporter).template_arr); */
        /*   (**exporter).template_arr = NULL; */

        // deinitialize the sendbuffers
        ipfix_deinit_sendbuffer(&(exporter->data_sendbuffer));
        ipfix_deinit_sendbuffer(&(exporter->template_sendbuffer));
        ipfix_deinit_sendbuffer(&(exporter->sctp_template_sendbuffer));

        // find the collector in the exporter
        int i=0;
        for(i=0;i<exporter->collector_max_num;i++) {
	        if (exporter->collector_arr[i].state != C_UNUSED)
                remove_collector(&exporter->collector_arr[i]);
        }
        // deinitialize the collectors
        ipfix_deinit_collector_array(&(exporter->collector_arr));

#ifdef SUPPORT_DTLS
	deinit_openssl_ctx(exporter);
	free( (void *) exporter->certificate_chain_file);
	free( (void *) exporter->private_key_file);
	free( (void *) exporter->ca_file);
	free( (void *) exporter->ca_path);
#endif

        // free own memory
        free(exporter);
        exporter=NULL;

        return 0;
}

static void update_exporter_max_message_size(ipfix_exporter *exporter) {
    ipfix_receiving_collector *col;
    int i;
    uint16_t max_message_size;
    max_message_size = exporter->max_message_size;
    for(i=0;i<exporter->collector_max_num;i++) {
	col = &exporter->collector_arr[i];
	if(col->state != C_UNUSED &&
		(col->protocol == UDP || col->protocol == DTLS_OVER_UDP)) {
	    uint16_t maxsize = 0;
	    switch (col->protocol) {
		case UDP:
		    /* IP header: 20 bytes
		     * UDP header: 8 bytes */
		    maxsize = col->mtu - (20 + 8);
		    break;
#ifdef SUPPORT_DTLS
		case DTLS_OVER_UDP:
		    /* DTLS record header:
		     *   ContentType: 1 byte
		     *   ProtocolVersion: 2 bytes
		     *   epoch: 2 bytes
		     *   seqno: 6 bytes
		     *   length: 2 bytes
		     *   (assuming GenericBlockCipher, AES_256_CBC and SHA256)
		     *   IV: 16 bytes
		     *   MAC: 32 bytes
		     *   padding: 15 bytes (worst case)
		     *   padding_length: 1 byte */
		    maxsize = col->mtu - 77 - (20 + 8);
		    /* TODO: Find out maximum size of payload */
		    if (maxsize > IPFIX_DTLS_MAX_RECORD_LENGTH)
			maxsize = IPFIX_DTLS_MAX_RECORD_LENGTH;
		    break;
#endif
		default:
		    break;
	    }
	    if (max_message_size > maxsize)
		max_message_size = maxsize;

	}
    }
    exporter->max_message_size = max_message_size;
    DPRINTF("New exporter max_message_size: %u",max_message_size);
}

/* Gets MTU estimate of collector.
 * Calls update_exporter_max_message_size()
 * Calls remove_collector() if an error occurs.
 */
static int update_collector_mtu(ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {
    if (col->protocol == UDP && col->mtu_mode == IPFIX_MTU_DISCOVER) {
	int mtu = get_mtu(col->data_socket);
	DPRINTF("get_mtu() returned %d",mtu);
	if (mtu<0) {
	    remove_collector(col);
	    return -1;
	}
	col->mtu = mtu;
	update_exporter_max_message_size(exporter);
#ifdef SUPPORT_DTLS
    } else if (col->protocol == DTLS_OVER_UDP && col->mtu_mode == IPFIX_MTU_DISCOVER) {
	int mtu = -1;
	int mtu_ssl;
	int mtu_bio;
	if (col->dtls_main.ssl) {
	    mtu_ssl = col->dtls_main.ssl->d1->mtu;
	    DPRINTF("MTU got from SSL object: %d",mtu_ssl);
	    if (mtu_ssl > 0) {
		mtu = mtu_ssl;
	    }
	    mtu_bio = BIO_ctrl(SSL_get_wbio(col->dtls_main.ssl),BIO_CTRL_DGRAM_QUERY_MTU,0,0);
	    DPRINTF("MTU got from BIO object: %d",mtu_bio);
	    if (mtu_bio > 0 && (mtu == -1 || mtu_bio < mtu)) mtu = mtu_bio;
	}
	if (mtu>0) {
	    /* OpenSSL defines the MTU as the maximum payload length
	     * of UDP datagrams.
	     * We define MTU differently: the maximum size of an IP packet.
	     * The difference is 28 bytes (20 bytes for the IP header and
	     * 8 bytes for the UDP header) */
	    col->mtu = mtu + 20 + 8;
	    update_exporter_max_message_size(exporter);
	} else {
	    DPRINTF("Unable to get MTU from SSL object.");
	    remove_collector(col);
	    return -1;
	}
#endif
    }
    return 0;
}

static ipfix_receiving_collector *get_free_collector_slot(ipfix_exporter *exporter) {
    for(int i=0; i<exporter->collector_max_num; i++) {
	if(exporter->collector_arr[i].state == C_UNUSED)
	    return &exporter->collector_arr[i];
    }
    return NULL;
}

static int add_collector_datafile(ipfix_receiving_collector *collector, const char *basename, uint32_t maxfilesize) {
    collector->ipv4address[0] = '\0';
    collector->port_number = 0;
    collector->data_socket = -1;
    collector->protocol = DATAFILE;
    memset(&(collector->addr), 0, sizeof(collector->addr));
    collector->last_reconnect_attempt_time = 0;

    collector->basename = strdup(basename);
    collector->filenum = -1;
    collector->maxfilesize = maxfilesize;
    ipfix_new_file(collector); 
    collector->state = C_CONNECTED;
    return 0;
}

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
static int add_collector_rawdir(ipfix_receiving_collector *collector, const char *path) {
    collector->ipv4address[0] = '\0';
    collector->port_number = 0;
    collector->data_socket = -1;
    memset(&(collector->addr), 0, sizeof(collector->addr));
    collector->last_reconnect_attempt_time = 0;
    collector->protocol = RAWDIR;

    collector->packet_directory_path = strdup(path);
    collector->messages_sent = 0;
    collector->state = C_CONNECTED;
    return 0;
}
#endif
static void set_mtu_config(ipfix_receiving_collector *col,
	ipfix_aux_config_udp *aux_config_udp) {
    if (aux_config_udp && aux_config_udp->mtu>0) {
	col->mtu_mode = IPFIX_MTU_FIXED;
	col->mtu = aux_config_udp->mtu;
    } else {
	col->mtu_mode = IPFIX_MTU_MODE_DEFAULT;
	col->mtu = IPFIX_MTU_DEFAULT;
    }
}

#ifdef SUPPORT_DTLS
static int add_collector_dtls(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	void *aux_config) {

    col->dtls_replacement.socket = -1;
    col->dtls_replacement.ssl = NULL;

    col->dtls_max_connection_lifetime = 0;
    col->dtls_connect_timeout = 30;

    // we need aux_config for setting up a DTLS collector
    if (!aux_config) {
        return -1;
    }

    ipfix_aux_config_dtls *aux_config_dtls;
    if (col->protocol == DTLS_OVER_SCTP) {
	aux_config_dtls = &((ipfix_aux_config_dtls_over_sctp*)aux_config)->dtls;
    } else if (col->protocol == DTLS_OVER_UDP) {
	aux_config_dtls = &((ipfix_aux_config_dtls_over_udp*)aux_config)->dtls;
	col->dtls_max_connection_lifetime = ((ipfix_aux_config_dtls_over_udp*)aux_config)->max_connection_lifetime;
	ipfix_aux_config_udp *aux_config_udp;
	aux_config_udp = &((ipfix_aux_config_dtls_over_udp*)aux_config)->udp;
	/* Sets col->mtu_mode and col->mtu */
	set_mtu_config(col,aux_config_udp);
    } else {
	return -1;
    }
    if (aux_config_dtls->peer_fqdn)
	col->peer_fqdn = strdup(aux_config_dtls->peer_fqdn);

    if (setup_dtls_connection(exporter,col,&col->dtls_main)) {
	/* failure */
	free( (void *) col->peer_fqdn);
	col->peer_fqdn = NULL;
	return -1;
    }
    col->state = C_NEW; /* By setting the state to C_NEW we are
				 basically allocation the slot. */
    col->last_reconnect_attempt_time = time(NULL);
    /* col->state must *not* be C_UNUSED when we call
       update_collector_mtu(). That's why we call this function
       after setting state to C_NEW. */
    if (update_collector_mtu(exporter, col)) {
	/* update_collector_mtu calls remove_collector
	   in case of failure which in turn sets
	   col->state to C_UNUSED and frees col->peer_fqdn. */
	return -1;
    }
    /* We have to call update_exporter_max_message_size()
     * because update_collector_mtu *only* calls
     * update_exporter_max_message_size() if the MTU
     * mode is IPFIX_MTU_DISCOVER. */
    update_exporter_max_message_size(exporter);
    /* Initiate connection setup */
    dtls_manage_connection(exporter, col);
    return 0;
}
#endif

/* Remaining protocols are
   SCTP, UDP and TCP at the moment
 */
static int add_collector_remaining_protocols(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	void *aux_config) {
    if (col->protocol == UDP) {
	ipfix_aux_config_udp *aux_config_udp;
	aux_config_udp = ((ipfix_aux_config_udp*)aux_config);
	/* Sets col->mtu_mode and col->mtu */
        set_mtu_config(col,aux_config_udp);
    }
    // call a separate function for opening the socket.
    // This function can handle both UDP and SCTP sockets.
    col->data_socket = ipfix_init_send_socket(col->addr, col->protocol);
    // error handling, in case we were unable to open the port:
    if(col->data_socket < 0 ) {
	msg(MSG_ERROR, "add collector, initializing socket failed");
	return -1;
    }
    // now, we may set the collector to valid;
    if (col->protocol == UDP)
	col->state = C_CONNECTED; /* UDP sockets are connected from the very
				   beginning. */
    else
	col->state = C_NEW; /* By setting the state to C_NEW we are
			     basically allocation the slot. */

    /* col->state must *not* be C_UNUSED when we call
       update_collector_mtu(). That's why we call this function
       after setting state to C_NEW. */
    if (update_collector_mtu(exporter, col)) {
	/* update_collector_mtu calls remove_collector
	   in case of failure which in turn sets
	   col->state to C_UNUSED. */
	return -1;
    }
    /* We have to call update_exporter_max_message_size()
     * because update_collector_mtu *only* calls
     * update_exporter_max_message_size() if the MTU
     * mode is IPFIX_MTU_DISCOVER. */
    update_exporter_max_message_size(exporter);
    col->last_reconnect_attempt_time = time(NULL);

    return 0;
}

static int valid_transport_protocol(enum ipfix_transport_protocol p) {
    switch(p) {
	case DTLS_OVER_UDP:
#ifdef SUPPORT_DTLS
	    return 1;
#else
	    msg(MSG_FATAL, "Library compiled without DTLS support.");
	    return 0;
#endif
	case DTLS_OVER_SCTP:
#ifdef SUPPORT_DTLS_OVER_SCTP
	    return 1;
#else
	    msg(MSG_FATAL, "Library compiled without DTLS over SCTP support.");
	    return 0;
#endif
	case SCTP:
#ifdef SUPPORT_SCTP
	    return 1;
#else
	    msg(MSG_FATAL, "Library compiled without SCTP support.");
	    return 0;
#endif
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT 
	case RAWDIR:
	    return 1;
#else
	    msg(MSG_FATAL, "Library compiled without RAWDIR support.");
	    return 0;
#endif 

	case TCP:
	    msg(MSG_FATAL, "Transport Protocol TCP not implemented");
	    return 0;
	case UDP:
	case DATAFILE:
	    return 1;
	default:
	    msg(MSG_FATAL, "Transport Protocol not supported");
	    return 0;
    }
}

/*!
 * \brief Add a Collector to the given Exporter and trigger connection setup.
 *
 * Up to IPFIX_MAX_COLLECTORS Collectors may be added to a single Exporter.
 * Data Records are sent to all Collectors in parallel. All active Templates
 * are transmitted to a given Collector before Data Records are sent to the
 * this Collector.
 *
 * If one of the DTLS variants is chosen as the transport protocol, a DTLS
 * handshake with the Collector is initiated.  This handshake procedure may
 * take a while. Because this function never blocks the calling thread, it
 * usually returns before the handshake is completed. In order to give this
 * library a chance to complete the ongoing handshake, the user should call
 * <tt>ipfix_beat()</tt> regularly (e.g., every 10 milliseconds).
 *
 * Depending on the transport protocol, additional configuration data
 * must be passed via the <tt>aux_config</tt> parameter which in turn points to
 * an instance of one of the following types. Check the documentation of the
 * individual types in order to learn about import details which are specific
 * to the chosen transport protocol.
 * <table><tr><td><em>transport protocol</em></td><td><em>type of *aux_config</em></td></tr>
 * <tr><td>RAWDIR</td><td>NULL</td></tr>
 * <tr><td>SCTP</td><td>NULL</td></tr>
 * <tr><td>UDP</td><td>ipfix_aux_config_udp</td></tr>
 * <tr><td>DTLS_OVER_UDP</td><td>ipfix_aux_config_dtls_over_udp</td></tr>
 * <tr><td>DTLS_OVER_SCTP</td><td>ipfix_aux_config_dtls_over_sctp</td></tr>
 * </table>

 * \param exporter pointer to previously initialized exporter struct
 * \param coll_ip4_addr IP address of receiving Collector in dotted notation (e.g. "1.2.3.4")
 * \param coll_port port number of receiving Collector
 * \param proto transport protocol to use, either RAWDIR, SCTP, UDP,
 * DTLS_OVER_UDP or DTLS_OVER_SCTP. See <tt>\ref ipfix_transport_protocol</tt> for
 * more details.
 * \param aux_config auxiliary configuration data. The type of the data structure depends on the
 *	transport protocol chosen. See above.
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_remove_collector()
 */
int ipfix_add_collector(ipfix_exporter *exporter, const char *coll_ip4_addr,
	uint16_t coll_port, enum ipfix_transport_protocol proto, void *aux_config)
{
    // check, if exporter is valid
    if(exporter == NULL) {
	msg(MSG_FATAL, "add_collector, exporter is NULL");
	return -1;
    }
    if ( ! valid_transport_protocol(proto)) return -1;

    // get free slot
    ipfix_receiving_collector *collector = get_free_collector_slot(exporter);
    if( ! collector) {
	msg(MSG_FATAL, "no more free slots for new collectors available, limit %d reached",
		exporter->collector_max_num
	   );
	return -1;
    }

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
    /* It is the duty of add_collector_rawdir to set collector->state */
    if (proto==RAWDIR) return add_collector_rawdir(collector,coll_ip4_addr);
#endif
    if (proto==DATAFILE) return add_collector_datafile(collector, coll_ip4_addr, coll_port);
    /*
    FIXME: only a quick fix to make that work
    Must be copied, else pointered data must be around forever
    Better use binary/u_int32_t representation
    */
    strncpy(collector->ipv4address, coll_ip4_addr, sizeof(collector->ipv4address));
    /* strncpy does not null terminate the destination char array if the
     * length of the source string is equal or greater then the maximum length
     * (third parameter) */
    collector->ipv4address[sizeof(collector->ipv4address)-1] = '\0';
    collector->port_number = coll_port;
    collector->protocol = proto;

    memset(&(collector->addr), 0, sizeof(collector->addr));
    collector->addr.sin_family = AF_INET;
    collector->addr.sin_port = htons(coll_port);
    collector->addr.sin_addr.s_addr = inet_addr(coll_ip4_addr);

#ifdef SUPPORT_DTLS
    /* It is the duty of add_collector_dtls to set collector->state */
    if (proto == DTLS_OVER_UDP || proto == DTLS_OVER_SCTP)
	return add_collector_dtls(exporter, collector, aux_config);
#endif
    return add_collector_remaining_protocols(exporter, collector, aux_config);
}

static void remove_collector(ipfix_receiving_collector *collector) {
    DPRINTF("Removing collector.");
#ifdef SUPPORT_DTLS
    /* Shutdown DTLS connection */
    if (collector->protocol == DTLS_OVER_UDP || collector->protocol == DTLS_OVER_SCTP) {
	dtls_shutdown_and_cleanup(&collector->dtls_main);
	dtls_shutdown_and_cleanup(&collector->dtls_replacement);
	free( (void *) collector->peer_fqdn);
    }
    collector->peer_fqdn = NULL;
#endif
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
    if (collector->protocol != RAWDIR) {
#endif
    if ( collector->data_socket != -1) {
	DPRINTF("Closing data socket");
	close ( collector->data_socket );
    }
    collector->data_socket = -1;
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
    }
    if (collector->protocol == RAWDIR) {
	free(collector->packet_directory_path);
    }
#endif
    if (collector->protocol == DATAFILE) {
	free(collector->basename);
    }
    collector->state = C_UNUSED;
}

/*!
 * \brief Remove a Collector from the exporting process
 *
 * This call removes the given Collector from the set of Collectors. This
 * includes shutting down the transport connection.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param coll_ip4_addr IP address of receiving Collector in dotted notation (e.g. "1.2.3.4")
 * \param coll_port port number of receiving Collector
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_add_collector()
 */
/*
 */
int ipfix_remove_collector(ipfix_exporter *exporter, const char *coll_ip4_addr, uint16_t coll_port) {
    int i;
    for(i=0;i<exporter->collector_max_num;i++) {
	ipfix_receiving_collector *collector = &exporter->collector_arr[i];
	if( ( strcmp( collector->ipv4address, coll_ip4_addr) == 0 )
		&& collector->port_number == coll_port) {
	    remove_collector(collector);
	    return 0;
	}
    }
    msg(MSG_ERROR, "remove_collector, exporter %s not found", coll_ip4_addr);
    return -1;
}

/************************************************************************************/
/* Template management                                                              */
/************************************************************************************/



/*
 * Helper function: Finds a template in the exporter
 * Parameters:
 * exporter: Exporter to search for the template
 * template_id: ID of the template we search
 * Returns: the index of the template in the exporter or -1 on failure.
 */

static int ipfix_find_template(
	ipfix_exporter *exporter,
	uint16_t template_id) {

    DPRINTFL(MSG_VDEBUG, "ipfix_find_template with ID: %d",template_id);

    int i=0;
    for (;i<exporter->ipfix_lo_template_maxsize;i++) {
	if(exporter->template_arr[i].state != T_UNUSED &&
		exporter->template_arr[i].template_id == template_id) {
	    DPRINTFL(MSG_VDEBUG,
		    "ipfix_find_template with ID: %d, validity %d found at %d",
		    template_id, exporter->template_arr[i].state, i);
	    return i;
	}
    }
    return -1;
}

static int ipfix_get_free_template_slot(ipfix_exporter *exporter) {
    DPRINTFL(MSG_VDEBUG, "ipfix_get_free_template_slot");

    int i=0;
    for (;i<exporter->ipfix_lo_template_maxsize;i++) {
	if(exporter->template_arr[i].state == T_UNUSED) {
	    DPRINTFL(MSG_VDEBUG, "ipfix_get_free_template_slot found at %d",i);
	    return i;
	}
    }
    DPRINTFL(MSG_DEBUG, "ipfix_get_free_template_slot failed.");
    return -1;
}

/*!
 * \brief Remove a Template from the Exporting process but create a withdrawal message first
 *
 * This will free the templates data store!
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID of the template to remove
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_start_template()
 */
/*
 */
int ipfix_remove_template(ipfix_exporter *exporter, uint16_t template_id) {
    int found_index = ipfix_find_template(exporter,template_id);
    if (found_index < 0) {
	msg(MSG_ERROR, "remove_template ID %u not found", template_id);
	return -1;
    }
    if(exporter->template_arr[found_index].state == T_SENT){
	DPRINTFL(MSG_VDEBUG,
		"creating withdrawal msg for ID: %d, validity %d",
		template_id, exporter->template_arr[found_index].state);
	char *p_pos;
	char *p_end;

	// write the withdrawal message fields into the buffer
	// beginning of the buffer
	p_pos = exporter->template_arr[found_index].template_fields;
	// end of the buffer since the WITHDRAWAL message for one template is always 8 byte
	p_end = p_pos + 8;

	// set ID is 2 for a template, 4 for a template with fixed fields:
	// for withdrawal messages we keep the template set ID
	p_pos +=  2;
	// write 8 to the length field
	write_unsigned16 (&p_pos, p_end, 8);
	// keep the template ID:
	p_pos +=  2;
	// write 0 for the field count, since it indicates that this is a withdrawal message
	write_unsigned16 (&p_pos, p_end, 0);
	exporter->template_arr[found_index].fields_length = 8;
	exporter->template_arr[found_index].field_count = 0;
	exporter->template_arr[found_index].fixedfield_count = 0;
	exporter->template_arr[found_index].fields_added = 0;
	exporter->template_arr[found_index].state = T_WITHDRAWN;
	DPRINTFL(MSG_VDEBUG, "... Withdrawn");
    } else {
	ipfix_deinit_template(&(exporter->template_arr[found_index]) );
    }
    return 0;
}

/************************************************************************************/
/* End of Template management                                                       */
/************************************************************************************/



/*
 * Update the ipfix message header in the sendbuffer
 *
 * The ipfix message header is set according to:
 * - the exporter ( Source ID and sequence number)
 * - the length of the contained data
 * - the current system time
 * - the ipfix version number
 *
 * Note: the first HEADER_USED_IOVEC_COUNT  iovec struct are reserved for the header! These will be overwritten!
 */
static void ipfix_update_header(ipfix_exporter *p_exporter, ipfix_receiving_collector *collector, ipfix_sendbuffer *sendbuf)
{
    struct timeval now;
    if (gettimeofday(&now, NULL)) {
	now.tv_sec = 0;
	now.tv_usec= 0;
	msg(MSG_ERROR,"update_header, gettimeofday() failed. Set current time to 0");
    }

    switch(p_exporter->export_protocol) {
    case NFV9_PROTOCOL:
	sendbuf->nfv9_message_header.version = htons(NFV9_PROTOCOL);
        sendbuf->nfv9_message_header.source_id = htonl(p_exporter->observation_domain_id);

	// Time fields
	uint32_t millisec_uptime = ((now.tv_sec * 1000) + (now.tv_usec / 1000))
	    - (((p_exporter->start_time).tv_sec * 1000) + ((p_exporter->start_time).tv_usec / 1000));
        sendbuf->nfv9_message_header.unix_secs = htonl((uint32_t)now.tv_sec);
        sendbuf->nfv9_message_header.sysUpTime = htonl(millisec_uptime);

	sendbuf->nfv9_message_header.count = htons(sendbuf->record_count);
	sendbuf->nfv9_message_header.sequence_number = htonl(collector->messages_sent);
	break;

    case IPFIX_PROTOCOL:
	sendbuf->ipfix_message_header.version = htons(IPFIX_PROTOCOL);
        sendbuf->ipfix_message_header.observation_domain_id = htonl(p_exporter->observation_domain_id);

	// Time field
	sendbuf->ipfix_message_header.export_time = htonl((uint32_t)now.tv_sec);

	uint16_t total_length = 0;
        // Is the length already computed?
        if (sendbuf->committed_data_length != 0) {
                total_length = sendbuf->committed_data_length + sizeof(ipfix_header);
        } else {
                // compute it on our own:
                // sum up all lengths in the iovecs:
                unsigned int i;

                // start the loop with 1, as 0 is reserved for the header!
		/// @todo Include iovec 0 for the header as it should be set and contain a length
                for (i = 1; i< sendbuf->current;  i++) {
                        total_length += sendbuf->entries[i].iov_len;
                }

                // add the header length to the total length:
                total_length += sizeof(ipfix_header);
        }

        // write the length into the header
        sendbuf->ipfix_message_header.length = htons(total_length);

        // write the sequence number into the header
        sendbuf->ipfix_message_header.sequence_number = htonl(p_exporter->sequence_number);
	break;

    default:
	msg(MSG_ERROR, "Cannot update a header for unknown protocol");
	break;
    }

}


/*create a new filehandle and set recvcoll->fh, recvcoll->byteswritten, recvcoll->filenum 
 * to their new values
 * returns the newly created filehandle*/
static int ipfix_new_file(ipfix_receiving_collector* recvcoll){
	int f = -1;
	if (recvcoll->fh > 0) close(recvcoll->fh);
	recvcoll->filenum++;
	recvcoll->bytes_written = 0;

	/*11 == maximum length of uint32_t including terminating \0*/
	char *filename = malloc(sizeof(char)*(strlen(recvcoll->basename)+11)); 
	if(! filename){
		msg(MSG_ERROR, "could not malloc filename\n");
		goto out;
	}
	sprintf(filename, "%s%010d", recvcoll->basename, recvcoll->filenum);
	while(1){
		f = open(filename, O_WRONLY | O_CREAT | O_EXCL,
					 S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP);
		if (f<0) { 
			if (errno == EEXIST){ //increase the filenumber and try again
				recvcoll->filenum++; //if the current file already exists
				msg(MSG_VDEBUG, "Skipping %s", filename);
				sprintf(filename, "%s%010d", recvcoll->basename, recvcoll->filenum);
				continue;
			}
			msg(MSG_ERROR, "could not open DATAFILE file %s", filename);
			f = -1;
			goto out;
		}
		break;
	}
	msg(MSG_INFO, "Created new file: %s", filename);
out:
	free(filename);
	recvcoll->fh = f;
	return f;
}


/*
 * Create and initialize an ipfix_sendbuffer for at most maxelements
 * Parameters: ipfix_sendbuffer** sendbuf pointer to a pointer to an ipfix-sendbuffer
 */
static int ipfix_init_sendbuffer(export_protocol_version export_protocol, ipfix_sendbuffer **sendbuf)
{
        ipfix_sendbuffer *tmp;

        // mallocate memory for the sendbuffer
        if(!(tmp=(ipfix_sendbuffer *)malloc(sizeof(ipfix_sendbuffer)))) {
                goto out;
        }

        tmp->current = HEADER_USED_IOVEC_COUNT; // leave the 0th field blank for the header
        tmp->committed = HEADER_USED_IOVEC_COUNT;
        tmp->marker = HEADER_USED_IOVEC_COUNT;
        tmp->committed_data_length = 0;
        tmp->record_count = 0;

	// link the appropriate message header to the start of the buffer
	switch(export_protocol) {
	case NFV9_PROTOCOL:
	    memset(&(tmp->nfv9_message_header), 0, sizeof(nfv9_header));
	    tmp->entries[0].iov_len = sizeof(nfv9_header);
	    tmp->entries[0].iov_base = &(tmp->nfv9_message_header);
	    break;
	case IPFIX_PROTOCOL:
	    memset(&(tmp->ipfix_message_header), 0, sizeof(ipfix_header));
	    tmp->entries[0].iov_len = sizeof(ipfix_header);
	    tmp->entries[0].iov_base = &(tmp->ipfix_message_header);
	    break;
	default:
	    goto out1;
	    break;
	}

        // initialize an ipfix_set_manager
	(tmp->set_manager).set_counter = 0;
        memset(&(tmp->set_manager).set_header_store, 0, sizeof((tmp->set_manager).set_header_store));
        (tmp->set_manager).data_length = 0;

        *sendbuf=tmp;
        return 0;

out1:
        free(tmp);
out:
        return -1;
}

/*
 * reset ipfix_sendbuffer
 * Resets the contents of an ipfix_sendbuffer, so the sendbuffer can again
 * be filled with data.
 * (Present headers are also purged).
 */
static int ipfix_reset_sendbuffer(ipfix_sendbuffer *sendbuf)
{
        sendbuf->current = HEADER_USED_IOVEC_COUNT;
        sendbuf->committed = HEADER_USED_IOVEC_COUNT;
        sendbuf->marker = HEADER_USED_IOVEC_COUNT;
        sendbuf->committed_data_length = 0;
        sendbuf->record_count = 0;

        memset(&(sendbuf->ipfix_message_header), 0, sizeof(ipfix_header));
        memset(&(sendbuf->nfv9_message_header), 0, sizeof(nfv9_header));

        // also reset the set_manager!
	(sendbuf->set_manager).set_counter = 0;
        memset(&(sendbuf->set_manager).set_header_store, 0, sizeof((sendbuf->set_manager).set_header_store));
        (sendbuf->set_manager).data_length = 0;

        return 0;
}


/*
 * Deinitialize (free) an ipfix_sendbuffer
 */
static int ipfix_deinit_sendbuffer(ipfix_sendbuffer **sendbuf)
{
        // free the sendbuffer itself:
        free(*sendbuf);
        *sendbuf = NULL;

        return 0;
}


/*
 * initialize array of collectors
 * Allocates memory for an array of collectors
 * Parameters:
 * col: collector array to initialize
 * col_capacity: maximum amount of collectors to store in this array
 */
static int ipfix_init_collector_array(ipfix_receiving_collector **col, int col_capacity)
{
        int i;
        ipfix_receiving_collector *tmp;

        tmp=(ipfix_receiving_collector *)malloc((sizeof(ipfix_receiving_collector) * col_capacity));
        if(!tmp) {
                return -1;
        }

        for (i = 0; i< col_capacity; i++) {
		ipfix_receiving_collector *c = &tmp[i];
                c->state = C_UNUSED;
		c->ipv4address[0] = '\0';
		c->port_number = 0;
		c->protocol = 0;
		c->data_socket = -1;
		c->last_reconnect_attempt_time = 0;
		c->messages_sent = 0;
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
		c->packet_directory_path = NULL;
#endif
#ifdef SUPPORT_DTLS
		c->dtls_main.socket = c->dtls_replacement.socket = -1;
		c->dtls_main.ssl = c->dtls_replacement.ssl = NULL;
		c->dtls_main.last_reconnect_attempt_time =
		    c->dtls_replacement.last_reconnect_attempt_time = 0;
		c->peer_fqdn = NULL;
#endif

        }

        *col=tmp;
        return 0;
}


/*
 * deinitialize an array of collectors
 * Parameters:
 * col: collector array to clean up
 */
static int ipfix_deinit_collector_array(ipfix_receiving_collector **col)
{
        free(*col);
        *col=NULL;

        return 0;
}


/*
 * Initializes a send socket
 * Parameters:
 * serv_ip4_addr of the recipient in dot notation
 * serv_port: port
 * protocol: transport protocol
 */
static int ipfix_init_send_socket(struct sockaddr_in serv_addr, enum ipfix_transport_protocol protocol)
{
    int sock = -1;

    switch(protocol) {
	case UDP:
	case DTLS_OVER_UDP:
	    sock= init_send_udp_socket( serv_addr );
	    break;

#ifdef SUPPORT_SCTP
	case SCTP:
	case DTLS_OVER_SCTP:
	    sock= init_send_sctp_socket( serv_addr );
	    break;
#endif
	default:
	    return -1; /* Should not occur since we check the transport
			  protocol in valid_transport_protocol()*/
    }

    return sock;
}

/*
 * initialize array of templates
 * Allocates memory for an array of templates
 * Parameters:
 * exporter: exporter, whose template array we'll initialize
 * template_capacity: maximum amount of templates to store in this array
 */
static int ipfix_init_template_array(ipfix_exporter *exporter, int template_capacity)
{
        int i;

        // allocate the memory for template_capacity elements:
        exporter->ipfix_lo_template_maxsize  = template_capacity;
        exporter->template_arr =  (ipfix_lo_template*) malloc (template_capacity * sizeof(ipfix_lo_template) );

        for(i = 0; i< template_capacity; i++) {
                exporter->template_arr[i].state = T_UNUSED;
        }

        return 0;
}


/*
 * deinitialize an array of templates
 * Parameters:
 * exporter: exporter, whose template store will be purged
 */
static int ipfix_deinit_template_array(ipfix_exporter *exporter)
{
        /* FIXME: free all templates in the array!
         This was our memory leak.
         JanP, 2005-21-1
         */
        int i;
        
        for(i=0; i< exporter->ipfix_lo_template_maxsize; i++) {
                // if template was sent we need a withdrawal message first
                if (exporter->template_arr[i].state == T_SENT){
                 	ipfix_remove_template(exporter, exporter->template_arr[i].template_id );
                }
        }
        // send all created withdrawal messages
        ipfix_send_templates(exporter);
        
	for(i=0; i< exporter->ipfix_lo_template_maxsize; i++) {
                // try to free all templates:
	    if (ipfix_deinit_template(&(exporter->template_arr[i]) )) {
                msg(MSG_ERROR, "failed to deinitialize template %i", i);
	    }
        }
        free(exporter->template_arr);

        exporter->template_arr = NULL;
        exporter->ipfix_lo_template_maxsize = 0;

        return 0;
}


/*
 * Updates the template sendbuffers
 * will be called, after a template has been added or removed
 *
 * Watch out: By calling this function you undertake a commitment!
 * The commitment is that you send out sctp_sendbuf to all SCTP collectors!
 * This function alters the state of all templates that are
 * in state T_COMMITED to state T_SENT. In the SCTP case templates
 * will be copied to sctp_sendbuf only once and this is when this state
 * transition takes place.
 * So if you call this function and do not send out sctp_sendbuf afterwards
 * the affected templates will never be sent because this state transition
 * takes place only once.
 */
static int ipfix_update_template_sendbuffer (ipfix_exporter *exporter)
{
        int i;

        ipfix_sendbuffer* t_sendbuf = exporter->template_sendbuffer;
	ipfix_sendbuffer* sctp_sendbuf = exporter->sctp_template_sendbuffer;

        // clean the template sendbuffers
        ipfix_reset_sendbuffer(t_sendbuf);
	ipfix_reset_sendbuffer(sctp_sendbuf);

        // place all valid templates into the template sendbuffer
        // could be done just like put_data_field:

        for (i = 0; i < exporter->ipfix_lo_template_maxsize; i++ )  {
                switch (exporter->template_arr[i].state) {
                	case (T_TOBEDELETED):
				// free memory and mark T_UNUSED
				ipfix_deinit_template(&(exporter->template_arr[i]) );
				break;
			case (T_COMMITED): // send to SCTP and UDP collectors and mark as T_SENT
				if (sctp_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "SCTP template sendbuffer too small to handle more than %i entries", sctp_sendbuf->current);
					return -1;
                        	}
                        	if (t_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "UDP template sendbuffer too small to handle more than %i entries", t_sendbuf->current);
					return -1;
                        	}
				sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
                        	sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
                        	sctp_sendbuf->current++;
                        	sctp_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				sctp_sendbuf->record_count++;
				
				t_sendbuf->entries[ t_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
				t_sendbuf->entries[ t_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
				t_sendbuf->current++;
				t_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				t_sendbuf->record_count++;
                        	
                        	exporter->template_arr[i].state = T_SENT;
                        	break;
                	case (T_SENT): // only to UDP collectors
                		if (t_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "UDP template sendbuffer too small to handle more than %i entries", t_sendbuf->current);
					return -1;
                        	}
                		t_sendbuf->entries[ t_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
				t_sendbuf->entries[ t_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
				t_sendbuf->current++;
				t_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				t_sendbuf->record_count++;
				break;
                	case (T_WITHDRAWN): // put the SCTP withdrawal message and mark T_TOBEDELETED
                		if (sctp_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
					msg(MSG_ERROR, "SCTP template sendbuffer too small to handle more than %i entries", sctp_sendbuf->current);
					return -1;
				}
				sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
				sctp_sendbuf->entries[ sctp_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
				sctp_sendbuf->current++;
				sctp_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
				sctp_sendbuf->record_count++;
				
				/* ASK: Why don't we just delete the template? */
				exporter->template_arr[i].state = T_TOBEDELETED;
				DPRINTFL(MSG_VDEBUG, "Withdrawal for template ID: %d added to sctp_sendbuffer", exporter->template_arr[i].template_id);
				break;
			default : // Do nothing : T_UNUSED or T_UNCLEAN
				break;
                }
        } // end loop over all templates

        // that's it!
        return 0;
}

#ifdef SUPPORT_SCTP
/*
 * function used by SCTP to reconnect to a collector, if connection
 * was lost.
 */
static int sctp_reconnect(ipfix_receiving_collector *collector){
	int ret, error;
	socklen_t len;
	fd_set readfds;
	struct timeval timeout;
	time_t time_now = time(NULL);
	struct sctp_status ss;
	union sctp_notification snp;
	struct sctp_assoc_change *sac;
	struct msghdr msg;
	struct iovec iv;
	ssize_t r;

	collector->last_reconnect_attempt_time = time_now;
	// error occurred while being connected?
	if(collector->state == C_CONNECTED) {
		// the socket has not yet been closed
		close(collector->data_socket);
		collector->data_socket = -1;
	}
	    
	// create new socket if not yet done
	if(collector->data_socket < 0) {
		collector->data_socket = init_send_sctp_socket( collector->addr );
		if( collector->data_socket < 0) {
		    msg(MSG_ERROR, "SCTP socket creation in reconnect failed, %s", strerror(errno));
		    collector->state = C_DISCONNECTED;
		    return -1;
		}
		collector->state = C_NEW;
	}
	/* Determine whether socket is readable.

	   If it is readable, we can query the result of the connection
	   setup. The result can be either success of failure.

	   If the socket is not yet readable, the connection setup is
	   still ongoing.
	*/
	/* We don't want select() to wait but to return immediately.
	   Set timeout to 0. */
	timeout.tv_sec = timeout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(collector->data_socket, &readfds);
	ret = select(collector->data_socket + 1,&readfds,NULL,NULL,&timeout);
	if (ret == 0) {
	    // connection attempt not yet finished
	    msg(MSG_DEBUG, "waiting for socket to become readable...");
	    collector->state = C_NEW;
	    return -1;
	} else if (ret>0) {
	    // connected or connection setup failed.
	    msg(MSG_DEBUG, "socket is readable.");
	} else {
	    // error
	    msg(MSG_ERROR, "select() failed: %s", strerror(errno));
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}

	/* Query pending error */
	len = sizeof error;
	if (getsockopt(collector->data_socket, SOL_SOCKET,
		    SO_ERROR, &error, &len) != 0) {
	    msg(MSG_ERROR, "getsockopt(fd,SOL_SOCKET,SO_ERROR,...) failed: %s",
		    strerror(errno));
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}
	if (error) {
	    msg(MSG_ERROR, "SCTP connection setup failed: %s",
		    strerror(error));
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}

	/* Read from socket. Expect a SCTP_ASSOC_CHANGE with
	   sac_state==SCTP_COMM_UP */

	iv.iov_base = &snp;
	iv.iov_len = sizeof snp;

	memset(&msg,0,sizeof(msg));
	msg.msg_iov = &iv;
	msg.msg_iovlen = 1;
	if ((r = recvmsg(collector->data_socket, &msg, 0))<0) {
	    msg(MSG_ERROR, "SCTP connection setup failed. recvmsg returned: %s",
		    strerror(error));
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}
	if (r==0) {
	    msg(MSG_ERROR, "SCTP connection setup failed. recvmsg returned 0");
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}
	if (!(msg.msg_flags & MSG_NOTIFICATION)) {
	    msg(MSG_ERROR, "SCTP connection setup failed. recvmsg unexpected user data.");
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}
	switch (snp.sn_header.sn_type) {
	    case SCTP_ASSOC_CHANGE:
		sac = &snp.sn_assoc_change;
		if (!sac->sac_state==SCTP_COMM_UP) {
		    msg(MSG_ERROR, "SCTP connection setup failed. "
			    "Received unexpected SCTP_ASSOC_CHANGE notification with state %d",
			    sac->sac_state);
		    close(collector->data_socket);
		    collector->data_socket = -1;
		    collector->state = C_DISCONNECTED;
		    return -1;
		}
		msg(MSG_DEBUG,"Received SCTP_COMM_UP event.");
		break;
	    default:
		msg(MSG_ERROR, "SCTP connection setup failed. "
			"Received unexpected notification of type %d",
			snp.sn_header.sn_type);
		close(collector->data_socket);
		collector->data_socket = -1;
		collector->state = C_DISCONNECTED;
		return -1;
	}

	/* Query SCTP status */
	len = sizeof ss;
	if (getsockopt(collector->data_socket, IPPROTO_SCTP,
		    SCTP_STATUS, &ss, &len) != 0) {
	    msg(MSG_ERROR, "getsockopt(fd,IPPROTO_SCTP,SCTP_STATUS,...) failed: %s",
		    strerror(errno));
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}
	/* Make sure SCTP connection is in state ESTABLISHED */
	if (ss.sstat_state != SCTP_ESTABLISHED) {
	    msg(MSG_ERROR, "SCTP socket not in state ESTABLISHED");
	    close(collector->data_socket);
	    collector->data_socket = -1;
	    collector->state = C_DISCONNECTED;
	    return -1;
	}

	msg(MSG_INFO, "Successfully (re)connected to SCTP collector.");
	collector->state = C_CONNECTED;
	return 0;
}
#endif /*SUPPORT_SCTP*/

/*******************************************************************/
/* Transmission                                                    */
/*******************************************************************/

static bool ipfix_write_sendbuffer_to_datafile(ipfix_sendbuffer *sendbuffer, ipfix_receiving_collector *col)
{
    ssize_t nwritten = 0;
    // @todo For V9 there is no immediate access to message length, need to calculate it.
    if(col->bytes_written>0 && (col->bytes_written +
				ntohs(sendbuffer->ipfix_message_header.length)
				> (uint64_t)(col->maxfilesize) * 1024)) {
	ipfix_new_file(col);
    }

    if (col->fh < 0) {
	msg(MSG_ERROR, "invalid file handle for DATAFILE file (==0!)");
	return false;
    }
    if (sendbuffer->ipfix_message_header.length == 0) {
	msg(MSG_ERROR, "packet size == 0!");
	return false;
    }
    if ((nwritten = writev(col->fh, sendbuffer->entries, sendbuffer->current)) < 0) {
	msg(MSG_ERROR, "could not write to DATAFILE file");
	return false;
    }
    col->bytes_written += nwritten;
    msg(MSG_DEBUG, "bytes_written: %d \t Total: %llu", nwritten, col->bytes_written);
    return true;
}

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
static void ipfix_write_sendbuffer_to_rawdir(ipfix_sendbuffer *sendbuffer, ipfix_receiving_collector *col)
{
    char fnamebuf[1024];
    sprintf(fnamebuf, "%s/%08d", col->packet_directory_path, col->messages_sent);
    int f = creat(fnamebuf, S_IRWXU | S_IRWXG);
    if(f<0) {
	msg(MSG_ERROR, "could not open RAWDIR file %s", fnamebuf);
    } else if(writev(f, sendbuffer->entries, sendbuffer->current)<0) {
	msg(MSG_ERROR, "could not write to RAWDIR file %s", fnamebuf);
    }
    close(f);
}
#endif

/*
 * If necessary, sends all associated templates
 * Parameters:
 *  exporter sending exporting process
 * Return value: 1 on success, -1 on failure, 0 on no need to send.
 */
static int ipfix_send_templates(ipfix_exporter* exporter)
{
    int i;
    int bytes_sent;
    // determine, if we need to send the template data:
    time_t time_now = time(NULL);
    // has the timer expired? (for UDP and DTLS over UDP)
    // Remember: This is a global timer for all collectors associated with a given exporter
    bool expired = ( (time_now - exporter->last_template_transmission_time) >  exporter->template_transmission_timer);

    // update the sendbuffers
    // Watch out: You undertake a commitment by calling this function
    // See the definition of the function for more details.
    ipfix_update_template_sendbuffer(exporter);

    // send the sendbuffer to all collectors depending on their protocol
    for (i = 0; i < exporter->collector_max_num; i++) {
	ipfix_receiving_collector *col = &exporter->collector_arr[i];
	// is the collector a valid target?
	if (col->state == C_UNUSED) {
	    continue; // No. Continue to next loop iteration.
	}
#ifdef SUPPORT_DTLS
	if (col->protocol == DTLS_OVER_UDP ||
	    col->protocol == DTLS_OVER_SCTP) {
	    /* ensure that we are connected i.e. DTLS handshake has been finished.
	     * This function does no harm if we are already connected. */
	    if (dtls_manage_connection(exporter,col) < 0)
		/* continue if dtls_manage_connection failed */
		continue;
	    /* dtls_manage_connection() might return success even if we're not yet connected.
	     * This might happen if OpenSSL is still waiting for data from the
	     * remote end and therefore returned SSL_ERROR_WANT_READ. */
	    if ( col->state != C_CONNECTED ) {
		DPRINTF("We are not yet connected so we can't send templates.");
		break;
	    }
	}
#endif
	switch(col->protocol){
#ifdef SUPPORT_DTLS_OVER_SCTP
	case DTLS_OVER_SCTP:
	    if (exporter->sctp_template_sendbuffer->committed_data_length > 0) {
		// update the sendbuffer header, as we must set the export time & sequence number!
		ipfix_update_header(exporter, col,
				    exporter->sctp_template_sendbuffer);
		dtls_over_sctp_send(exporter,col,
				    exporter->sctp_template_sendbuffer->entries,
				    exporter->sctp_template_sendbuffer->current,
				    0);//packet lifetime in ms (0 = reliable, do not change for templates)
		col->messages_sent++;
	    }
	    break;
#endif
#ifdef SUPPORT_DTLS
	case DTLS_OVER_UDP:
	    if (expired) {
		exporter->last_template_transmission_time = time_now;
		dtls_send_templates(exporter, col);
	    }
	    break;
#endif
	case UDP:
	    if (expired && (exporter->template_sendbuffer->committed_data_length > 0)){
		exporter->last_template_transmission_time = time_now;
		// update the sendbuffer header, as we must set the export time & sequence number!
		ipfix_update_header(exporter, col,
				    exporter->template_sendbuffer);
		if((bytes_sent = writev(col->data_socket,
					exporter->template_sendbuffer->entries,
					exporter->template_sendbuffer->current
			))  == -1){
		    if (errno == EMSGSIZE) {
			msg(MSG_ERROR,
			    "Unable to send templates to %s:%d b/c message is bigger than MTU. That is a severe problem.",
			    col->ipv4address,
			    col->port_number);
		    } else {
			msg(MSG_ERROR,
			    "could not send templates to %s:%d errno: %s  (UDP)",
			    col->ipv4address,
			    col->port_number,
			    strerror(errno));
		    }
		} else {
		    msg(MSG_VDEBUG, "%d Template Bytes sent to UDP collector %s:%d",
			bytes_sent, col->ipv4address, col->port_number);
		}
		col->messages_sent++;
	    }
	    break;

#ifdef SUPPORT_SCTP
	case SCTP:
	    switch (col->state){
	    case C_DISCONNECTED:
		if(exporter->sctp_reconnect_timer == 0) { // 0 = no more reconnection attempts
		    msg(MSG_ERROR, "reconnect failed, removing collector %s:%d (SCTP)", col->ipv4address, col->port_number);
		    remove_collector(col);
		    break;
		}
		if ((time_now - col->last_reconnect_attempt_time) <=  exporter->sctp_reconnect_timer) {
		    break; // Not time to reconnect
		}
		// fall through
	    case C_NEW:
		sctp_reconnect(col);
		// fall through
	    case C_CONNECTED:
		if (exporter->sctp_template_sendbuffer->committed_data_length > 0) {
		    // update the sendbuffer header, as we must set the export time & sequence number!
		    ipfix_update_header(exporter, col,
					exporter->sctp_template_sendbuffer);
		    if((bytes_sent = sctp_sendmsgv(col->data_socket,
						   exporter->sctp_template_sendbuffer->entries,
						   exporter->sctp_template_sendbuffer->current,
						   (struct sockaddr*)&(col->addr),
						   sizeof(col->addr),
						   0,0, // payload protocol identifier, flags
						   0,//Stream Number
						   0,//packet lifetime in ms (0 = reliable, do not change for templates)
						   0 // context
			    )) == -1) {
			// send failed
			msg(MSG_ERROR, "could not send templates to %s:%d errno: %s  (SCTP)",col->ipv4address, col->port_number, strerror(errno));
			sctp_reconnect(col); //1st reconnect attempt
			// if result is C_DISCONNECTED and sctp_reconnect_timer == 0, collector will
			// be removed on the next call of ipfix_send_templates()
		    } else {
			// send was successful
			msg(MSG_VDEBUG, "%d template bytes sent to SCTP collector %s:%d",
			    bytes_sent, col->ipv4address, col->port_number);
		    }
		} else {
		    DPRINTF("No Template to send to SCTP collector");
		}
		col->messages_sent++;
		break;
	    default:
		msg(MSG_FATAL, "Unknown collector socket state");
		return -1;
	    }
	    break;
#endif

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	case RAWDIR:
	    ipfix_update_header(exporter, col,
				exporter->template_sendbuffer);
	    ipfix_write_sendbuffer_to_rawdir(exporter->template_sendbuffer, col);
	    col->messages_sent++;
	    break;
#endif
	case DATAFILE:
	    if (exporter->template_sendbuffer->committed_data_length > 0) {
		ipfix_update_header(exporter, col,
				    exporter->template_sendbuffer);
		ipfix_write_sendbuffer_to_datafile(exporter->template_sendbuffer, col);
		col->messages_sent++;
	    }
	    break;
	default:
	    return -1; /* Should not occur since we check the transport
			  protocol in valid_transport_protocol()*/
	}
    } // end exporter loop

    return 1;
}

#ifdef DEBUG
static void ipfix_sendbuffer_debug(ipfix_sendbuffer *sendbuffer)
{
    // debugging output of data buffer:
    DPRINTFL(MSG_VDEBUG, "Sendbuffer contains %u bytes (Set headers + records)",  sendbuffer->committed_data_length );
    DPRINTFL(MSG_VDEBUG, "Sendbuffer contains %u fields (IPFIX Message header + set headers + records)", sendbuffer->committed );
    int tested_length = 0;
    for (unsigned int j =0; j <  sendbuffer->committed; j++) {
	if(sendbuffer->entries[j].iov_len > 0 ) {
	    tested_length += sendbuffer->entries[j].iov_len;
	}
    }
    /* Keep in mind that the IPFIX message header (16 bytes) is not included
       in committed_data_length. So there should be a difference of 16 bytes
       between tested_length and committed_data_length */
    DPRINTFL(MSG_VDEBUG, "Total length of sendbuffer: %u bytes (IPFIX Message header + set headers + records)", tested_length );
}
#endif

/*
 Send data to collectors
 Sends all data committed via ipfix_put_data_field to this exporter.
 Parameters:
 exporter sending exporting process

 Return value:
 on success: 0
 on failure: -1
 */
static int ipfix_send_data(ipfix_exporter* exporter)
{
    int bytes_sent;
    // send the current data_sendbuffer if there is data
    if (exporter->data_sendbuffer->committed_data_length > 0 ) {
	// send the sendbuffer to all collectors
	for (int i = 0; i < exporter->collector_max_num; i++) {
	    ipfix_receiving_collector *col = &exporter->collector_arr[i];
	    // update the header in the sendbuffer
	    ipfix_update_header(exporter, col, exporter->data_sendbuffer);
	    // is the collector a valid target?
	    if (col->state != C_CONNECTED) {
		continue; // No. Continue to next loop iteration.
	    }
#ifdef DEBUG
	    DPRINTFL(MSG_VDEBUG, "Sending to exporter %s", col->ipv4address);
	    ipfix_sendbuffer_debug(exporter->data_sendbuffer);
#endif
	    switch(col->protocol){
	    case UDP:
		if((bytes_sent=writev( col->data_socket,
				       exporter->data_sendbuffer->entries,
				       exporter->data_sendbuffer->committed
			)) == -1){
		    msg(MSG_ERROR, "could not send data to %s:%d errno: %s  (UDP)",col->ipv4address, col->port_number, strerror(errno));
		    if (errno == EMSGSIZE) {
			msg(MSG_ERROR, "Updating MTU estimate for collector %s:%d",
			    col->ipv4address,
			    col->port_number);
			/* If update_collector_mtu fails, it calls
			   remove_collector(). So keep in mind that
			   the collector might be gone (set to C_UNUSED)
			   after calling this function. */
			update_collector_mtu(exporter, col);
		    }
		}else{

		    msg(MSG_VDEBUG, "%d data bytes sent to UDP collector %s:%d",
			bytes_sent, col->ipv4address, col->port_number);
		}
		break;
#ifdef SUPPORT_DTLS_OVER_SCTP
	    case DTLS_OVER_SCTP:
		if((bytes_sent=dtls_over_sctp_send( exporter, col,
						    exporter->data_sendbuffer->entries,
						    exporter->data_sendbuffer->committed,
						    exporter->sctp_lifetime
			)) == -1){
		    msg(MSG_VDEBUG, "could not send data to %s:%d (DTLS over SCTP)",col->ipv4address, col->port_number);
		}else{
		    msg(MSG_VDEBUG, "%d data bytes sent to DTLS over SCTP collector %s:%d",
			bytes_sent, col->ipv4address, col->port_number);
		}
		break;
#endif

#ifdef SUPPORT_SCTP
	    case SCTP:
		if((bytes_sent = sctp_sendmsgv(col->data_socket,
					       exporter->data_sendbuffer->entries,
					       exporter->data_sendbuffer->committed,
					       (struct sockaddr*)&(col->addr),
					       sizeof(col->addr),
					       0,0, // payload protocol identifier, flags
					       0,//Stream Number
					       exporter->sctp_lifetime,//packet lifetime in ms(0 = reliable )
					       0 // context
			)) == -1) {
		    // send failed
		    msg(MSG_ERROR, "could not send data to %s:%d errno: %s  (SCTP)",col->ipv4address, col->port_number, strerror(errno));
		    // drop data and call sctp_reconnect
		    sctp_reconnect(col);
		    // if result is C_DISCONNECTED and sctp_reconnect_timer == 0, collector will 
		    // be removed on the next call of ipfix_send_templates()
		}
		msg(MSG_VDEBUG, "%d data bytes sent to SCTP collector %s:%d",
		    bytes_sent, col->ipv4address, col->port_number);
		break;
#endif
#ifdef SUPPORT_DTLS
	    case DTLS_OVER_UDP:
		if((bytes_sent=dtls_send( exporter, col,
					  exporter->data_sendbuffer->entries,
					  exporter->data_sendbuffer->committed
			)) == -1){
		    msg(MSG_VDEBUG, "could not send data to %s:%d (DTLS over UDP)",col->ipv4address, col->port_number);
		}else{
		    msg(MSG_VDEBUG, "%d data bytes sent to DTLS over UDP collector %s:%d",
			bytes_sent, col->ipv4address, col->port_number);
		}
		break;
#endif

#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	    case RAWDIR:
		ipfix_write_sendbuffer_to_rawdir(exporter->data_sendbuffer, col);
		break;
#endif
	    case DATAFILE:
		ipfix_write_sendbuffer_to_datafile(exporter->data_sendbuffer, col);
		break;

	    default:
		msg(MSG_FATAL, "Transport Protocol not supported");
		break; /* Should not occur since we check the transport
			  protocol in valid_transport_protocol()*/
	    }
	    col->messages_sent++;
	} // end exporter loop
	// increment sequence number
	exporter->sequence_number += exporter->sn_increment;
	exporter->sn_increment = 0;
    }  // end if

    // reset the sendbuffer
    ipfix_reset_sendbuffer(exporter->data_sendbuffer);
    return 0;
}


/*!
 * \brief Send data to Collectors.
 *
 * Sends all data committed via <tt>ipfix_put_data_field</tt> to this exporter.
 * If necessary, sends all associated templates.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \return >=0 success
 * \return -1 failure
 * \sa ipfix_start_template(), ipfix_start_data_set()
 */
/*
 */
int ipfix_send(ipfix_exporter *exporter)
{
        int ret = 0;

        if(ipfix_send_templates(exporter) < 0) {
                msg(MSG_ERROR, "sending templates failed");
                ret = -1;
        }
        if(ipfix_send_data(exporter) < 0) {
                msg(MSG_ERROR, "sending data failed");
                ret = -1;
        }

        return ret;
}

/*******************************************************************/
/* Generation of a data set                                        */
/*******************************************************************/

/*!
 * \brief Marks the beginning of a data set
 *
 * The set ID <em>must</em> match a previously defined template ID! It is the
 * user's responsibility to make sure that the corresponding template has been
 * defined using <tt>ipfix_start_template()</tt> and other functions in
 * advance as the library will not perform any checks. There can only be one
 * open data set at any given time. It is not possible to start multiple data
 * sets in parallel.
 *
 * The user must also make sure that there is enough free space left in the
 * IPFIX Message so that the Data Set Header (IPFIX_OVERHEAD_PER_SET == 4
 * bytes) and at least a single Data Record fit inside. Use
 * ipfix_get_remaining_space() to find out how many bytes can still by added
 * without overshooting the maximum message size of an IPFIX Message.
 *
 * The <tt>template_id</tt> parameter has to be in <em>network byte order</em>.
 * This is in contrast to the corresponding parameter of
 * <tt>ipfix_start_template()</tt>
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID of the used template <em>(in network byte order)</em>
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>There is already an open data
 * set</li><li>Insufficient buffer space</li></ul>
 * \sa ipfix_end_data_set(), ipfix_put_data_field()
 */
/*
 */
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t template_id)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	
	// security check
	if(exporter->data_sendbuffer->current != exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "start_data_set called twice.");
                return -1;
        }
    
        // check, if there is enough space in the data set buffer
        // the -1 is because, we expect, we want to add at least one data field.
        if(exporter->data_sendbuffer->current >= IPFIX_MAX_SENDBUFSIZE-1 ) {
                msg(MSG_ERROR, "start_data_set sendbuffer too small to handle more than %i entries",
                    exporter->data_sendbuffer->current
                   );
                return -1;
        }

	// check if we do have space for another set header
        if((current + 1) >= IPFIX_MAX_SETS_PER_PACKET ) {
                msg(MSG_ERROR, "start_data_set set_header_store too small to handle more than %i entries",
                    current + 1
                   );
                return -1;
        }

        // write the set id (=template id) to the data set buffer (length will be added by ipfix_end_data_set):
	manager->set_header_store[current].set_id = template_id;

        // link current set header in entries
        exporter->data_sendbuffer->entries[exporter->data_sendbuffer->current].iov_base = &(manager->set_header_store[current]);
        exporter->data_sendbuffer->entries[exporter->data_sendbuffer->current].iov_len = sizeof(ipfix_set_header);

        exporter->data_sendbuffer->current++;
	exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;

        // initialize the counting of the record's data:
        manager->data_length = 0;

        return 0;
}

/*!
 * \brief Get the number of bytes that can still be added to the current IPFIX
 * message without overshooting the maximum message size.
 *
 * The maximum size of an IPFIX message may be limited by at least the
 * following two factors:
 * <ul>
 * <li>The maximum transmission unit (MTU) if UDP is used as the transport
 * protocol.</li>
 * <li>The maximum DTLS record size if DTLS is used.</li>
 * <li>The maximum message size imposed by IPFIX itself. This is due to the 16
 * bit length field.</li>
 * </ul>
 * If an Exporter has multiple Collectors, then the maximum message size is
 * determined by the collector that allows for the <em>smallest</em> message
 * size. This is due to the fact that ipfixlolib uses a single send buffer for
 * all Collectors. For example, if there is an SCTP Collector and a DTLS over
 * UDP Collector, the maximum message size is usually determined by the DTLS
 * over UDP Collector because its maximum message size is equal to the Maximum
 * Transmission Unit (MTU) imposed by the IP layer minus the overhead caused by
 * UDP and DTLS.
 *
 * It should be noted that there is an overhead of (IPFIX_OVERHEAD_PER_SET ==
 * 4) bytes per Data Set. As a result, you should make sure that there is
 * enough space for the Data Set header (IPFIX_OVERHEAD_PER_SET == 4 bytes) and
 * for at least a single Data Record <em>before</em> calling
 * ipfix_start_data_set().
 *
 * \param exporter pointer to previously initialized exporter struct
 * @return remaining space in bytes. This value will never be negative. If the
 * IPFIX message is already longer than the maximum length, 0 is returned.
 * \sa ipfix_start_data_set(), ipfix_put_data_field()
 */
uint16_t ipfix_get_remaining_space(ipfix_exporter *exporter) {
    int32_t space;
    space = exporter->max_message_size
	// 16 bytes for IPFIX header
	- 16
	- exporter->data_sendbuffer->committed_data_length;
    if(exporter->data_sendbuffer->current != exporter->data_sendbuffer->committed) {
	space -= sizeof(ipfix_set_header);
	space -= exporter->data_sendbuffer->set_manager.data_length;
    }
    if (space < 0) space = 0;
    if (space > IPFIX_MTU_MAX) space = IPFIX_MTU_MAX;
    return space;
}

/*!
 * \brief Add a data field to the send buffer.
 *
 * It is the user's responsibility to make sure that the size of the resulting
 * IPFIX message does not exceed the maximum message size which is derived from
 * the maximum transmission unit (MTU) and other factors. (See
 * ipfix_get_remaining_space() for more details) Use
 * <tt>ipfix_get_remaining_space()</tt> to determine how much space is still
 * left.
 *
 * <em>No sanity checks of any kind are performed.</em> The user has to ensure
 * that<ul>
 * <li>the type of data matches the type required by the Information Element
 * that has been defined for the corresponding field which includes converting
 * integer values into network byte order,</li>
 * <li>the length of data matches the length of the field as defined in the
 * template.</li>
 * </ul>
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param data pointer to data that should be added to the send buffer. Please
 * note, that this <em>pointer</em> will be stored in the send buffer instead
 * of copying the data. As a consequence, the data <em>must</em> stay at the
 * given memory location until the IPFIX message has been sent via
 * ipfix_send().
 * \param length length of data pointed to by <tt>data</tt>
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>no open data set</li><li>send
 * buffer too small</li></ul>
 * \sa ipfix_get_remaining_space()
 */
int ipfix_put_data_field(ipfix_exporter *exporter,void *data, unsigned length) {
    ipfix_sendbuffer *dsb = exporter->data_sendbuffer;
    if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
	msg(MSG_ERROR, "ipfix_put_data_field called but there is no started set.");
	return -1;
    }
    if (dsb->current >= IPFIX_MAX_SENDBUFSIZE) {
	msg(MSG_ERROR, "Sendbuffer too small to handle  %i entries!\n", dsb->current );
	return -1;
    }
    dsb->entries[ dsb->current ].iov_base = data;
    dsb->entries[ dsb->current ].iov_len =  length;
    dsb->current++;
    dsb->set_manager.data_length+= length;
    return 0;
}

/*!
 * \brief Marks the end of a data set
 *
 * The data is not transmitted until <tt>ipfix_send()</tt> is called.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param number_of_records number of data records in this set (used to calculate the sequence number)
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>No open data set</li></ul>
 * \sa ipfix_send()
 */
int ipfix_end_data_set(ipfix_exporter *exporter, uint16_t number_of_records)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	uint16_t record_length;

	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "ipfix_end_data_set called but there is no started set to end.");
                return -1;
        }

	// add number of data records to sequence number increment
	exporter->sn_increment += number_of_records;

        // calculate and store the total length of the set:
        record_length = manager->data_length + sizeof(ipfix_set_header);
	manager->set_header_store[current].length = htons(record_length);

        // update the sendbuffer
        exporter->data_sendbuffer->committed_data_length += record_length;

	// Keep track of number of records in current message
	exporter->data_sendbuffer->record_count += number_of_records;

	// now as we are finished with this set, increase set_counter
	manager->set_counter++;
	
	// update committed 
	exporter->data_sendbuffer->committed = exporter->data_sendbuffer->current;
	exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;
	    
        return 0;
}


/*!
 * \brief Cancel a previously started data set
 *
 * This call ends a previously started data set and discards all data that has
 * been added since the call to <tt>ipfix_start_data_set()</tt>.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>no open data set i.e. no data set to cancel</li></ul>
 * \sa ipfix_start_data_set()
 */
/*
 */
int ipfix_cancel_data_set(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	unsigned int i;

	// security check
	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "cancel_data_set called but there is no set to cancel.");
                return -1;
        }
    
        // clean set id and length:
	manager->set_header_store[current].set_id = 0;
        manager->data_length = 0;

        // clean up entries
	for(i=exporter->data_sendbuffer->committed; i<exporter->data_sendbuffer->current; i++) {
	    exporter->data_sendbuffer->entries[i].iov_base = NULL;
	    exporter->data_sendbuffer->entries[i].iov_len = 0;
	}

        exporter->data_sendbuffer->current = exporter->data_sendbuffer->committed;
        exporter->data_sendbuffer->marker = exporter->data_sendbuffer->committed;

        return 0;
}

/*!
 * \brief Sets the data field marker to the current position in order to allow
 * deletion of newly added fields
 *
 * \param exporter pointer to previously initialized exporter struct
 * \return 0 This value is <em>always</em> returned.
 * \sa ipfix_delete_data_fields_upto_marker()
 */
int ipfix_set_data_field_marker(ipfix_exporter *exporter)
{
    exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;
    return 0;
}

/*!
 * \brief Delete recently added fields up to the marker
 *
 * \param exporter pointer to previously initialized exporter struct
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>no open data set</li></ul>
 * \sa ipfix_set_data_field_marker()
 */
int ipfix_delete_data_fields_upto_marker(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned int i;

	// security check
	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "delete_data_fields_upto_marker called but there is no set.");
                goto out;
        }

	// if marker is before current, clean the entries and set current back 
	if(exporter->data_sendbuffer->marker < exporter->data_sendbuffer->current) {
	    for(i=exporter->data_sendbuffer->marker; i<exporter->data_sendbuffer->current; i++) {
		// decrease data_length
		manager->data_length -= exporter->data_sendbuffer->entries[i].iov_len;
		exporter->data_sendbuffer->entries[i].iov_base = NULL;
		exporter->data_sendbuffer->entries[i].iov_len = 0;
	    }
	    exporter->data_sendbuffer->current = exporter->data_sendbuffer->marker;
	}

        return 0;

out:
        return -1;
}

/*******************************************************************/
/* Generation of a data template set and option template set       */
/*******************************************************************/

/*!
 * \brief Marks the beginning of a Data Template Set and a Template Record
 *
 * ipfixlolib supports only one Data Template Record per Date Template Set. So
 * this function basically starts a Data Template Set and one Data Template
 * Record.
 *
 * Data Templates are a proprietary extension to IPFIX.  See
 * <tt>draft-dressler-ipfix-aggregation-00</tt> for details.  Data Templates
 * are like standard Templates except that they may specify fields with fixed
 * values i.e. fields that share the same value for all Data Records. These
 * values are transmitted once as part of the Data Template Record instead of
 * repeatedly transmitting them with every Data Record.
 *
 * A Data Template consists of (in this order):
 * <ul>
 * <li>Field Specifiers (like in standard Template Records)</li>
 * <li>Field Specifiers of the fixed-value fields</li>
 * <li>Data values of the fixed-value fields</li>
 * </ul>
 *
 * Use the following functions to add the Field Specifiers and Data values to
 * the template:
 * <ul>
 * <li>ipfix_put_template_field()
 * <li>ipfix_put_template_fixedfield()
 * <li>ipfix_put_template_data()
 * </ul>
 *
 * <em>Note:</em> It is not possible to start and define multiple Data
 * Templates in parallel.  ipfix_end_template() has to be called first before a
 * new Data Template can be defined.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID for this Template. Must be > 255.
 * \param preceding
 * \param field_count number of fields that will be added to this Data Template Record.
 *        It is considered an error if more or fewer fields are added.
 * \param fixedfield_count number of fixed-value fields that will be added to this Data Template Record.
 *        It is considered an error if more or fewer fields are added.
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li><tt>template_id</tt> is not
 * great than 255</li><li>maximum number of defined templates has been
 * exceeded</li></ul>
 * \sa ipfix_end_template(), ipfix_put_template_field(), ipfix_put_template_fixedfield(), ipfix_send()
 */

/*
 * Allocate memory for a new template
 * end_data_template will add this template to the exporter
 */
int ipfix_start_datatemplate (ipfix_exporter *exporter,
	uint16_t template_id, uint16_t preceding, uint16_t field_count,
	uint16_t fixedfield_count) {
    /* Is this a regular Template (Set id == 2) or a Data Template (Set id == 4).
       See draft-dressler-ipfix-aggregation-00 for more details on Data Templates.
       Data Templates are (still) a proprietary extension to IPFIX. */
    int datatemplate=(fixedfield_count || preceding) ? 1 : 0;
    if (datatemplate && (exporter->export_protocol != IPFIX_PROTOCOL)) {
	msg(MSG_ERROR, "Only IPFIX supports data templates");
	return -1;
    }
    /* Make sure that template_id is > 255 */
    if (!(template_id > 255)) {
	msg(MSG_ERROR, "Template id has to be > 255. Start of template cancelled.");
	return -1;
    }
    int found_index = ipfix_find_template(exporter, template_id);

    // have we found a template?
    if(found_index >= 0) {
	// we must overwrite the old template.
	// first, clean up the old template:
	switch (exporter->template_arr[found_index].state){
	    case T_SENT:
		// create a withdrawal message first
		ipfix_remove_template(exporter, exporter->template_arr[found_index].template_id);
		/* fall through */
	    case T_WITHDRAWN:
		// send withdrawal messages
		ipfix_send_templates(exporter);
		/* fall through */
	    case T_COMMITED:
	    case T_UNCLEAN:
	    case T_TOBEDELETED:
		// nothing to do, template can be deleted
		ipfix_deinit_template(&(exporter->template_arr[found_index]));
		break;
	    default:
		DPRINTFL(MSG_VDEBUG, "template valid flag is T_UNUSED or invalid\n");	
		break;
	}
    } else {
	/* allocate a new, free slot */
	found_index = ipfix_get_free_template_slot(exporter);
	if (found_index < 0) {
	    msg(MSG_ERROR,"Unable to find free template slot.");
	    return -1;
	}
    }

    char *p_pos;
    char *p_end;

    // allocate memory for the template's field specifiers:
    // 8 bytes for each field specifier, as one field specifier contains:
    // field type, field length (2*2bytes)
    // and an optional Enterprise Number (4 bytes)
    // Also, reserve 4+4 bytes for the Set Header and the Template Record header
    // In case of a Data Template, the Data Template Record header is 8 bytes long
    // (instead of 4 bytes). The total overhead is 4+8=12 in this case.

    exporter->template_arr[found_index].max_fields_length = 8 * (field_count + fixedfield_count) + (datatemplate ? 12 : 8);
    exporter->template_arr[found_index].template_fields = (char*)malloc(exporter->template_arr[found_index].max_fields_length );

    // initialize the rest:
    exporter->template_arr[found_index].state = T_UNCLEAN;
    exporter->template_arr[found_index].template_id = template_id;
    exporter->template_arr[found_index].field_count = field_count;
    exporter->template_arr[found_index].fixedfield_count = fixedfield_count;
    exporter->template_arr[found_index].fields_added = 0;

    // also, write the template header fields into the buffer (except the length field);

    // beginning of the buffer
    p_pos = exporter->template_arr[found_index].template_fields;
    // end of the buffer
    p_end = p_pos + exporter->template_arr[found_index].max_fields_length;

    // ++ Start of Set Header
    // set ID is 2 for a Template Set, 4 for a Data Template with fixed fields:
    // see RFC 5101: 3.3.2 Set Header Format
    /// NFV9 uses 0 for Template Set ID, and does not support Data Templates
    // see RFC 3954: 5.2 Template FlowSet Format
    switch(exporter->export_protocol) {
    case NFV9_PROTOCOL:
	write_unsigned16 (&p_pos, p_end, 0);
	break;
    case IPFIX_PROTOCOL:
	write_unsigned16 (&p_pos, p_end, datatemplate ? 4 : 2);
	break;
    default:
	msg(MSG_ERROR, "Cannot write Template Set ID for unknown protocol");
	break;
    }

    // write 0 to the length field; this will be overwritten by end_template
    write_unsigned16 (&p_pos, p_end, 0);
    // ++ End of Set Header

    // ++ Start of Template Record Header
    // write the template ID: (has to be > 255)
    write_unsigned16 (&p_pos, p_end, template_id); 
    // write the field count:
    write_unsigned16 (&p_pos, p_end, field_count);
    if (datatemplate) {
	// write the fixedfield count:
	write_unsigned16 (&p_pos, p_end, fixedfield_count);
	// write the preceding:
	write_unsigned16 (&p_pos, p_end, preceding);
    }
    // ++ End of Template Record Header

    exporter->template_arr[found_index].fields_length = (datatemplate ? 12 : 8);

    return 0;
}
/*!
 * \brief Marks the beginning of an option template set. (Not yet implemented)
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id the template's ID (in host byte order)
 * \param scope_length the option scope length (in host byte oder)
 * \param option_length the option scope length (in host byte oder)
 * \return -1 Not yet implemented. This value is <em>always</em> returned.
 */
int ipfix_start_optionstemplate(ipfix_exporter *exporter,
	uint16_t template_id, uint16_t scope_length, uint16_t option_length)
{
        msg(MSG_FATAL, "start_optionstemplate() not implemented");
        return -1;
}

/*!
 * \brief Add a field to the previously started template, options template, or data
 * template.
 *
 * This function is called after <tt>ipfix_start_template()</tt>
 * or <tt>ipfix_start_optionstemplate</tt>.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id the ID specified on call to ipfix_start_template()
 * \param type Information Element ID of the field
 * \param length length of the field (in host byte order)
 * \param enterprise_id enterprise number (in host byte order) or 0 for Information Elements registered at IANA
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>template ID
 * unknown</li><li>number of fields exceeds the number that was announced when
 * starting the template.</li></ul>
 * \sa ipfix_start_template(), ipfix_start_optionstemplate()
 */
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id,
	uint16_t type, uint16_t length, uint32_t enterprise_id) {

    int found_index;
    /* set pointers to the buffer */
    char *p_pos;
    char *p_end;
    int enterprise_specific = 0;

    /* test if this is an enterprise-specific field */
    if ((enterprise_id != 0) || ipfix_enterprise_flag_set(type)) {
	    enterprise_specific = 1;
	    /* make sure that enterprise bit is set */
	    type |= IPFIX_ENTERPRISE_BIT;
    }

    found_index = ipfix_find_template(exporter, template_id);

    /* test for a valid slot */
    if(found_index < 0) {
	msg(MSG_VDEBUG, "template ID %u not found", template_id);
	return -1;
    }
    if (exporter->template_arr[found_index].fields_added >=
	    exporter->template_arr[found_index].field_count +
	    exporter->template_arr[found_index].fixedfield_count) {
	msg(MSG_ERROR, "Cannot add more template fields.");
	return -1;
    }

    /* beginning of the buffer */
    p_pos = exporter->template_arr[found_index].template_fields;
    // end of the buffer
    p_end = p_pos + exporter->template_arr[found_index].max_fields_length;

    DPRINTFL(MSG_VDEBUG, "template found at %d", found_index);
    DPRINTFL(MSG_VDEBUG, "A p_pos %p, p_end %p", p_pos, p_end);
    DPRINTFL(MSG_VDEBUG, "max_fields_length %d", exporter->template_arr[found_index].max_fields_length);
    DPRINTFL(MSG_VDEBUG, "fields_length %d", exporter->template_arr[found_index].fields_length);

    // add offset to the buffer's beginning: this is, where we will write to.
    p_pos += exporter->template_arr[found_index].fields_length;

    DPRINTFL(MSG_VDEBUG, "B p_pos %p, p_end %p", p_pos, p_end);

    if(enterprise_specific) {
	DPRINTFL(MSG_VDEBUG, "Notice: using enterprise ID %d with data %d", template_id, enterprise_id);
    }

    // now write the field to the buffer:
    write_extension_and_fieldID(&p_pos, p_end, type);
    // write the field length
    write_unsigned16(&p_pos, p_end, length);

    // add the 4 bytes to the written length:
    exporter->template_arr[found_index].fields_length += 4;

    // write the vendor specific id
    if (enterprise_specific) {
	write_unsigned32(&p_pos, p_end, enterprise_id);
	exporter->template_arr[found_index].fields_length += 4;
    }

    exporter->template_arr[found_index].fields_added ++;
    return 0;
}


/*!
 * \brief Start defining a new Template (Set).
 *
 * ipfixlolib supports only one Template Record per Template Set. So this
 * function basically starts a Template Set and one Template Record.
 *
 * <em>Note:</em> It is not possible to start and define multiple Templates in parallel.
 * ipfix_end_template() has to be called first before a new Template can be
 * defined.
 *
 * Individual fields are added to the Template by calling
 * ipfix_put_data_field() before calling ipfix_end_template() to end the
 * Template.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID for this Template. Must be > 255.
 * \param field_count number of fields that will be added to this Template Record.
 *        It is considered an error if more or fewer fields are added.
 * \return 0 success
 * \return -1 failure. Reasons might be that <tt>template_id</tt> is not great than 255
 *   or that the maximum number of defined templates has been exceeded.
 * \sa ipfix_end_template(), ipfix_put_template_field(), ipfix_send()
**/
int ipfix_start_template (ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count) {
        return ipfix_start_datatemplate(exporter, template_id, 0, field_count, 0);
}


/*!
 * \brief Append fixed-value data type field to the exporter's current Data
 * Template Set, see <tt>ipfix_put_template_field()</tt>.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID of the template
 * \param type Information Element ID of the field
 * \param length length of the field (in host byte order)
 * \param enterprise_id enterprise number (in host byte order) or 0 for Information Elements registered at IANA
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_start_datatemplate()
*/
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id) {
        if (exporter->export_protocol != IPFIX_PROTOCOL) {
	    msg(MSG_ERROR, "Only IPFIX supports data templates");
	    return -1;
	}
        return ipfix_put_template_field(exporter, template_id, type, length, enterprise_id);
}


/*!
 * \brief Append fixed-value data to the exporter's current data template set
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID of the template
 * \param data pointer to the data (data must be in network byte order)
 * \param data_length length of data to be added, in bytes
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_start_datatemplate()
*/
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length) {
        int found_index;
        /* set pointers to the buffer */
        char *p_pos;
        char *p_end;
	
        int i;

	if (exporter->export_protocol != IPFIX_PROTOCOL) {
	    msg(MSG_ERROR, "Only IPFIX supports data templates");
	    return -1;
	}

        found_index = ipfix_find_template(exporter, template_id);

        /* test for a valid slot */
        if (found_index < 0) {
                msg(MSG_VDEBUG, "template ID %u not found", template_id);
                return -1;
        }

        ipfix_lo_template *templ=(&(*exporter).template_arr[found_index]);
	if (templ->fields_added != templ->field_count + templ->fixedfield_count) {
		msg(MSG_ERROR, "All field specifiers must have been added before adding data values to Data Template Record.");
                return -1;
	}
        templ->max_fields_length += data_length;
        templ->template_fields=(char *)realloc(templ->template_fields, templ->max_fields_length);

        /* beginning of the buffer */
        p_pos =  templ->template_fields;
        // end of the buffer
        p_end = p_pos + templ->max_fields_length;
#if 0
        DPRINTFL(MSG_VDEBUG, "template found at %i", found_index);
        DPRINTFL(MSG_VDEBUG, "A p_pos %p, p_end %p", p_pos, p_end);
        DPRINTFL(MSG_VDEBUG, "max_fields_len %u ", (*exporter).template_arr[found_index].max_fields_length);
        DPRINTFL(MSG_VDEBUG, "fieldss_len %u ", (*exporter).template_arr[found_index].fields_length);
#endif

        // add offset to the buffer's beginning: this is, where we will write to.
        p_pos += templ->fields_length;
#if 0
        DPRINTFL(MSG_VDEBUG, "B p_pos %p, p_end %p", p_pos, p_end);
#endif

        for(i = 0; i < data_length; i++) {
            write_octet(&p_pos, p_end, *(((uint8_t*)data)+i) );
        }

        // add to the written length:
        templ->fields_length += data_length;

        return 0;
}

/*!
 * \brief End a previously started and defined template, options template, or data template.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param template_id ID of the template
 * \return 0 success
 * \return -1 failure. Reasons include:<ul><li>Template ID unknown</li><li>number of Template fields added does not match number announced when calling <tt>ipfix_start_template()</tt></li></ul>
 * \sa ipfix_start_template(), ipfix_put_template_field(), ipfix_send()
 */
int ipfix_end_template(ipfix_exporter *exporter, uint16_t template_id)
{
    int found_index;
    char *p_pos;
    char *p_end;

    found_index = ipfix_find_template(exporter, template_id);

    // test for a valid slot:
    if (found_index < 0) {
	msg(MSG_ERROR, "template %u not found", template_id);
	return -1;
    }
    ipfix_lo_template *templ=(&exporter->template_arr[found_index]);
    if (templ->fields_added != templ->field_count + templ->fixedfield_count) {
	msg(MSG_ERROR, "Number of added template fields does not match number passed to ipfix_start_template");
	ipfix_deinit_template(templ);
	return -1;
    }
    // reallocate the memory , i.e. free superfluous memory, as we allocated enough memory to hold
    // all possible vendor specific IDs.
    templ->template_fields=(char *)realloc(templ->template_fields, templ->fields_length);
    templ->max_fields_length=templ->fields_length;

    /*
       write the real length field:
       set pointers:
       beginning of the buffer
       */
    p_pos = exporter->template_arr[found_index].template_fields;
    // end of the buffer
    p_end = p_pos + exporter->template_arr[found_index].max_fields_length;
    // add offset of 2 bytes to the buffer's beginning: this is, where we will write to.
    p_pos += 2;

    // write the length field
    write_unsigned16 (&p_pos, p_end, templ->fields_length);
    // call the template valid
    templ->state = T_COMMITED;
    // force resending templates to UDP collectors by resetting transmission time
    exporter->last_template_transmission_time = 0;

    return 0;
}

/*
 * removes a template set from the exporter
 * Checks, if the template is in use, before trying to free it.
 * Parameters:
 *  template* : pointer to the template to be freed
 * Returns: 0  on success, -1 on failure
 * This is an internal function.
 */
static int ipfix_deinit_template(ipfix_lo_template *templ) {
    // note: ipfix_deinit_template_array tries to free all possible templates, many of them
    // won't be initialized. So you'll get a lot of warning messages, which are just fine...

    if(templ == NULL)
	return -1;

    // first test, if we can free this template
    if (templ->state == T_UNUSED)
	return -1;
    DPRINTFL(MSG_VDEBUG, "deleting Template ID: %d validity: %d", templ->template_id, templ->state);
    templ->state = T_UNUSED;
    free(templ->template_fields);
    templ->template_fields = 0;

    return 0;
}
/*!
 * \brief Set time after that Templates are going to be resent
 *
 * If UDP is used as the transport protocol, this value determines the interval
 * in which active Templates are retransmitted.
 * \param exporter pointer to previously initialized exporter struct
 * \param timer timeout value in seconds
 * \return 0 This value is always returned.
 */
int ipfix_set_template_transmission_timer(ipfix_exporter *exporter, uint32_t timer){
	
    exporter->template_transmission_timer = timer;
    return 0;
}

/*!
 * \brief Set SCTP packet lifetime
 *
 * Set packet lifetime for SCTP data packets (lifetime > 0 : unreliable packets) 	 
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param lifetime packet lifetime in milliseconds
 * \return 0 This value is always returned.
 */
int ipfix_set_sctp_lifetime(ipfix_exporter *exporter, uint32_t lifetime) {
    exporter->sctp_lifetime = lifetime;
    return 0;
}
/*!
 * \brief Set up SCTP reconnect timer
 *
 * Time after which a reconnection attempt is made in case the connection to
 * the Collector is lost.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param timer timeout value in seconds
 * \return 0 This value is always returned.
 */
int ipfix_set_sctp_reconnect_timer(ipfix_exporter *exporter, uint32_t timer) {
    exporter->sctp_reconnect_timer = timer;
    return 0;
}

/*!
 * \brief Setup X.509 certificate used for authentication
 *
 * Set the the names of the files in which the X.509 certificate and the
 * matching private key can be found. If private_key_file is NULL the
 * certificate chain file will be searched for the private key.  See OpenSSL
 * man pages for more details
 *
 * The certificate can only be set once per exporter. Calling this function
 * twice for the same exporter is an error. This function must not be called
 * after the first DTLS connection has been set up.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param certificate_chain_file name of file in which the certificate chain is
 * stored in PEM format
 * \param private_key_file name of file in which the private key is stored in
 *   PEM format. If NULL, the private key is read from certificate_chain_file.
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_set_ca_locations()
 */
int ipfix_set_dtls_certificate(ipfix_exporter *exporter,
	const char *certificate_chain_file, const char *private_key_file) {
#ifdef SUPPORT_DTLS
    if (exporter->ssl_ctx) {
	msg(MSG_ERROR, "Too late to set certificate. SSL context already created.");
	return -1;
    }
    if (exporter->certificate_chain_file) {
	msg(MSG_ERROR, "Certificate can not be reset.");
	return -1;
    }
    if ( ! certificate_chain_file) {
	msg(MSG_ERROR, "ipfix_set_dtls_certificate called with bad parameters.");
	return -1;
    }
    exporter->certificate_chain_file = strdup(certificate_chain_file);
    if (private_key_file) {
	exporter->private_key_file = strdup(private_key_file);
    }
    return 0;
#else /* SUPPORT_DTLS */
    msg(MSG_FATAL, "Library compiled without DTLS support.");
    return -1;
#endif /* SUPPORT_DTLS */
}

/*!
 * \brief Set the locations of the CA certificates.
 *
 * See <tt>SSL_CTX_load_verify_locations(3)</tt> for more details. These
 * locations can only be set once per exporter. Calling this function twice for
 * the same exporter is an error.
 *
 * \param exporter pointer to previously initialized exporter struct
 * \param ca_file All CA certificates found in this <em>file</em> are trusted
 * for verification of the peer's certificate. This file has to be in PEM format.
 * \param ca_path All CA certificates found in this <em>directory</em> are
 * trusted for verification of the peer's certificate. See
 * <tt>SSL_CTX_load_verify_locations(3)</tt> for details about how the files
 * have to be named. All files need to be in PEM format.
 * \return 0 success
 * \return -1 failure
 * \sa ipfix_set_dtls_certificate()
 */
int ipfix_set_ca_locations(ipfix_exporter *exporter, const char *ca_file, const char *ca_path) {
#ifdef SUPPORT_DTLS
    if (exporter->ssl_ctx) {
	msg(MSG_ERROR, "Too late to set CA locations. SSL context already created.");
	return -1;
    }
    if (exporter->ca_file || exporter->ca_path) {
	msg(MSG_ERROR, "CA locations can not be reset.");
	return -1;
    }
    if (ca_file) exporter->ca_file = strdup(ca_file);
    if (ca_path) exporter->ca_path = strdup(ca_path);
    return 0;
#else /* SUPPORT_DTLS */
    msg(MSG_FATAL, "Library compiled without DTLS support.");
    return -1;
#endif
}

/* check if the enterprise bit in an ID is set */
static int ipfix_enterprise_flag_set(uint16_t id)
{
        return bit_set(id, IPFIX_ENTERPRISE_BIT);
}

#ifdef SUPPORT_DTLS_OVER_SCTP
static void
   handle_sctp_event(BIO *bio, void *context, void *buf)
   {
#if 0
       ipfix_dtls_connection *con = (ipfix_dtls_connection *) context;
#endif
       struct sctp_assoc_change *sac;
       struct sctp_send_failed *ssf;
       struct sctp_paddr_change *spc;
       struct sctp_remote_error *sre;
#ifdef SUPPORT_DTLS_OVER_SCTP
       struct sctp_sender_dry_event *ssde;
#endif
       union sctp_notification *snp;
       char addrbuf[INET6_ADDRSTRLEN];
       const char *ap;
       struct sockaddr_in *sin;
       struct sockaddr_in6 *sin6;

       snp = buf;

       switch (snp->sn_header.sn_type) {
       case SCTP_ASSOC_CHANGE:
             sac = &snp->sn_assoc_change;
             msg(MSG_DEBUG,"SCTP Event: assoc_change: state=%hu, error=%hu, instr=%hu "
                 "outstr=%hu\n", sac->sac_state, sac->sac_error,
                 sac->sac_inbound_streams, sac->sac_outbound_streams);
             break;
       case SCTP_SEND_FAILED:
             ssf = &snp->sn_send_failed;
             msg(MSG_DEBUG,"SCTP Event: sendfailed: len=%hu err=%d\n", ssf->ssf_length,
                 ssf->ssf_error);
             break;

       case SCTP_PEER_ADDR_CHANGE:
             spc = &snp->sn_paddr_change;
             if (spc->spc_aaddr.ss_family == AF_INET) {
               sin = (struct sockaddr_in *)&spc->spc_aaddr;
               ap = inet_ntop(AF_INET, &sin->sin_addr,
                              addrbuf, INET6_ADDRSTRLEN);
             } else {
               sin6 = (struct sockaddr_in6 *)&spc->spc_aaddr;
               ap = inet_ntop(AF_INET6, &sin6->sin6_addr,
                              addrbuf, INET6_ADDRSTRLEN);
             }
             msg(MSG_DEBUG,"SCTP Event: intf_change: %s state=%d, error=%d\n", ap,
                    spc->spc_state, spc->spc_error);
             break;
       case SCTP_REMOTE_ERROR:
             sre = &snp->sn_remote_error;
             msg(MSG_DEBUG,"SCTP Event: remote_error: err=%hu len=%hu\n",
                 ntohs(sre->sre_error), ntohs(sre->sre_length));
             break;
       case SCTP_SHUTDOWN_EVENT:
             msg(MSG_DEBUG,"SCTP Event: shutdown event\n");
             break;
#ifdef SUPPORT_DTLS_OVER_SCTP
       case SCTP_SENDER_DRY_EVENT:
	     ssde = &snp->sn_sender_dry_event;
             msg(MSG_DEBUG,"SCTP Event: sender dry event\n");
             break;
       case SCTP_AUTHENTICATION_EVENT:
             msg(MSG_DEBUG,"SCTP Event: authentication event\n");
             break;
#endif
       default:
             msg(MSG_DEBUG,"SCTP Event: unknown type: %hu\n", snp->sn_header.sn_type);
             break;
       };
   }

#endif


#ifdef __cplusplus
}
#endif
