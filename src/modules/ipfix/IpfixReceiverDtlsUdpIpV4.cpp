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

#ifdef SUPPORT_DTLS
#include "IpfixReceiverDtlsUdpIpV4.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "common/msg.h"
#include "common/openssl/OpenSSL.h"

#include <stdexcept>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>

#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif


using namespace std;

/** 
 * Does DTLS over UDP/IPv4 specific initialization.
 * @param port Port to listen on
 * @param ipAddr IP address to bind to our socket, if equals "", no specific IP address will be bound.
 */
IpfixReceiverDtlsUdpIpV4::IpfixReceiverDtlsUdpIpV4(int port, const std::string ipAddr,
	const std::string &certificateChainFile, const std::string &privateKeyFile,
	const std::string &caFile, const std::string &caPath,
	const std::set<string> &peerFqdnsParam, const uint32_t buffer,
	unsigned int moduleId)
    : IpfixReceiver(port),listen_socket(-1),
	ssl_ctx(certificateChainFile,privateKeyFile,caFile,caPath, ! peerFqdnsParam.empty()),
	peerFqdns(peerFqdnsParam),
	statReceivedPackets(0),
	connections(my_CompareSourceID)
	 {
    struct sockaddr_in serverAddress;

    try {
	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
	    /* FIXME: should we use strerror_r? */
	    msg(LOG_CRIT, "Could not create socket: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsUdpIpV4, socket creation failed");
	}

	setBufferSize(listen_socket, buffer);
	
	// if ipAddr set: Bind a specific IP address to our socket.
	// else: use wildcard address
	if(ipAddr.empty())
	    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	else
	    if ( ! inet_pton(AF_INET,ipAddr.c_str(),&serverAddress.sin_addr) ) {
		THROWEXCEPTION("IP address invalid.");
	    }
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	if(bind(listen_socket, (struct sockaddr*)&serverAddress, 
	    sizeof(struct sockaddr_in)) < 0) {
	    msg(LOG_CRIT, "Could not bind socket: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsUdpIpV4 %s:%d",ipAddr.c_str(), port );
	}

	/* TODO: Find out what this is? */
	SensorManager::getInstance().addSensor(this, "IpfixReceiverDtlsUdpIpV4",
								moduleId);

	msg(LOG_NOTICE, "DTLS over UDP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								listen_socket);
    } catch(...) {
	if (listen_socket>=0) {
	    close(listen_socket);
	    listen_socket = -1;
	}
	throw; // rethrow
    }
}


/**
 * Does UDP/IPv4 specific cleanup
 */
IpfixReceiverDtlsUdpIpV4::~IpfixReceiverDtlsUdpIpV4() {
    close(listen_socket);
}

#ifdef DEBUG
void IpfixReceiverDtlsUdpIpV4::dumpConnections() {
    struct in_addr addr;
    char ipaddr[INET_ADDRSTRLEN];
    DPRINTF_INFO("Dumping DTLS connections:");
    if (connections.empty()) {
	DPRINTF_INFO("  (none)");
	return;
    }
    connections_map::const_iterator it = connections.begin();
    for(;it!=connections.end();it++) {
	const IpfixRecord::SourceID &sourceID = it->first;
	memcpy(&addr.s_addr,sourceID.exporterAddress.ip, sourceID.exporterAddress.len);
	inet_ntop(AF_INET,&addr,ipaddr,sizeof(ipaddr));
	DPRINTF_INFO("  %s",it->second->inspect().c_str());
    }
}
#endif


const char *IpfixReceiverDtlsUdpIpV4::DtlsConnection::states[] = {
    "ACCEPTING","CONNECTED","SHUTDOWN"
};

int verify_peer_cb_udp(void *context, const char *dnsname) {
    IpfixReceiverDtlsUdpIpV4 *receiver =
	static_cast<IpfixReceiverDtlsUdpIpV4 *>(context);
    string strdnsname(dnsname);
    transform(strdnsname.begin(),strdnsname.end(),strdnsname.begin(),
	    ::tolower);
    if (receiver->peerFqdns.find(strdnsname)!=receiver->peerFqdns.end())
	return 1;
    else
	return 0;
}

