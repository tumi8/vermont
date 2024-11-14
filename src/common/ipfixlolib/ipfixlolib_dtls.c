#include <fcntl.h>

#include "common/defs.h" /* __FALLTHROUGH__ */
#include "common/msg.h"
#include "common/openssl/OpenSSL.h"
#include "ipfixlolib_private.h"
#include "ipfixlolib_dtls.h"
#include "ipfixlolib_dtls_private.h"

#ifdef SUPPORT_SCTP
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
       struct sctp_sender_dry_event *ssde;
       union sctp_notification *snp;
       char addrbuf[INET6_ADDRSTRLEN];
       const char *ap;
       struct sockaddr_in *sin;
       struct sockaddr_in6 *sin6;

       snp = buf;

       switch (snp->sn_header.sn_type) {
       case SCTP_ASSOC_CHANGE:
             sac = &snp->sn_assoc_change;
             msg(LOG_INFO,"SCTP Event: assoc_change: state=%hu, error=%hu, instr=%hu "
                 "outstr=%hu\n", sac->sac_state, sac->sac_error,
                 sac->sac_inbound_streams, sac->sac_outbound_streams);
             break;
       case SCTP_SEND_FAILED:
             ssf = &snp->sn_send_failed;
             msg(LOG_INFO,"SCTP Event: sendfailed: len=%hu err=%d\n", ssf->ssf_length,
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
             msg(LOG_INFO,"SCTP Event: intf_change: %s state=%d, error=%d\n", ap,
                    spc->spc_state, spc->spc_error);
             break;
       case SCTP_REMOTE_ERROR:
             sre = &snp->sn_remote_error;
             msg(LOG_INFO,"SCTP Event: remote_error: err=%hu len=%hu\n",
                 ntohs(sre->sre_error), ntohs(sre->sre_length));
             break;
       case SCTP_SHUTDOWN_EVENT:
             msg(LOG_INFO,"SCTP Event: shutdown event\n");
             break;
       case SCTP_SENDER_DRY_EVENT:
	     ssde = &snp->sn_sender_dry_event;
             msg(LOG_INFO,"SCTP Event: sender dry event\n");
             break;
       case SCTP_AUTHENTICATION_EVENT:
             msg(LOG_INFO,"SCTP Event: authentication event\n");
             break;
       default:
             msg(LOG_INFO,"SCTP Event: unknown type: %hu\n", snp->sn_header.sn_type);
             break;
       };
   }
#endif /*SUPPORT_DTLS_OVER_SCTP */
#endif /* SUPPORT_SCTP */

/* A separate SSL_CTX object is created for every ipfix_exporter.
 * Returns 0 on success, -1 on error
 * */
static int ensure_exporter_set_up_for_dtls(ipfix_exporter_certificate *c) {
    ensure_openssl_init();

    if (c->ssl_ctx) return 0;

    /* This SSL_CTX object will be freed in deinit_openssl_ctx() */
    if ( ! (c->ssl_ctx=SSL_CTX_new(DTLS_client_method())) ) {
	msg(LOG_CRIT, "Failed to create SSL context");
	msg_openssl_errors();
	return -1;
    }
    SSL_CTX_set_read_ahead(c->ssl_ctx,1);

    if ( (c->ca_file || c->ca_path) &&
	! SSL_CTX_load_verify_locations(c->ssl_ctx,c->ca_file,c->ca_path) ) {
	msg(LOG_CRIT,"SSL_CTX_load_verify_locations() failed.");
	msg_openssl_errors();
	return -1;
    }
    /* Load our own certificate */
    if (c->certificate_chain_file) {
	if (!SSL_CTX_use_certificate_chain_file(c->ssl_ctx, c->certificate_chain_file)) {
	    msg(LOG_CRIT,"Unable to load certificate chain file %s",c->certificate_chain_file);
	    msg_openssl_errors();
	    return -1;
	}
	if (!SSL_CTX_use_PrivateKey_file(c->ssl_ctx, c->private_key_file, SSL_FILETYPE_PEM)) {
	    msg(LOG_CRIT,"Unable to load private key file %s",c->private_key_file);
	    msg_openssl_errors();
	    return -1;
	}
	if (!SSL_CTX_check_private_key(c->ssl_ctx)) {
	    msg(LOG_CRIT,"Private key and certificate do not match");
	    msg_openssl_errors();
	    return -1;
	}
	DPRINTF_INFO("We successfully loaded our certificate.");
    } else {
	DPRINTF_INFO("We do NOT have a certificate.");
    }
    /* We leave the certificate_authorities list of the Certificate Request
     * empty. See RFC 4346 7.4.4. Certificate request. */
    return 0;
}

