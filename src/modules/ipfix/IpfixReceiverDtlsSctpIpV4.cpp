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

#ifdef SUPPORT_DTLS_OVER_SCTP
#include "IpfixReceiverDtlsSctpIpV4.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "common/msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sstream>

/** 
 * Does DTLS over SCTP/IPv4 specific initialization.
 * @param port Port to listen on
 */
IpfixReceiverDtlsSctpIpV4::IpfixReceiverDtlsSctpIpV4(int port, const std::string ipAddr,
	const std::string &certificateChainFile, const std::string &privateKeyFile,
	const std::string &caFile, const std::string &caPath,
	const std::set<string> &peerFqdnsParam, const uint32_t buffer)
    : IpfixReceiver(port),ssl_ctx(certificateChainFile,privateKeyFile,caFile,caPath,
	! peerFqdnsParam.empty()),listen_socket(-1),maxfd(0),
	peerFqdns(peerFqdnsParam), statReceivedPackets(0)
	 {
	
    struct sockaddr_in serverAddress;
    struct sctp_event_subscribe ses;
    int flags;

    /* FIXME: make error messages consistent with IpfixReceiverDtlsUdpIpV4 */

    try {
	listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(listen_socket < 0) {
	    /* FIXME: should we use strerror_r? */
	    msg(MSG_FATAL, "socket creation failed: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsSctpIpV4, socket creation failed: %s", strerror(errno));
	}
	/* set socket to non-blocking i/o */
	flags = fcntl(listen_socket,F_GETFL,0);
	flags |= O_NONBLOCK;
	if (fcntl(listen_socket,F_SETFL,flags)<0) {
	    THROWEXCEPTION("IPFIX: Failed to set socket to non-blocking i/o");
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
	    msg(MSG_FATAL, "Cannot bind socket: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsSctpIpV4 %s:%d",ipAddr.c_str(), port );
	}
	if(listen(listen_socket, SCTP_MAX_BACKLOG) < 0 ) {
	    msg(MSG_FATAL, "Can not listen socket: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsSctpIpV4 %s:%d",ipAddr.c_str(), port );
	}
	msg(MSG_INFO, "SCTP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								listen_socket);

	memset(&ses, 0, sizeof(ses));
	ses.sctp_data_io_event = 1;
	if ( setsockopt(listen_socket, IPPROTO_SCTP, SCTP_EVENTS, &ses, sizeof(ses))) {
	    msg(MSG_FATAL, "setsockopt() failed: %s", strerror(errno));
	    THROWEXCEPTION("Cannot create IpfixReceiverDtlsSctpIpV4 %s:%d",ipAddr.c_str(), port );
	}

	/* BIO_new_dgram_sctp sets the necessary socket options on listen_socket.
	 * This includes activating SCTP-AUTH and subscribing to all necessary events.
	 * We can delete this BIO right after because we're only calling it for the
	 * side effects on listen_socket. */
	BIO_free(BIO_new_dgram_sctp(listen_socket, BIO_NOCLOSE));

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(listen_socket,&readfds);
	update_maxfd();

	/* TODO: Find out what this is? */
	SensorManager::getInstance().addSensor(this, "IpfixReceiverDtlsSctpIpV4", 0);

	msg(MSG_INFO, "DTLS over SCTP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
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
 * Does SCTP/IPv4 specific cleanup
 */
IpfixReceiverDtlsSctpIpV4::~IpfixReceiverDtlsSctpIpV4() {
	close(listen_socket);
}

int verify_peer_cb_sctp(void *context, const char *dnsname) {
    IpfixReceiverDtlsSctpIpV4 *receiver =
	static_cast<IpfixReceiverDtlsSctpIpV4 *>(context);
    string strdnsname(dnsname);
    transform(strdnsname.begin(),strdnsname.end(),strdnsname.begin(),
	    ::tolower);
    if (receiver->peerFqdns.find(strdnsname)!=receiver->peerFqdns.end())
	return 1;
    else
	return 0;
}


/**
 * DTLS over SCTP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverDtlsSctpIpV4::run() {

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(struct sockaddr_in);

    int ret;
    int rfd;
    struct timespec timeOut;

    /* set a 400ms time-out on the pselect */
    timeOut.tv_sec = 0L;
    timeOut.tv_nsec = 400000000L;

    while(!exitFlag) {
	fd_set tmpreadfds = readfds;
	fd_set tmpwritefds = writefds;
	ret = pselect(maxfd + 1, &tmpreadfds, &tmpwritefds, NULL, &timeOut, NULL);
	if (ret == 0) {
	    /* Timeout */
	    continue;
	}
	if ((ret == -1) && (errno == EINTR)) {
	    DPRINTF("select() returned due to signal");
	    /* There was a signal... ignore */
	    continue;
	}
	if (ret < 0) {
	    msg(MSG_ERROR ,"select() returned with an error: %s",strerror(errno));
	    THROWEXCEPTION("IpfixReceiverDtlsSctpIpV4: terminating listener thread");
	    break;
	}
	DPRINTF("select() returned %d",ret);
	// looking for a new client to connect at listen_socket
	if (FD_ISSET(listen_socket, &tmpreadfds)){
	    rfd = accept(listen_socket, (struct sockaddr*)&clientAddress, &clientAddressLen);

	    if (rfd >= 0){
		if ( ! isHostAuthorized(&clientAddress.sin_addr,
			    sizeof(clientAddress.sin_addr))) {
		    /* Do not accept connections from unauthorized hosts. */
		    close(rfd);
		} else {
		    msg(MSG_DEBUG, "IpfixReceiverDtlsSctpIpV4: Client connected from %s:%d, FD=%d", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), rfd);
		    DtlsConnectionPtr conn = DtlsConnectionPtr( new DtlsConnection(*this,&clientAddress,rfd));
		    connections.insert(make_pair(rfd,conn));
		    update_maxfd();
		}
	    }else{
		msg(MSG_ERROR ,"accept() in ipfixReceiver failed");
		/* TODO: Don't throw an exception here. */
		THROWEXCEPTION("IpfixReceiverDtlsSctpIpV4: unable to accept new connection");
	    }
	}
	// check all connected sockets for new available data
	for (rfd = 0; rfd <= maxfd; ++rfd) {
	    if (rfd == listen_socket) continue;
	    if (FD_ISSET(rfd, &readfds) || FD_ISSET(rfd, &writefds)) {
		if (FD_ISSET(rfd, &readfds)) DPRINTF("descriptor %d is ready for reading",rfd);
		if (FD_ISSET(rfd, &writefds)) DPRINTF("descriptor %d is ready for writing",rfd);
		connections_map::iterator it = connections.find(rfd);
		if (it == connections.end()) {
		    /* This should not happend. */
		    msg(MSG_ERROR,"Can't find connection for file descriptor.");
		    FD_CLR(rfd,&readfds);
		    FD_CLR(rfd,&writefds);
		    continue;
		}
		ret = it->second->fdready();
		if (ret == 0) {
		    DPRINTF("fdready() returned 0. Deleting connection.");
		    remove_connection(rfd);
		    update_maxfd();
		}
	    }
	}
    }
    msg(MSG_DEBUG, "IpfixReceiverDtlsSctpIpV4: Exiting");
}

/**
 * statistics function called by StatisticsManager
 */
std::string IpfixReceiverDtlsSctpIpV4::getStatisticsXML(double interval)
{
	ostringstream oss;
	
	oss << "<receivedPackets>" << statReceivedPackets << "</receivedPackets>" << endl;	

	return oss.str();
}

void IpfixReceiverDtlsSctpIpV4::remove_connection(int socket) {
    connections.erase(socket);
}

void IpfixReceiverDtlsSctpIpV4::update_maxfd(void) {
    maxfd = 0;
    if (listen_socket>=0) maxfd = listen_socket;
    if (!connections.empty() && connections.rbegin()->first > maxfd)
	maxfd = connections.rbegin()->first;
}

IpfixReceiverDtlsSctpIpV4::DtlsConnection::DtlsConnection(IpfixReceiverDtlsSctpIpV4 &_parent,struct sockaddr_in *pclientAddress,int _socket) :
    clientAddress(*pclientAddress), socket(_socket), parent(_parent),
    last_used(time(NULL)), sourceID(new IpfixRecord::SourceID) {

    memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
    sourceID->exporterAddress.len = 4;
    sourceID->exporterPort = ntohs(clientAddress.sin_port);
    sourceID->protocol = IPFIX_protocolIdentifier_SCTP;
    sourceID->receiverPort = parent.receiverPort;
    sourceID->fileDescriptor = socket;

    ssl = parent.ssl_ctx.SSL_new();

    BIO *bio;
    /* create output abstraction for SSL object */
    bio = BIO_new_dgram_sctp(socket,BIO_NOCLOSE);
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &clientAddress);
    SSL_set_bio(ssl,bio,bio);
    SSL_set_accept_state(ssl);
    vpcd.cb = &verify_peer_cb_sctp;
    vpcd.context = &parent;
    SSL_set_ex_data(ssl,get_openssl_ex_data_idx_vpcd(),&vpcd);

    /* Pretend that the file description is ready */
    fdready();

}

IpfixReceiverDtlsSctpIpV4::DtlsConnection::~DtlsConnection() {
    DPRINTF("~DtlsConnection()");
    shutdown();
}

/* Return values:
 * 1 Success
 * 0 Failure. Remove this connection
 */
int IpfixReceiverDtlsSctpIpV4::DtlsConnection::fdready() {
    int ret, error;
    boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
    if (socket < 0) return 0;
    ret = SSL_read(ssl,data.get(),MAX_MSG_LEN);
    error = SSL_get_error(ssl,ret);
#ifdef DEBUG
    msg_openssl_return_code(MSG_DEBUG,"SSL_read()",ret,error);
    DPRINTF("Error: %s",strerror(errno));
    DPRINTF("Received shutdown: %s",SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN ? "yes":"no");
#endif
    if (ret<0) {
	if (error == SSL_ERROR_WANT_READ) {
	    FD_SET(socket,&parent.readfds);
	    FD_CLR(socket,&parent.writefds);
	    return 1;
	} else if (error == SSL_ERROR_WANT_WRITE) {
	    FD_SET(socket,&parent.writefds);
	    FD_CLR(socket,&parent.readfds);
	    return 1;
	}
	msg_openssl_return_code(MSG_ERROR,"SSL_read()",ret,error);
	msg_openssl_errors();
	shutdown();
	return 0;
    } else if (ret==0) {
	if (error == SSL_ERROR_ZERO_RETURN) {
	    // remote side closed connection
	    DPRINTF("remote side closed connection.");
	} else {
	    msg_openssl_return_code(MSG_ERROR,"SSL_read()",ret,error);
	    msg_openssl_errors();
	}
	shutdown();
	return 0;
    } else {
	DPRINTF("SSL_read() returned %d bytes.",ret);
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

void IpfixReceiverDtlsSctpIpV4::DtlsConnection::shutdown() {
    int ret, error;
    /* send empty packet to packet processors to signal end
       of connection. */
    boost::shared_array<uint8_t> data(NULL);
    parent.mutex.lock();
    for (std::list<IpfixPacketProcessor*>::iterator i = parent.packetProcessors.begin();
	    i != parent.packetProcessors.end(); ++i) { 
	(*i)->processPacket(data, 0, sourceID);
    }
    parent.mutex.unlock();
    if (!ssl) return;
    ret = SSL_shutdown(ssl);
    if (ret == 0) {
	DPRINTF("Calling SSL_shutdown a second time.");
	ret = SSL_shutdown(ssl);
    }
    error = SSL_get_error(ssl,ret);
#if DEBUG
    msg_openssl_return_code(MSG_DEBUG,"SSL_shutdown()",ret,error);
#endif
    DPRINTF("SSL_free(ssl)");
    parent.ssl_ctx.SSL_free(ssl);
    ssl = NULL;
    FD_CLR(socket,&parent.readfds);
    FD_CLR(socket,&parent.writefds);
    close(socket);
    socket = -1;
}

#endif /*SUPPORT_SCTP*/