int IpfixReceiverDtlsUdpIpV4::DtlsConnection::verify_peer() {
    return verify_ssl_peer(ssl,&verify_peer_cb_udp,&parent);
}

/**
 * UDP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverDtlsUdpIpV4::run() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen;
    clientAddressLen = sizeof(struct sockaddr_in);
    
    fd_set fd_array; //all active filedescriptors
    fd_set readfds;  //parameter for for pselect

    int ret;
    struct timespec timeOut;
    int timeout_count = 0;

    FD_ZERO(&fd_array);
    FD_SET(listen_socket, &fd_array);

    /* set a 400ms time-out on the pselect */
    timeOut.tv_sec = 0L;
    timeOut.tv_nsec = 400000000L;
    
    while(!exitFlag) {
	readfds = fd_array; // because select() changes readfds
	ret = pselect(listen_socket + 1, &readfds, NULL, NULL, &timeOut, NULL);
	if (ret == 0) {
	    /* Timeout */
	    if (++timeout_count == 10) {
		idle_processing();
		timeout_count = 0;
	    }
	    continue;
	}
	if ((ret == -1) && (errno == EINTR)) {
	    /* There was a signal... ignore */
	    continue;
	}
	if (ret < 0) {
	    msg(LOG_ERR ,"select() returned with an error");
	    THROWEXCEPTION("IpfixReceiverDtlsUdpIpV4: terminating listener thread");
	    break;
	}

	boost::shared_array<uint8_t> secured_data(new uint8_t[MAX_MSG_LEN]);
	boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
	boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);

	ret = recvfrom(listen_socket, secured_data.get(), MAX_MSG_LEN,
		     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
	if (ret < 0) {
	    msg(LOG_CRIT, "recvfrom returned without data, terminating listener thread");
	    break;
	}
	if ( ! isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {
	    msg(LOG_CRIT, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
	    continue;
	}
#ifdef DEBUG
	char ipaddr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET,&clientAddress.sin_addr,ipaddr,sizeof(ipaddr));
	DPRINTF_INFO("Received packet of size %d from %s:%d",ret,ipaddr,ntohs(clientAddress.sin_port));
#endif
	/* Set up sourceID */
	memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
	sourceID->exporterAddress.len = 4;
	sourceID->exporterPort = ntohs(clientAddress.sin_port);
	sourceID->protocol = IPFIX_protocolIdentifier_UDP;
	sourceID->receiverPort = receiverPort;
	sourceID->fileDescriptor = listen_socket;
	/* Search for an existing connection with same source IP and port */
	connections_map::iterator it = connections.find(*sourceID);
	DtlsConnectionPtr conn;
	if (it == connections.end()) {
	    /* create a new connection if we did not find any. */
	    DPRINTF_INFO("New connection");
	    if (connections.size() >= DTLS_MAX_CONCURRENT_CONNECTIONS) {
		msg(LOG_ERR,"Maximum number (%d) of concurrent "
			"connections reached. Ignoring new connection "
			"attempt.",DTLS_MAX_CONCURRENT_CONNECTIONS);
		continue;

	    }
	    conn = DtlsConnectionPtr( new DtlsConnection(*this,&clientAddress));
	    it = connections.insert(make_pair(*sourceID,conn)).first;
	} else {
	    /* Use existing connection */
	    DPRINTF_INFO("Found existing connection.");
	    conn = it->second;
	}
	conn->consumeDatagram(sourceID, secured_data,ret);
	dumpConnections();
	    
    }
    msg(LOG_INFO, "IpfixReceiverDtlsUdpIpV4: Exiting");
}

/**
 * statistics function called by StatisticsManager
 */
std::string IpfixReceiverDtlsUdpIpV4::getStatisticsXML(double interval)
{
	ostringstream oss;
	
	oss << "<receivedPackets>" << statReceivedPackets << "</receivedPackets>" << endl;	

	return oss.str();
}

