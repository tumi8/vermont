/** @file
 * IPFIX Receiving module.
 *
 * The IPFIX Receiver module receives messages from the network and passes it to 
 * into a parsing module.
 */



#include "ipfixReceiver.h"

#include "rcvIpfix.h"
#include "ipfix.h"
#include "msg.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


/******************************************* Forward declaration *********************************/

static int createUdpIpv4Receiver(IpfixReceiver* ipfixReceiver, int port);
static void* listenerThread(void* ipfixReceiver_);
static void destroyUdpReceiver(IpfixReceiver* ipfixReceiver);
static void udpListener(IpfixReceiver* ipfixReceiver);

/******************************************* Implementation *************************************/

/**
 * Initializes internal data.
 * Call once before using any function in this module
 * @return 0 on success
 */
int initializeIpfixReceivers() {
        return 0;
}

/**
 * Destroys internal data.
 * Call once to tidy up. Don't use any function in this module after a call
 * to this function.
 * @return 0 on success.
 */
int deinitializeIpfixReceivers() {
        return 0;
}

/**
 * Creates an IpfixReceiver. 
 * The created receiver will do type specific initialization. It's not
 * possible to change the Receiver_Type after creation time.
 * IpfixReceiver is ready to use, after a call to this function.
 * You should assign one (or more) PackeProcessor(s) for reasonable use
 * @c setPacketProcessor()
 * @param receiver_type Desired Transport/Network protocol
 * @param port Port to listen on
 * @return handle to the created instance.
 */
IpfixReceiver* createIpfixReceiver(Receiver_Type receiver_type, int port) {
        IpfixReceiver* ipfixReceiver;
        
        if(!(ipfixReceiver=(IpfixReceiver*)malloc(sizeof(IpfixReceiver)))) {
                msg(MSG_FATAL, "Ran out of memory");
                goto out0;
        }
        
        ipfixReceiver->receiver_type = receiver_type;
        
        ipfixReceiver->processorCount = 0;
        ipfixReceiver->packetProcessor = NULL;
        
        ipfixReceiver->authCount = 0;
        ipfixReceiver->authHosts = NULL;
	ipfixReceiver->exit = 0;
        
        if (pthread_mutex_init(&ipfixReceiver->mutex, NULL) != 0) {
                msg(MSG_FATAL, "Could not init mutex");
                goto out1;
        }
        
        if (pthread_mutex_lock(&ipfixReceiver->mutex) != 0) {
                msg(MSG_FATAL, "Could not lock mutex");
                goto out1;
        }
        
        switch (receiver_type) {
        case UDP_IPV4:
                createUdpIpv4Receiver(ipfixReceiver, port);
                break;
        case UDP_IPV6:
                msg(MSG_FATAL, "UDP over IPv6 support isn't implemented yet");
                goto out1;
        case TCP_IPV4:
                msg(MSG_FATAL, "TCP over IPv4 support is horribly broken. We won't start it");
                goto out1;
        case TCP_IPV6:
                msg(MSG_FATAL, "TCP over IPv6 support isn't implemented yet");
                goto out1;
        case SCTP_IPV4:
                msg(MSG_FATAL, "SCTP over IPv4 support isn't implemented yet");
                goto out1;
        case SCTP_IPV6:
                msg(MSG_FATAL, "SCTP over IPv6 support isn't implemented yet");
                goto out1;
        default:
                msg(MSG_FATAL, "Unknown protocol");
                goto out1;
        }


        if(pthread_create(&(ipfixReceiver->thread), 0, listenerThread, ipfixReceiver) != 0) {
                msg(MSG_FATAL, "Could not create listener thread");
                goto out1;
        }
        
        return ipfixReceiver;
out1:
        destroyIpfixReceiver(ipfixReceiver);
out0:
        return NULL;
}


/**
 * Frees memory used by an IpfixReceiver.
 * Attention: Memory of the assigned PacketProcessors is NOT freed.
 * This has to be done by the calling instance itself.
 * @param ipfixReceiver Handle returned by @c createIpfixReceiver()
 */
