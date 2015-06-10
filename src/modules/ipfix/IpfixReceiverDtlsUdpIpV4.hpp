/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#ifndef _IPFIX_RECEIVER_DTLSUDPIPV4_H_
#define _IPFIX_RECEIVER_DTLSUDPIPV4_H_

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

#include <openssl/ssl.h>
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif

/* Maximum amount of time in seconds a connection may remain in ACCEPT state. */
#define DTLS_ACCEPT_TIMEOUT 30
/* Idle timeout in seconds. If no datagrams have been received on a
 * connection for that amount of time the connection will be removed. */
#define DTLS_IDLE_TIMEOUT (12 * 60 * 60)
/* Time in seconds a connection stays in state SHUTDOWN before it gets
 * removed. */
#define DTLS_SHUTDOWN_TIMEOUT 10
/* Maximum number of concurrent connections */
#define DTLS_MAX_CONCURRENT_CONNECTIONS 50

#endif /* SUPPORT_DTLS */

class IpfixReceiverDtlsUdpIpV4 : public IpfixReceiver, Sensor {
#ifdef SUPPORT_DTLS
    public:
	IpfixReceiverDtlsUdpIpV4(int port, const std::string ipAddr = "",
	    const std::string &certificateChainFile = "", const std::string &privateKeyFile = "",
	    const std::string &caFile = "", const std::string &caPath = "",
	    const std::set<string> &peerFqdns = std::set<string>(), const uint32_t buffer = 0);
	virtual ~IpfixReceiverDtlsUdpIpV4();

	virtual void run();
	virtual std::string getStatisticsXML(double interval);
		
    private:
	int listen_socket;
	SSL_CTX_wrapper ssl_ctx;
	const std::set<string> peerFqdns;
	friend int verify_peer_cb_udp(void *context, const char *dnsname);
	uint32_t statReceivedPackets;  /**< number of received packets */ 

	class DtlsConnection {
	    public:
		DtlsConnection(IpfixReceiverDtlsUdpIpV4 &parent,struct sockaddr_in *clientAddress);
		~DtlsConnection();
		int consumeDatagram(boost::shared_ptr<IpfixRecord::SourceID> &sourceID, boost::shared_array<uint8_t> secured_data, size_t len);
		std::string inspect(bool includeState = true);
		bool isInactive();

	    private:
		struct sockaddr_in clientAddress;
		IpfixReceiverDtlsUdpIpV4 &parent;
		SSL* ssl;
		enum state_t { ACCEPTING, CONNECTED, SHUTDOWN };
		static const char *states[];
		state_t state;
		time_t last_used;
		int accept();
		void shutdown();
		int verify_peer();
	};
	typedef boost::shared_ptr<DtlsConnection> DtlsConnectionPtr;

	typedef bool (*CompareSourceID)(const IpfixRecord::SourceID&, const IpfixRecord::SourceID&);
	// compare based on source address and source port
	static bool my_CompareSourceID(const IpfixRecord::SourceID& lhs, const IpfixRecord::SourceID& rhs) {
	    int result;
	    if (lhs.exporterAddress.len < rhs.exporterAddress.len) return true;
	    if (lhs.exporterAddress.len > rhs.exporterAddress.len) return false;
	    result = memcmp(lhs.exporterAddress.ip, rhs.exporterAddress.ip, lhs.exporterAddress.len);
	    if (result < 0) return true;
	    if (result > 0) return false;
	    if (lhs.exporterPort < rhs.exporterPort) return true;
	    return false;
	}
	typedef std::map<IpfixRecord::SourceID,DtlsConnectionPtr,CompareSourceID> connections_map;
	connections_map connections;
	void idle_processing();

#ifdef DEBUG
	void dumpConnections(void);
#else
	inline void dumpConnections(void) {}
#endif

#else /* SUPPORT_DTLS */

    /* This is the code that gets compiled if SUPPORT_DTLS is not set */

    public:
	IpfixReceiverDtlsUdpIpV4(int port, const std::string ipAddr = "",
	    const std::string &certificateChainFile = "", const std::string &privateKeyFile = "",
	    const std::string &caFile = "", const std::string &caPath = "",
	    const std::set<string> &peerFqdns = std::set<string>() , const uint32_t buffer = 0) {
	    THROWEXCEPTION("DTLS over UDP not supported!");
	}

	virtual ~IpfixReceiverDtlsUdpIpV4() {}

	virtual void run() {}
	virtual std::string getStatisticsXML(double interval) { return ""; }

#endif /* SUPPORT_DTLS */
};

#endif
