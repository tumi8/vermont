/*
 PSAMP Reference Implementation
 Observer.cpp
 Implementation of the packet capturing thread
 Author: Michael Drueing <michael@drueing.de>

 changed by: Ronny T. Lampert
             Gerhard Münz
 */


#include "Observer.h"

#include "common/msg.h"
#include "common/Thread.h"
#include "common/defs.h"

#include <pcap.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <math.h>

/* Code adopted from tcpreplay: */
/* subtract uvp from tvp and store in vvp */
#ifndef timersub
#define timersub(tvp, uvp, vvp)                 \
     do {                                        \
         (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;      \
         (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;   \
         if ((vvp)->tv_usec < 0) {               \
             (vvp)->tv_sec--;                    \
             (vvp)->tv_usec += 1000000;          \
         }                                       \
     } while (0)
#endif
/* add tvp and uvp and store in vvp */
#ifndef timeradd
#define timeradd(tvp, uvp, vvp)                 \
    do {                                        \
        (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;      \
        (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;   \
        if ((vvp)->tv_usec >= 1000000) {        \
            (vvp)->tv_sec++;                    \
            (vvp)->tv_usec -= 1000000;          \
        }                                       \
    } while (0)
#endif
/* compare tvp and uvp using cmp */
#ifndef timercmp
#define timercmp(tvp, uvp, cmp)                 \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?         \
    ((tvp)->tv_usec cmp (uvp)->tv_usec) :       \
    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#endif
/* multiply tvp by x and store in uvp */
#define timermul(tvp, uvp, x)                   \
    do {                                        \
        (uvp)->tv_sec = (tvp)->tv_sec * x;      \
        (uvp)->tv_usec = (tvp)->tv_usec * x;    \
        while((uvp)->tv_usec > 1000000) {       \
            (uvp)->tv_sec++;                    \
            (uvp)->tv_usec -= 1000000;          \
        }                                       \
    } while(0)
/* multiply tvp by x and store in uvp (with cast) */
#define timermulfloat(tvp, uvp, x)              \
    do {                                        \
        (uvp)->tv_sec = (time_t)((tvp)->tv_sec * x);      \
        (uvp)->tv_usec = (suseconds_t)((tvp)->tv_usec * x);    \
        while((uvp)->tv_usec > 1000000) {       \
            (uvp)->tv_sec++;                    \
            (uvp)->tv_usec -= 1000000;          \
        }                                       \
    } while(0)

using namespace std;


InstanceManager<Packet> Observer::packetManager("Packet");

Observer::Observer(const std::string& interface, bool offline, uint64_t maxpackets) : thread(Observer::observerThread, "Observer"), allDevices(NULL),
	captureDevice(NULL), capturelen(PCAP_DEFAULT_CAPTURE_LENGTH), pcap_timeout(PCAP_TIMEOUT),
	pcap_promisc(1), maxPackets(maxpackets), ready(false), filter_exp(0), observationDomainID(0), // FIXME: this must be configured!
	receivedBytes(0), lastReceivedBytes(0), processedPackets(0),
	lastProcessedPackets(0),
	captureInterface(NULL), fileName(NULL), replaceTimestampsFromFile(false),
	stretchTimeInt(1), stretchTime(1.0), autoExit(true), slowMessageShown(false),
	statTotalLostPackets(0), statTotalRecvPackets(0)
{
	if(offline) {
		readFromFile = true;
		fileName = (char*)malloc(interface.size() + 1);
		strcpy(fileName, interface.c_str());
	} else {
		readFromFile = false;
		captureInterface = (char*)malloc(interface.size() + 1);
		strcpy(captureInterface, interface.c_str());
	}

	usedBytes += sizeof(Observer)+interface.size()+1;

	if(capturelen > PCAP_MAX_CAPTURE_LENGTH) {
		THROWEXCEPTION("compile-time parameter PCAP_DEFAULT_CAPTURE_LENGTH (%d) exceeds maximum capture length %d, "
				"adjust compile-time parameter PCAP_MAX_CAPTURE_LENGTH!", capturelen, PCAP_DEFAULT_CAPTURE_LENGTH);

	}
}

Observer::~Observer()
{
	msg(MSG_DEBUG, "Observer: destructor called");

	// to make sure that exitFlag is set and performShutdown() is called
	shutdown(false);

	/* collect and output statistics */
	pcap_stat pstats;
	if (captureDevice && pcap_stats(captureDevice, &pstats)==0) {
		msg(MSG_DIALOG, "PCAP statistics (INFO: if statistics were activated, this information does not contain correct data!):");
		msg(MSG_DIALOG, "Number of packets received on interface: %u", pstats.ps_recv);
		msg(MSG_DIALOG, "Number of packets dropped by PCAP: %u", pstats.ps_drop);
	}

	msg(MSG_DEBUG, "freeing pcap/devices");
	if(captureDevice) {
		pcap_close(captureDevice);
	}

	/* no pcap_freecode here, is already done after attaching the filter */

	if(allDevices) {
		pcap_freealldevs(allDevices);
	}

	free(captureInterface);
	delete[] filter_exp;
	if (fileName) { free(fileName); fileName = NULL; }
	msg(MSG_DEBUG, "successful shutdown");
}
/*
 This is the main observer loop. It graps packets from libpcap and
 dispatches them to the registered receivers.
 */
void *Observer::observerThread(void *arg)
{
	/* first we need to get the instance back from the void *arg */
	Observer *obs=(Observer *)arg;
	InstanceManager<Packet>& packetManager = obs->packetManager;

	Packet *p = NULL;
	const unsigned char *pcapData;
	struct pcap_pkthdr packetHeader;
	bool have_send = false;
	obs->registerCurrentThread();
	bool file_eof = false;

	msg(MSG_INFO, "Observer started with following parameters:");
	msg(MSG_INFO, "  - readFromFile=%d", obs->readFromFile);
	if (obs->fileName) msg(MSG_INFO, "  - fileName=%s", obs->fileName);
	if (obs->captureInterface) msg(MSG_INFO, "  - captureInterface=%s", obs->captureInterface);
	msg(MSG_INFO, "  - filterString='%s'", (obs->filter_exp ? obs->filter_exp : "none"));
	msg(MSG_INFO, "  - maxPackets=%u", obs->maxPackets);
	msg(MSG_INFO, "  - capturelen=%d", obs->capturelen);
	msg(MSG_INFO, " - dataLinkType=%d", obs->dataLinkType);
	if (obs->readFromFile) {
		msg(MSG_INFO, "  - autoExit=%d", obs->autoExit);
		msg(MSG_INFO, "  - stretchTime=%f", obs->stretchTime);
		msg(MSG_INFO, "  - replaceTimestampsFromFile=%s", obs->replaceTimestampsFromFile==true?"true":"false");
	}

	// start capturing packets
	msg(MSG_INFO, "now running capturing thread for device %s", obs->captureInterface);


	if(!obs->readFromFile) {
		while(!obs->exitFlag && (obs->maxPackets==0 || obs->processedPackets<obs->maxPackets)) {
			// wait until data can be read from pcap file descriptor
			fd_set fd_wait;
			FD_ZERO(&fd_wait);
			FD_SET(pcap_fileno(obs->captureDevice), &fd_wait);
			struct timeval st;
			st.tv_sec = 1;
			st.tv_usec = 0;
			int result = select(FD_SETSIZE, &fd_wait, NULL, NULL, &st);
			if (result == -1) {
				if (errno==EINTR) continue; // just continue on interrupted system call
				msg(MSG_FATAL, "select() on pcap file descriptor returned -1, error: %s", strerror(errno));
				msg(MSG_FATAL, "shutting down observer");
				break;
			}
			if (result == 0) {
				continue;
			}

			/*
			 get next packet (no zero-copy possible *sigh*)
			 NOTICE: potential bottleneck, if pcap_next() is calling gettimeofday() at a high rate;
			 there is partially caching function described in an Sun or IBM (Developerworks) article
			 that can act as a via LD_PRELOAD used overlay function.
			 unfortunately I don't have an URL ready -Freek
			 */
			DPRINTFL(MSG_VDEBUG, "trying to get packet from pcap");
			pcapData = pcap_next(obs->captureDevice, &packetHeader);
			if(!pcapData)
			/* no packet data was available */
			continue;
			DPRINTFL(MSG_VDEBUG, "got new packet!");

			// show current packet as c-structure on stdout
			//for (unsigned int i=0; i<packetHeader.caplen; i++) {
			//printf("0x%02hhX, ", ((unsigned char*)pcapData)[i]);
			//}
			//printf("\n");

			// initialize packet structure (init copies packet data)
			p = packetManager.getNewInstance();
			p->init((char*)pcapData, packetHeader.caplen, packetHeader.ts, obs->observationDomainID, packetHeader.len, obs->dataLinkType);

			DPRINTF("received packet at %u.%04u, len=%d",
					(unsigned)p->timestamp.tv_sec,
					(unsigned)p->timestamp.tv_usec / 1000,
					packetHeader.caplen
			);

			// update statistics
			obs->receivedBytes += packetHeader.caplen;
			obs->processedPackets++;

			while (!obs->exitFlag) {
				DPRINTFL(MSG_VDEBUG, "trying to push packet to queue");
				if ((have_send = obs->send(p))) {
					DPRINTFL(MSG_VDEBUG, "packet pushed");
					break;
				}
			}
		}
	} else {
		// file handle
		FILE* fh = pcap_file(obs->captureDevice);
		// timestamps in current time
		struct timeval now, start = {0,0};
		// timestamps in old time
		struct timeval first = {0,0};
		// differences
		struct timeval wait_val, delta_now, delta_file, delta_to_be;

		// make compiler happy ...
		delta_to_be.tv_sec = 0;
		delta_to_be.tv_usec = 0;

		struct timespec wait_spec;
		bool firstPacket = true;
		// read-from-file loop
		while(!obs->exitFlag && (obs->maxPackets==0 || obs->processedPackets<obs->maxPackets)) {

			DPRINTFL(MSG_VDEBUG, "trying to get packet from pcap file");
			pcapData=pcap_next(obs->captureDevice, &packetHeader);
			if(!pcapData) {
				/* no packet data was available */
				if(feof(fh))
				        msg(MSG_DIALOG, "Observer: reached end of file (%llu packets)", obs->processedPackets);
                        file_eof = true;
      				break;
      			}
			DPRINTFL(MSG_VDEBUG, "got new packet!");
			if (obs->stretchTime > 0) {
				if (gettimeofday(&now, NULL) < 0) {
					msg(MSG_FATAL, "Error gettimeofday: %s", strerror(errno));
					break;
				}
				if(firstPacket)
				{
					start = now;
					first = packetHeader.ts;
					firstPacket = false;
				} else {
					timersub(&now, &start, &delta_now);
					timersub(&packetHeader.ts, &first, &delta_file);
					if(obs->stretchTimeInt != 1) {
						if(obs->stretchTimeInt == 0)
							timermulfloat(&delta_file, &delta_to_be, obs->stretchTime);
						else
							timermul(&delta_file, &delta_to_be, obs->stretchTimeInt);
					}
					else
						delta_to_be = delta_file;
					DPRINTF("delta_now %d.%d delta_to_be %d.%d", delta_now.tv_sec, delta_now.tv_usec,  delta_to_be.tv_sec, delta_to_be.tv_usec);
					if(timercmp(&delta_now, &delta_to_be, <))
					{
						timersub(&delta_to_be, &delta_now, &wait_val);
						wait_spec.tv_sec = wait_val.tv_sec;
						wait_spec.tv_nsec = wait_val.tv_usec * 1000;
						if(nanosleep(&wait_spec, NULL) != 0)
							msg(MSG_INFO, "Observer: nanosleep returned nonzero value, errno=%u (%s)", errno, strerror(errno));
					}
					else if (delta_now.tv_sec > (delta_to_be.tv_sec + 1) && obs->stretchTime!=INFINITY)
					    if (!obs->slowMessageShown) {
					    	obs->slowMessageShown = true;
					    	msg(MSG_ERROR, "Observer: reading from file is more than 1 second behind schedule!");
					    }
				}
			}
			// optionally replace the timestamp with current time
			if (obs->replaceTimestampsFromFile)
			    timeradd(&start, &delta_to_be, &packetHeader.ts);

			// initialize packet structure (init copies packet data)
			p = obs->packetManager.getNewInstance();
			p->init((char*)pcapData,
				// in contrast to live capturing, the data length is not limited
				// to any snap length when reading from a pcap file
				(packetHeader.caplen < obs->capturelen) ? packetHeader.caplen : obs->capturelen,
				packetHeader.ts, obs->observationDomainID, packetHeader.len, obs->dataLinkType);


			DPRINTF("received packet at %u.%03u, len=%d",
				(unsigned)p->timestamp.tv_sec,
				(unsigned)p->timestamp.tv_usec / 1000,
				packetHeader.caplen
				);

			// update statistics
			obs->receivedBytes += packetHeader.caplen;
			obs->processedPackets++;

			while (!obs->exitFlag) {
				DPRINTFL(MSG_VDEBUG, "trying to push packet to queue");
				if ((have_send = obs->send(p))) {
					DPRINTFL(MSG_VDEBUG, "packet pushed");
					break;
				}
			}
		}
	}

	if (obs->autoExit && (file_eof || (obs->maxPackets && obs->processedPackets>=obs->maxPackets)) ) {
		// notify Vermont to shut down
		DPRINTF("notifying Vermont to shut down, as all PCAP file data was read, or maximum packet count was reached");
		obs->shutdownVermont();
	}

	msg(MSG_DEBUG, "exiting observer thread");
	obs->unregisterCurrentThread();
	pthread_exit((void *)1);
}


/*
 call after an Observer has been created
 error checking on pcap here, because it can't be done in the constructor
 and it may be too late, if done in the thread
 */
bool Observer::prepare(const std::string& filter)
{
	struct in_addr i_netmask, i_network;

	// we need to store the filter expression, because pcap needs
	// a char* and doesn't accept a const char* ... nasty pcap-devs!!!
	if (!filter.empty()) {
		filter_exp = new char[filter.size() + 1];
		strcpy(filter_exp, filter.c_str());
		usedBytes += filter.size()+1;
	}

	if (!readFromFile) {
		// query all available capture devices
		msg(MSG_INFO, "Finding devices");
		if(pcap_findalldevs(&allDevices, errorBuffer) == -1) {
			msg(MSG_FATAL, "error getting list of interfaces: %s", errorBuffer);
			goto out;
		}

		for(pcap_if_t *dev = allDevices; dev != NULL; dev=dev->next) {
			msg(MSG_DEBUG, "PCAP: name=%s, desc=%s", dev->name, dev->description);
		}

		msg(MSG_INFO,
		    "pcap opening interface=%s, promisc=%d, snaplen=%d, timeout=%d",
		    captureInterface, pcap_promisc, capturelen, pcap_timeout
		   );
		captureDevice=pcap_open_live(captureInterface, capturelen, pcap_promisc, pcap_timeout, errorBuffer);
		// check for errors
		if(!captureDevice) {
			msg(MSG_FATAL, "Error initializing pcap interface: %s", errorBuffer);
			goto out1;
		}

		// make reads non-blocking
		if(pcap_setnonblock(captureDevice, 1, errorBuffer) == -1) {
			msg(MSG_FATAL, "Error setting pcap interface to non-blocking: %s", errorBuffer);
			goto out2;
		}

		/* we need the netmask for the pcap_compile */
		if(pcap_lookupnet(captureInterface, &network, &netmask, errorBuffer) == -1) {
			msg(MSG_ERROR, "unable to determine netmask/network: %s", errorBuffer);
			network=0;
			netmask=0;
		}
		i_network.s_addr=network;
		i_netmask.s_addr=netmask;
		msg(MSG_DEBUG, "pcap seems to run on network %s", inet_ntoa(i_network));
		msg(MSG_INFO, "pcap seems to run on netmask %s", inet_ntoa(i_netmask));
	} else {
		captureDevice=pcap_open_offline(fileName, errorBuffer);
		// check for errors
		if(!captureDevice) {
			msg(MSG_FATAL, "Error opening pcap file %s: %s", fileName, errorBuffer);
			goto out1;
		}

		netmask=0;
	}

	dataLinkType = pcap_datalink(captureDevice);

	if (filter_exp) {
		msg(MSG_DEBUG, "compiling pcap filter code from: %s", filter_exp);
		if(pcap_compile(captureDevice, &pcap_filter, filter_exp, 1, netmask) == -1) {
			msg(MSG_FATAL, "unable to validate+compile pcap filter");
			goto out2;
		}

		if(pcap_setfilter(captureDevice, &pcap_filter) == -1) {
			msg(MSG_FATAL, "unable to attach filter to pcap: %s", pcap_geterr(captureDevice));
			goto out3;
		}
		/* you may free an attached code, see man-page */
		pcap_freecode(&pcap_filter);
	} else {
		msg(MSG_DEBUG, "using no pcap filter");
	}

	ready=true;

	return true;

out3:
	pcap_freecode(&pcap_filter);
out2:
	pcap_close(captureDevice);
	captureDevice=NULL;
out1:
	pcap_freealldevs(allDevices);
	allDevices=NULL;
out:
	return false;
}


/*
 this function is called by the logger timer thread and should dump
 some nice info using msg_stat
 */
void Observer::doLogging(void *arg)
{
	Observer *obs=(Observer *)arg;
	struct pcap_stat stats;

	 //pcap_stats() will set the stats to -1 if something goes wrong
	 //so it is okay if we dont check the return code
	obs->getPcapStats(&stats);
	msg_stat("%6d recv, %6d drop, %6d ifdrop", stats.ps_recv, stats.ps_drop, stats.ps_ifdrop);
}

/*
   call to get the main capture thread running
   open() has to be called before
*/
void Observer::performStart()
{
	if(!ready)
		THROWEXCEPTION("Can't start capturing, observer is not ready");

	msg(MSG_DEBUG, "now starting capturing thread");
	thread.run(this);
}

void Observer::performShutdown()
{
	/* be sure the thread is ending */
	msg(MSG_DEBUG, "joining the ObserverThread, may take a while (until next pcap data is received)");
	connected.shutdown();
	thread.join();
	msg(MSG_DEBUG, "ObserverThread joined");
}


/* you cannot change the caplen of an already running observer */
bool Observer::setCaptureLen(int x)
{
	msg(MSG_DEBUG, "Observer: setting capture length to %d bytes", x);
	/* we cant change pcap caplen if alredy pcap_open() called */
	if(ready) {
		THROWEXCEPTION("changing capture len on-the-fly is not supported by pcap");
	}
	if (x>PCAP_MAX_CAPTURE_LENGTH) {
		THROWEXCEPTION("maximum capture length is limited by constant PCAP_MAX_CAPTURE_LENGTH (%d), "
				"given value %d is too big", PCAP_MAX_CAPTURE_LENGTH, x);
	}
	capturelen=x;
	return true;
}

int Observer::getCaptureLen()
{
	return capturelen;
}

void Observer::replaceOfflineTimestamps()
{
	replaceTimestampsFromFile = true;
}

void Observer::setOfflineSpeed(float m)
{
	if(m == 1.0)
		return;

	stretchTime = 1/m;
	if(m < 1.0) {
		// speed decrease, try integer conversion
		stretchTimeInt = (uint16_t)(1/m);
		// allow only 10% inaccuracy, i.e.
		// (1/m - stretchTimeInt)/(1/m) = 1-stretchTimeInt*m < 0.1
		if((1 - stretchTimeInt * m) > 0.1)
			stretchTimeInt = 0; // use float
		else
		    msg(MSG_INFO, "Observer: speed multiplier set to %f in order to allow integer multiplication.", 1.0/stretchTimeInt);
	}
	else
		stretchTimeInt = 0;

}

void Observer::setOfflineAutoExit(bool autoexit)
{
	autoExit = autoexit;
}

bool Observer::setPacketTimeout(int ms)
{
	if(ready) {
		msg(MSG_ERROR, "changing read timeout on-the-fly is not supported by pcap");
		return false;
	}
	pcap_timeout=ms;
	return true;
}


int Observer::getPacketTimeout()
{
	return pcap_timeout;
}

/*
   get some capturing statistics
   struct pcap_stat is defined in pcap.h and has at least 3 u_int variables:
   ps_recv, ps_drop, ps_ifdrop

   should return: -1 on failure, 0 on OK
   */
int Observer::getPcapStats(struct pcap_stat *out)
{
	return(pcap_stats(captureDevice, out));
}

/**
 * statistics function called by StatisticsManager
 */
std::string Observer::getStatisticsXML(double interval)
{
	ostringstream oss;
	pcap_stat pstats;
	if (captureDevice && pcap_stats(captureDevice, &pstats)==0) {
		unsigned int recv = pstats.ps_recv;
		unsigned int dropped = pstats.ps_drop;

		oss << "<pcap>";
		oss << "<received type=\"packets\">" << (uint32_t)((double)(recv-statTotalRecvPackets)/interval) << "</received>";
		oss << "<dropped type=\"packets\">" << (uint32_t)((double)(dropped-statTotalLostPackets)/interval) << "</dropped>";
		oss << "<totalReceived type=\"packets\">" << statTotalRecvPackets << "</totalReceived>";
		oss << "<totalDropped type=\"packets\">" << statTotalLostPackets << "</totalDropped>";
		oss << "</pcap>";
		statTotalLostPackets = dropped;
		statTotalRecvPackets = recv;
	}
	uint64_t diff = receivedBytes-lastReceivedBytes;
	lastReceivedBytes += diff;
	oss << "<observer>";
	oss << "<processed type=\"bytes\">" << (uint32_t)((double)diff/interval) << "</processed>";
	diff = processedPackets-lastProcessedPackets;
	lastProcessedPackets += diff;
	oss << "<processed type=\"packets\">" << (uint32_t)((double)diff/interval) << "</processed>";
	oss << "<totalProcessed type=\"packets\">" << processedPackets << "</totalProcessed>";
	oss << "</observer>";
	return oss.str();
}
