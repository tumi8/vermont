/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
/*
 * IPFIX Concentrator Module Library - SCTP Receiver
 * Copyright (C) 2007 Alex Melnik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef _IPFIX_RECEIVER_DTLSSCTPIPV4_H_
#define _IPFIX_RECEIVER_DTLSSCTPIPV4_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>
#include <map>
#include <set>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

#ifdef SUPPORT_DTLS

#include "common/openssl/SSLCTXWrapper.hpp"
#include "common/openssl/OpenSSL.h"

#include <openssl/ssl.h>

#if defined(SUPPORT_DTLS_OVER_SCTP) && !defined(OPENSSL_SCTP)
# error OpenSSL built without SCTP support. Rebuild OpenSSL with SCTP support or turn off SUPPORT_DTLS_OVER_SCTP
#endif

#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif
#endif

// Quote from man page: "maximum length to which the queue of pending connections
// for sockfd may grow. If a connection request arrives when the queue is
// full, the client may receive an error with an indication of ECONNREFUSED
// or, if the underlying protocol supports retransmission, the request may
// be ignored so that a later reattempt at connection succeeds."
#define SCTP_MAX_BACKLOG 5


class IpfixReceiverDtlsSctpIpV4 : public IpfixReceiver, Sensor {
#ifdef SUPPORT_DTLS_OVER_SCTP
    public:
	IpfixReceiverDtlsSctpIpV4(int port, const std::string ipAddr = "",
	    const std::string &certificateChainFile = "", const std::string &privateKeyFile = "",
	    const std::string &caFile = "", const std::string &caPath = "",
	    const std::set<string> &peerFqdns = std::set<string>(), const uint32_t buffer);
	virtual ~IpfixReceiverDtlsSctpIpV4();

	virtual void run();
	virtual std::string getStatisticsXML(double interval);
		
    private:
	/* It is utterly important that ssl_ctx is defined
	 * before connections_map!.
	 * The reason is that the SSL objects depend on the SSL_CTX object.
	 * There's a dependency between SSL_CTX and SSL objects.
	 * The SSL_CTX object is created first. All the SSL objects
	 * that derive from that SSL_CTX are created by calling
	 * ssl = SSL_new(ctx). It's important to call SSL_free(ssl) before
	 * calling SSL_CTX_free(ctx).
	 * The descructor of DtlsConnection calls SSL_free(ssl) whereas
	 * the descructor of SSL_CTX_wrapper calls SSL_CTX_free(ctx).
	 * Also note, that the descructors are called in the opposite order
	 * than the correspondig members are defined here.*/
	SSL_CTX_wrapper ssl_ctx;
	int listen_socket;
	fd_set readfds, writefds;
	int maxfd;
	const std::set<string> peerFqdns;
	friend int verify_peer_cb_sctp(void *context, const char *dnsname);
	uint32_t statReceivedPackets;  /**< number of received packets */ 

	class DtlsConnection {
	    public:
		DtlsConnection(IpfixReceiverDtlsSctpIpV4 &parent,struct sockaddr_in *clientAddress, int socket);
		~DtlsConnection();
		int fdready();

	    private:
		struct sockaddr_in clientAddress;
		int socket;
		IpfixReceiverDtlsSctpIpV4 &parent;
		SSL* ssl;
		time_t last_used;
		int accept();
		void shutdown();
		int verify_peer();
		struct verify_peer_cb_data vpcd;
		boost::shared_ptr<IpfixRecord::SourceID> sourceID;
	};
	typedef boost::shared_ptr<DtlsConnection> DtlsConnectionPtr;

	typedef std::map<int,DtlsConnectionPtr> connections_map;
	connections_map connections;
	void remove_connection(int socket);
	void update_maxfd(void);


#ifdef DEBUG
	void dumpConnections(void);
#else
	inline void dumpConnections(void) {}
#endif

#else /* SUPPORT_DTLS_OVER_SCTP */

    /* This is the code that gets compiled if SUPPORT_DTLS_OVER_SCTP is not set */

    public:
	IpfixReceiverDtlsSctpIpV4(int port, const std::string ipAddr = "",
	    const std::string &certificateChainFile = "", const std::string &privateKeyFile = "",
	    const std::string &caFile = "", const std::string &caPath = "",
	    const std::set<string> &peerFqdns = std::set<string>(), const uint32_t buffer = 0) {
	    THROWEXCEPTION("DTLS over SCTP not supported!");
	}

	virtual ~IpfixReceiverDtlsSctpIpV4() {}

	virtual void run() {}
	virtual std::string getStatisticsXML(double interval) { return ""; }

#endif /*SUPPORT_DTLS_OVER_SCTP*/
};

#endif