void destroyIpfixReceiver(IpfixReceiver* ipfixReceiver) {
        /* do the connection type specific cleanup */
        switch (ipfixReceiver->receiver_type) {
        case UDP_IPV4:
        case UDP_IPV6:
                destroyUdpReceiver(ipfixReceiver);
                break;
        case TCP_IPV4:
        case TCP_IPV6:
        case SCTP_IPV4:
        case SCTP_IPV6:
        default:
                msg(MSG_FATAL, "Unknown protocol");
        }
        
        /* general cleanup */
        
        if (pthread_mutex_unlock(&ipfixReceiver->mutex) != 0) {
                msg(MSG_FATAL, "Could not unlock mutex");
        }

        pthread_mutex_destroy(&ipfixReceiver->mutex);
       
        free(ipfixReceiver);
}


/**
 * Starts processing messages.
 * All sockets prepared by calls to @c createIpfixReceiver() will start
 * receiving messages until @c stopIpfixReceiver() is called.
 * @param ipfixReceiver handle to receiver, which should start receiving
 * @return 0 on success
 */
int startIpfixReceiver(IpfixReceiver* ipfixReceiver) {
        if (pthread_mutex_unlock(&ipfixReceiver->mutex) != 0) {
                msg(MSG_FATAL, "Could not unlock mutex");
                return -1;
        }
        return 0;
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixReceiver() call.
 * @return 0 on success, non-zero on error
 */
int stopIpfixReceiver(IpfixReceiver* ipfixReceiver) {
        if (pthread_mutex_lock(&ipfixReceiver->mutex) != 0) {
                msg(MSG_FATAL, "Could not lock mutex");
                return -1;
        }
        ipfixReceiver->exit = 1;
        return 0;
}

/**
 * Assigns a list of packetProcessors to the Receiver. The list has to be managed (creation and
 * destruction) by the calling instance.
 * @param ipfixReceiver handle of receiver, the packetProcessors should be assigned to
 * @param packetProcessor List of PacketProcessors
 * @param processorCount Number of PacketProcessors in the list.
 * @return 0 on success, non-zero on error
 */
int setPacketProcessors(IpfixReceiver* ipfixReceiver, void* packetProcessor, int processorCount) {
        ipfixReceiver->packetProcessor = packetProcessor;
        ipfixReceiver->processorCount = processorCount;

        return 0;
}

/**
 * Checks if PacketProcessors where assigned to the IpfixReceiver
 * @return 0 if no PacketProcessors where assigned, > 0 otherwise
 */
int hasPacketProcessor(IpfixReceiver* ipfixReceiver) {
        return ipfixReceiver->processorCount;
}


/**
 * Adds a struct in_addr to the list of hosts we accept packets from
 * @param ipfixReceiver IpfixReceiver to set the callback function for
 * @param host address to add to the list
 * @return 0 on success, non-zero on error
 */
int addAuthorizedHost(IpfixReceiver* ipfixReceiver, const char* host) {
        struct in_addr inaddr;

        if (inet_aton(host, &inaddr) == 0) {
                msg(MSG_ERROR, "Invalid host address: %s", host);
                return -1;
        }

        int n = ++ipfixReceiver->authCount;
        ipfixReceiver->authHosts = (struct in_addr*)realloc(ipfixReceiver->authHosts, n * sizeof(struct in_addr));
        memcpy(&ipfixReceiver->authHosts[n-1], &inaddr, sizeof(struct in_addr));
        
        return 0;
}

/**
 * Checks if a given host is a member of the list of authorized hosts
 * @param ipfixReceiver handle to an IpfixReceiver
 * @param inaddr Address of the host to check
 * @param addrlen Length of inaddr
 * @return 0 if host is NOT in list, non-zero otherwise
 */
int isHostAuthorized(IpfixReceiver* ipfixReceiver, struct in_addr* inaddr, int addrlen) {
        /* if we have a list of authorized hosts, discard message if sender is not in this list */
        if (ipfixReceiver->authCount > 0) {
                int i;
                for (i=0; i < ipfixReceiver->authCount; i++) {
                        if (memcmp(inaddr, &ipfixReceiver->authHosts[i], addrlen) == 0)
                                return 1;
                }
                /* isn't in list */
                return 0;
        }
        return 1;
}


/**
 * Thread function responsible for receiving packets from the network
 * @param ipfixReceiver_ handle to an IpfixReceiver created by @c createIpfixReceiver()
 * @return NULL
 */
static void* listenerThread(void* ipfixReceiver_) {
        IpfixReceiver* ipfixReceiver = (IpfixReceiver*)ipfixReceiver_;

        switch (ipfixReceiver->receiver_type) {
        case UDP_IPV4:
        case UDP_IPV6:
                udpListener(ipfixReceiver);
                break;
        case TCP_IPV4:
        case TCP_IPV6:
        case SCTP_IPV4:
        case SCTP_IPV6:
        default:
                msg(MSG_FATAL, "Unknown protocol");
        }

        return NULL;
}


/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void statsIpfixReceiver(void* ipfixReceiver_)
{
        IpfixReceiver* ipfixReceiver = (IpfixReceiver*)ipfixReceiver_;

	msg_stat("Concentrator: IpfixReceiver: %6d records received", ipfixReceiver->receivedRecords);
		ipfixReceiver->receivedRecords = 0;
}


/********************************************* Connection type specific functions ************************************************/


/** 
 * Does UDP/IPv4 specific initialization.
 * @param ipfixReceiver handle to an IpfixReceiver created by @createIpfixReceiver()
 * @param port Port to listen on
 * @return 0 on success, non-zero on error
 */
static int createUdpIpv4Receiver(IpfixReceiver* ipfixReceiver, int port) {
        struct sockaddr_in serverAddress;
        

        ipfixReceiver->listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(ipfixReceiver->listen_socket < 0) {
                perror("Could not create socket");
                return -1;
        }
        
	ipfixReceiver->exit = 0;
        
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);
        if(bind(ipfixReceiver->listen_socket, (struct sockaddr*)&serverAddress, 
		sizeof(struct sockaddr_in)) < 0) {
                perror("Could not bind socket");
                return -1;
        }
        return 0;
}