void deinit_openssl_ctx(ipfix_exporter_certificate *c) {
    if (c->ssl_ctx) { SSL_CTX_free(c->ssl_ctx); }
    c->ssl_ctx = NULL;
}

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
                msg(LOG_CRIT, "error opening socket, %s", strerror(errno));
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
                msg(LOG_CRIT, "could not set socket non-blocking");
		close(s);
                return -1;
	}
#ifdef SUPPORT_DTLS_OVER_SCTP
	/* enable the reception of SCTP_SNDRCV information on a per
	 * message basis. */
	memset(&event, 0, sizeof(event));
	event.sctp_data_io_event = 1;
	if (setsockopt(s, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event)) != 0) {
		msg(LOG_ERR, "SCTP: setsockopt() failed to enable sctp_data_io_event, %s", strerror(errno));
		close(s);
                return -1;
	}
#endif

	return s;
}

/* returns 0 on success and -1 on failure */
int setup_dtls_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col, ipfix_dtls_connection *con) {
    BIO *bio;
/* Resources allocated in this function. Those need to be freed in case of failure:
 * - socket
 * - SSL object
 * - BIO
 */

#ifdef DEBUG
    if (con->socket!=-1) {
	msg(LOG_CRIT,"socket != -1");
	close(con->socket);
	con->socket = -1;
    }
#endif

    /* Create socket
     create_dtls_socket() also activates PMTU Discovery. */
    if ((con->socket = create_dtls_socket(col)) < 0) {
	return -1;
    }
    DPRINTF_INFO("Created socket %d",con->socket);

    /* ensure an SSL_CTX object is set up */
    if (ensure_exporter_set_up_for_dtls(&exporter->certificate)) {
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* create SSL object */
    if ( ! (con->ssl = SSL_new(exporter->certificate.ssl_ctx))) {
	msg(LOG_CRIT, "Failed to create SSL object.");
	msg_openssl_errors();
	close(con->socket);con->socket = -1;
	return -1;
    }
    /* Set verification parameters and cipherlist */
    if (!col->dtls_connection.peer_fqdn) {
	SSL_set_cipher_list(con->ssl,"ALL"); // This includes anonymous ciphers
	DPRINTF_INFO("We are NOT going to verify the certificates of the collectors b/c "
		"the peerFqdn option is NOT set.");
    } else {
	if ( ! ((exporter->certificate.ca_file || exporter->certificate.ca_path) &&
		    exporter->certificate.certificate_chain_file) ) {
	    msg(LOG_ERR,"Cannot verify certificates of collectors because prerequisites not met. "
		    "Prerequisites are: 1. CApath or CAfile or both set, "
		    "2. We have a certificate including the private key");
	    SSL_free(con->ssl);con->ssl = NULL;
	    close(con->socket);con->socket = -1;
	    return -1;
	} else {
	    SSL_set_cipher_list(con->ssl,"DEFAULT");
	    SSL_set_verify(con->ssl,SSL_VERIFY_PEER |
		    SSL_VERIFY_FAIL_IF_NO_PEER_CERT,0);
	    DPRINTF_INFO("We are going to request certificates from the collectors "
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
	msg(LOG_CRIT,"Failed to create datagram BIO.");
	msg_openssl_errors();
	SSL_free(con->ssl);con->ssl = NULL;
	close(con->socket);con->socket = -1;
	return -1;
    }
#ifdef SUPPORT_DTLS_OVER_SCTP
    if (col->protocol != DTLS_OVER_SCTP)
#endif
    (void)BIO_ctrl(bio,BIO_CTRL_DGRAM_MTU_DISCOVER,0,0);
    (void)BIO_ctrl_set_connected(bio,&col->addr); /* TODO: Explain, why are we doing this? */
    SSL_set_bio(con->ssl,bio,bio);
    // connect (non-blocking, i.e. handshake is initiated, not terminated)
    if((connect(con->socket, (struct sockaddr*)&col->addr, sizeof(col->addr) ) == -1) && (errno != EINPROGRESS)) {
	    msg(LOG_CRIT, "connect failed, %s", strerror(errno));
	    SSL_free(con->ssl);con->ssl = NULL;
	    close(con->socket);con->socket = -1;
	    return -1;
    }
    DPRINTF_INFO("Set up SSL object.");

    con->last_reconnect_attempt_time = time(NULL);

    return 0;
}

static int dtls_verify_peer_cb(void *context, const char* dnsname) {
    const ipfix_receiving_collector *col =
	(const ipfix_receiving_collector *) context;
    return strcasecmp(col->dtls_connection.peer_fqdn,dnsname) ? 0 : 1;
}

static void dtls_fail_connection(ipfix_dtls_connection *con) {
    DPRINTF_INFO("Failing DTLS connection.");
    dtls_shutdown_and_cleanup(con);
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
	msg_openssl_return_code(LOG_INFO,"SSL_connect()",ret,error);
	msg(LOG_NOTICE, "Successfully (re)connected to %s-over-DTLS collector.", col->protocol == DTLS_OVER_SCTP ? "SCTP" : "UDP");
	msg(LOG_NOTICE,"TLS Cipher: %s",SSL_get_cipher_name(con->ssl));
	DPRINTF_INFO("DTLS handshake succeeded. We are now connected.");
	if (col->dtls_connection.peer_fqdn) { /* We need to verify the identity of our peer */
	    if (verify_ssl_peer(con->ssl,&dtls_verify_peer_cb,col)) {
		DPRINTF_INFO("Peer authentication successful.");
	    } else {
		msg(LOG_ERR,"Peer authentication failed. Shutting down connection.");
		dtls_fail_connection(con);
		return -1;
	    }
	}
	return 1;
    } else if (error == SSL_ERROR_WANT_READ) {
	msg_openssl_return_code(LOG_INFO,"SSL_connect()",ret,error);
	return 0;
    } else {
	msg_openssl_return_code(LOG_ERR,"SSL_connect()",ret,error);
	dtls_fail_connection(con);
	return -1;
    }
}

static int dtls_get_replacement_connection_ready(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col) {
    int ret;
    if (!col->dtls_connection.dtls_replacement.ssl) {
	/* No SSL object has been created yet. Let's open a socket and
	 * setup a new SSL object. */
	DPRINTF_INFO("Setting up replacement connection.");
	if (setup_dtls_connection(exporter,col,&col->dtls_connection.dtls_replacement)) {
	    return -1;
	}
    }
    ret = dtls_connect(col,&col->dtls_connection.dtls_replacement);
    if (ret == 1) {
	DPRINTF_INFO("Replacement connection setup successful.");
	return 1; /* SUCCESS */
    }
    if (ret == 0) {
	if (col->dtls_connection.dtls_connect_timeout && 
		(time(NULL) - col->dtls_connection.dtls_replacement.last_reconnect_attempt_time >
		 col->dtls_connection.dtls_connect_timeout)) {
	    msg(LOG_ERR,"DTLS replacement connection setup taking too long.");
	    dtls_fail_connection(&col->dtls_connection.dtls_replacement);
	} else {
	    DPRINTF_INFO("Replacement connection setup still ongoing.");
	    return 0;
	}
    }
    return -1;
}

static void dtls_swap_connections(ipfix_dtls_connection *a, ipfix_dtls_connection *b) {
    ipfix_dtls_connection tmp;
    memcpy(&tmp,a,sizeof(tmp));
    memcpy(a,b,sizeof(*b));
    memcpy(b,&tmp,sizeof(*b));
}

void dtls_shutdown_and_cleanup(ipfix_dtls_connection *con) {
    int ret,error;
    if (!con->ssl) return;
    DPRINTF_INFO("Shutting down SSL connection.");
    ret = SSL_shutdown(con->ssl);
    error = SSL_get_error(con->ssl,ret);
#ifdef DEBUG
    msg_openssl_return_code(LOG_INFO,"SSL_shutdown()",ret,error);
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
	DPRINTF_INFO("select returned: %d",ret);
	DPRINTF_INFO("Calling SSL_shutdown()");
	ret = SSL_shutdown(con->ssl);
	error = SSL_get_error(con->ssl,ret);
	msg_openssl_return_code(LOG_INFO,"SSL_shutdown()",ret,error);
	if (i++ == 3) {
	    msg(LOG_ERR,"Too many calls to select(). Breaking out.");
	    break;
	}
    }
    /* Note: SSL_free() also frees associated sending and receiving BIOs */
    SSL_free(con->ssl);
    con->ssl = NULL;
    con->last_reconnect_attempt_time = 0;
    /* Close socket */
    if ( con->socket != -1) {
	DPRINTF_INFO("Closing socket");
	ret = close(con->socket);
	DPRINTF_INFO("close returned %d",ret);
	con->socket = -1;
    }
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
int dtls_manage_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col) {
    int ret, rc = 0;

    if (col->state == C_CONNECTED) {
	if( col->protocol == DTLS_OVER_SCTP ||
		col->dtls_connection.dtls_max_connection_lifetime == 0 ||
		time(NULL) - col->dtls_connection.connect_time <
	    col->dtls_connection.dtls_max_connection_lifetime)
	return 0;
	/* Alright, the connection is already very old and needs to be
	 * replaced. Let's get the replacement / backup connection ready. */
	ret = dtls_get_replacement_connection_ready(exporter, col);
	rc = 1;
	if (ret == 1) { /* Connection setup completed */
	    rc = 0;
	    DPRINTF_INFO("Swapping connections.");
	    dtls_swap_connections(&col->dtls_connection.dtls_main, &col->dtls_connection.dtls_replacement);
	    DPRINTF_INFO("Shutting down old DTLS connection.");
	    dtls_shutdown_and_cleanup(&col->dtls_connection.dtls_replacement);
	    col->dtls_connection.connect_time = time(NULL);
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
	ret = dtls_connect(col,&col->dtls_connection.dtls_main);
	if (ret == 1) {
	    /* SUCCESS */
	    col->state = C_CONNECTED;
	    col->dtls_connection.connect_time = time(NULL);
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
		col->dtls_connection.dtls_connect_timeout)
	    return 1;
	rc = 1;
	if (setup_dtls_connection(exporter,col,&col->dtls_connection.dtls_main)) {
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
	    msg(LOG_CRIT, "sendbuffer for dtls_send too small.");
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
    msg_openssl_return_code(LOG_INFO,buf,len,error);
#endif
    switch (error) {
	case SSL_ERROR_NONE:
	    if (len!=sendbufcur - sendbuf) {
		msg(LOG_CRIT, "len!=sendbuflen when calling SSL_write()");
		return -1;
	    }
	    return sendbufcur - sendbuf; /* SUCCESS */
	case SSL_ERROR_WANT_READ:
	    return 0;
	case SSL_ERROR_SYSCALL:
	    if (errno == EMSGSIZE) {
		return -3;
	    }
	    __FALLTHROUGH__;
	default:
	    msg_openssl_return_code(LOG_ERR,"SSL_write()",len,error);
	    dtls_fail_connection(con);
	    return -2;
    }
}

/* Return values:
 * -1 error
 *  0 could not write because OpenSSL returned SSL_ERROR_WANT_READ
 *  n>0 number of bytes written
 */
int dtls_send(ipfix_exporter *exporter, ipfix_receiving_collector *col,
	      const struct iovec *iov, int iovcnt) {

    int len;

    /* DTLS negotiation has to be finished before we can send data.
     * Drop out of this function if we are not yet connected. */
    if (col->state != C_CONNECTED) {
	return -1;
    }

    len = dtls_send_helper(&col->dtls_connection.dtls_main, iov, iovcnt);
    if (len == -2) {
	col->state = C_DISCONNECTED;
	return -1;
    } else if (len == -3) {
	update_collector_mtu(exporter,col);
	return -1;
    }
    return len;
}

int dtls_send_templates(ipfix_exporter *exporter, ipfix_receiving_collector *col) {
    if (exporter->template_sendbuffer->committed_data_length == 0)
	return 0;

    ipfix_update_header(exporter, col,
	exporter->template_sendbuffer);
    col->messages_sent++;
    DPRINTF_INFO("Sending templates over DTLS.");
    return dtls_send(exporter,col,
	exporter->template_sendbuffer->entries,
	exporter->template_sendbuffer->current);
}

int add_collector_dtls(
	ipfix_exporter *exporter,
	ipfix_receiving_collector *col,
	void *aux_config) {

    col->dtls_connection.dtls_replacement.socket = -1;
    col->dtls_connection.dtls_replacement.ssl = NULL;

    col->dtls_connection.dtls_max_connection_lifetime = 0;
    col->dtls_connection.dtls_connect_timeout = 30;

    // we need aux_config for setting up a DTLS collector
    if (!aux_config) {
        return -1;
    }

    ipfix_aux_config_dtls *aux_config_dtls;
    if (col->protocol == DTLS_OVER_SCTP) {
	aux_config_dtls = &((ipfix_aux_config_dtls_over_sctp*)aux_config)->dtls;
    } else if (col->protocol == DTLS_OVER_UDP) {
	aux_config_dtls = &((ipfix_aux_config_dtls_over_udp*)aux_config)->dtls;
	col->dtls_connection.dtls_max_connection_lifetime =
	    ((ipfix_aux_config_dtls_over_udp*)aux_config)->max_connection_lifetime;
	ipfix_aux_config_udp *aux_config_udp;
	aux_config_udp = &((ipfix_aux_config_dtls_over_udp*)aux_config)->udp;
	/* Sets col->mtu_mode and col->mtu */
	set_mtu_config(col,aux_config_udp);
    } else {
	return -1;
    }
    if (aux_config_dtls->peer_fqdn)
	col->dtls_connection.peer_fqdn = strdup(aux_config_dtls->peer_fqdn);

    if (setup_dtls_connection(exporter,col,&col->dtls_connection.dtls_main)) {
	/* failure */
	free( (void *) col->dtls_connection.peer_fqdn);
	col->dtls_connection.peer_fqdn = NULL;
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

void ipfix_init_dtls_certificate(ipfix_exporter_certificate *certificate) {
    certificate->ssl_ctx = NULL;
    certificate->certificate_chain_file = NULL;
    certificate->private_key_file = NULL;
    certificate->ca_file = NULL;
    certificate->ca_path = NULL;
}


void ipfix_clear_dtls_certificate(ipfix_exporter_certificate *certificate) {
    deinit_openssl_ctx(certificate);
    free( (void *) certificate->certificate_chain_file);
    free( (void *) certificate->private_key_file);
    free( (void *) certificate->ca_file);
    free( (void *) certificate->ca_path);
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
int ipfix_set_dtls_certificate(ipfix_exporter_certificate *certificate,
	const char *certificate_chain_file, const char *private_key_file) {
    if (certificate->ssl_ctx) {
	msg(LOG_ERR, "Too late to set certificate. SSL context already created.");
	return -1;
    }
    if (certificate->certificate_chain_file) {
	msg(LOG_ERR, "Certificate can not be reset.");
	return -1;
    }
    if ( ! certificate_chain_file) {
	msg(LOG_ERR, "ipfix_set_dtls_certificate called with bad parameters.");
	return -1;
    }
    certificate->certificate_chain_file = strdup(certificate_chain_file);
    if (private_key_file) {
	certificate->private_key_file = strdup(private_key_file);
    }
    return 0;
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
int ipfix_set_ca_locations(ipfix_exporter_certificate *certificate, const char *ca_file, const char *ca_path) {
    if (certificate->ssl_ctx) {
	msg(LOG_ERR, "Too late to set CA locations. SSL context already created.");
	return -1;
    }
    if (certificate->ca_file || certificate->ca_path) {
	msg(LOG_ERR, "CA locations can not be reset.");
	return -1;
    }
    if (ca_file) certificate->ca_file = strdup(ca_file);
    if (ca_path) certificate->ca_path = strdup(ca_path);
    return 0;
}

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
int ipfix_dtls_advance_connections(ipfix_exporter *exporter) {
    int ret = 0;
    for (int i = 0; i < exporter->collector_max_num; i++) {
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
    return ret;
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