IpfixReceiverDtlsUdpIpV4::DtlsConnection::DtlsConnection(IpfixReceiverDtlsUdpIpV4 &_parent,struct sockaddr_in *pclientAddress) :
    parent(_parent), state(ACCEPTING), last_used(time(NULL)) {

    ssl = parent.ssl_ctx.SSL_new();

    memcpy(&clientAddress, pclientAddress, sizeof clientAddress);

    BIO *wbio, *rbio;
    /* create output abstraction for SSL object */
    wbio = BIO_new_dgram(parent.listen_socket,BIO_NOCLOSE);

    /* create a dummy BIO that always returns EOF */
    rbio = BIO_new(BIO_s_mem());
    /* -1 means EOF */
    BIO_set_mem_eof_return(rbio,-1);
    SSL_set_bio(ssl,rbio,wbio);
    SSL_set_accept_state(ssl);

    BIO_ctrl(SSL_get_wbio(ssl),BIO_CTRL_DGRAM_SET_PEER,0,&clientAddress);

}

IpfixReceiverDtlsUdpIpV4::DtlsConnection::~DtlsConnection() {
    if (ssl) parent.ssl_ctx.SSL_free(ssl);
}

#ifdef DEBUG
std::string IpfixReceiverDtlsUdpIpV4::DtlsConnection::inspect(bool includeState) {
    std::ostringstream o;
    char ipaddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&clientAddress.sin_addr,ipaddr,sizeof(ipaddr));
    o << ipaddr << ":" << ntohs(clientAddress.sin_port);
    if (includeState)
	o << " state:" << states[state];
    return o.str();
}
#endif

/* Accepts a DTLS connection
 * Returns values:
 * 1 Successfully connected (don't call accept() again)
 * 0 Not yet connected (call accept() again as soon as new data is available)
 * -1 Failure. Remove this connection
 */