/**
 * Does UDP/IPv4 specific cleanup
 * @param ipfixReceiver handle to an IpfixReceiver, created by @createIpfixReceiver()
 */
static void destroyUdpReceiver(IpfixReceiver* ipfixReceiver) {
        close(ipfixReceiver->listen_socket);
}


/**
 * UDP specific listener function. This function is called by @c listenerThread(), when receiver_type is
 * UDP_IPV4 or UDP_IPV6.
 * @param ipfixReceiver handle to an IpfixReceiver, created by @createIpfixReceiver()
 */
static void udpListener(IpfixReceiver* ipfixReceiver) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLen;
        byte* data = (byte*)malloc(sizeof(byte)*MAX_MSG_LEN);
	SourceID *sourceID = (SourceID*)malloc(sizeof(SourceID));
        int n, i;
        
        while(!ipfixReceiver->exit) {
                clientAddressLen = sizeof(struct sockaddr_in);
                n = recvfrom(ipfixReceiver->listen_socket, data, MAX_MSG_LEN,
			     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
                if (n < 0) {
                        msg(MSG_DEBUG, "recvfrom returned without data, terminating listener thread");
                        break;
                }
                
                if (isHostAuthorized(ipfixReceiver, &clientAddress.sin_addr, 
				     sizeof(clientAddress.sin_addr))) {

                        uint32_t ip = ntohl(clientAddress.sin_addr.s_addr);
			memcpy(sourceID->exporterAddress.ip, &ip, 4);
			sourceID->exporterAddress.len = 4;

                        pthread_mutex_lock(&ipfixReceiver->mutex);
                        IpfixPacketProcessor* pp = (IpfixPacketProcessor*)(ipfixReceiver->packetProcessor);
                        for (i = 0; i != ipfixReceiver->processorCount; ++i) { 
                         	pthread_mutex_lock(&pp[i].mutex);
				pp[i].processPacketCallbackFunction(pp[i].ipfixParser, data, n, sourceID);
                        	pthread_mutex_unlock(&pp[i].mutex);
			}
                        pthread_mutex_unlock(&ipfixReceiver->mutex);
                }
                else{
                        msg(MSG_DEBUG, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
                }
        }
        
        free(data);
}