int IpfixReceiverDtlsUdpIpV4::DtlsConnection::accept() {
    int ret, error;
    char buf[512];
    ret = SSL_accept(ssl);
    if (SSL_get_shared_ciphers(ssl,buf,sizeof buf) != NULL)
#ifdef DEBUG
       DPRINTF_INFO("Shared ciphers:%s",buf);
#else
       { /* do nothing */ }
#endif
    if (ret==1) {
	state = CONNECTED;
	DPRINTF_INFO("SSL_accept() succeeded.");
#ifdef DEBUG
	const char *str=SSL_CIPHER_get_name(SSL_get_current_cipher(ssl));
	DPRINTF_INFO("CIPHER is %s",(str != NULL)?str:"(NONE)");
#endif
	if (parent.ssl_ctx.get_verify_peers()) {
	    if (verify_peer()) {
		DPRINTF_INFO("Peer authentication successful.");
	    } else {
		msg(LOG_ERR,"Peer authentication failed. Shutting down connection.");
		shutdown();
		return -1;
	    }
	}
	return 1;
    }
    error = SSL_get_error(ssl,ret);
    DPRINTF_INFO("SSL_accept() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
    if (ret==-1 && error == SSL_ERROR_WANT_READ) {
	DPRINTF_INFO("SSL_accept() returned SSL_ERROR_WANT_READ");
	return 0;
    }
    msg(LOG_ERR,"SSL_accept() failed.");
    long verify_result = SSL_get_verify_result(ssl);
    if(SSL_get_verify_result(ssl)!=X509_V_OK) {
	msg(LOG_ERR,"Last verification error: %s", X509_verify_cert_error_string(verify_result));
    }
    state = SHUTDOWN;
    msg_openssl_errors();
    return -1;
}

void IpfixReceiverDtlsUdpIpV4::DtlsConnection::shutdown() {
    int ret;
#ifdef DEBUG
    int error;
#endif
    ret = SSL_shutdown(ssl);
    if (ret == 0) {
	DPRINTF_INFO("Calling SSL_shutdown a second time.");
	ret = SSL_shutdown(ssl);
    }
#ifdef DEBUG    
    error = SSL_get_error(ssl,ret);
    DPRINTF_INFO("SSL_shutdown() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
#endif
    state = SHUTDOWN;
}

/* Return values:
 * 1 Success
 * 0 Failure. Remove this connection
 */

int IpfixReceiverDtlsUdpIpV4::DtlsConnection::consumeDatagram(
	boost::shared_ptr<IpfixRecord::SourceID> &sourceID,
	boost::shared_array<uint8_t> secured_data, size_t len) {

    int ret, error;

    last_used = time(NULL);

    if (state == SHUTDOWN) {
	DPRINTF_INFO("state == SHUTDOWN. Ignoring datagram");
	return 1;
    }
#ifdef DEBUG
    if ( ! BIO_eof(SSL_get_rbio(ssl))) {
	msg(LOG_ERR,"EOF *not* reached on BIO. This should not happen.");
    }
#endif
    BIO_free(SSL_get_rbio(ssl));
    SSL_set_bio(ssl, BIO_new_mem_buf(secured_data.get(),len), SSL_get_wbio(ssl));
    BIO_set_mem_eof_return(SSL_get_rbio(ssl),-1);
    if (state == ACCEPTING) {
	ret = accept();
	if (ret == 0) return 1;
	if (ret == -1) return 0;
#ifdef DEBUG
	if ( ! BIO_eof(SSL_get_rbio(ssl))) {
	    msg(LOG_ERR,"EOF *not* reached on BIO. This should not happen.");
	}
#endif
	if (BIO_eof(SSL_get_rbio(ssl))) return 1; /* This should always be the case */
    }
    boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
    ret = SSL_read(ssl,data.get(),MAX_MSG_LEN);
    error = SSL_get_error(ssl,ret);
    DPRINTF_INFO("SSL_read() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
    if (ret<0) {
	if (error == SSL_ERROR_WANT_READ)
	    return 1;
	msg(LOG_ERR,"SSL_read() failed. SSL_get_error() returned: %d",error);
	msg_openssl_errors();
	shutdown();
	return 0;
    } else if (ret==0) {
	if (error == SSL_ERROR_ZERO_RETURN) {
	    // remote side closed connection
	    DPRINTF_INFO("remote side closed connection.");
	} else {
	    msg(LOG_ERR,"SSL_read() returned 0. SSL_get_error() returned: %d",error);
	    msg_openssl_errors();
	}
	shutdown();
	return 0;
    } else {
	DPRINTF_INFO("SSL_read() returned %d bytes.",ret);
    }
    parent.statReceivedPackets++;
    parent.mutex.lock();
    for (std::list<IpfixPacketProcessor*>::iterator i = parent.packetProcessors.begin();
	    i != parent.packetProcessors.end(); ++i) { 
	(*i)->processPacket(data, ret, sourceID);
    }
    parent.mutex.unlock();
    return 1;
}

void IpfixReceiverDtlsUdpIpV4::idle_processing() {
    /* Iterate over all connections and remove those that appear
     * to be unused. */
    connections_map::iterator tmp;
    connections_map::iterator it = connections.begin();
    bool changed = false;
    while(it!=connections.end()) {
	tmp = it++;
	if (tmp->second->isInactive()) {
	    DPRINTF_INFO("Removing connection %s",tmp->second->inspect(false).c_str());
	    connections.erase(tmp);
	    changed = true;
	}
    }
    if (changed) {
	dumpConnections();
    }
}

/* Checks whether the connection timed out.
 * Watch out: This method has side effects. If the connection did
 * timeout it returns true and expects the caller to remove the connection
 * from the connection map.
 * Return values:
 * false: Connection is still active. Keep it.
 * true:  Connection is idle and has been shut down. Remove it! */

bool IpfixReceiverDtlsUdpIpV4::DtlsConnection::isInactive() {
    time_t diff = time(NULL) - last_used;
    switch (state) {
	case ACCEPTING:
	    if (diff > DTLS_ACCEPT_TIMEOUT) {
		DPRINTF_INFO("accept timed out on %s",inspect(false).c_str());
		shutdown();
		return true;
	    }
	    break;
	case CONNECTED:
	    if (diff > DTLS_IDLE_TIMEOUT) {
		DPRINTF_INFO("idle timeout on %s",inspect(false).c_str());
		shutdown();
		return true;
	    }
	    break;
	case SHUTDOWN:
	    if (diff > DTLS_SHUTDOWN_TIMEOUT) {
		DPRINTF_INFO("shutdown timeout on %s",inspect(false).c_str());
		return true;
	    }
	    break;
    }
    return false;
}


#endif /*SUPPORT_DTLS*/

