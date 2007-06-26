/*
 * PSAMP Reference Implementation
 *
 * Filter.h
 *
 * Main filtering loop. Accepts a packet and applies filters on it
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

/*
 changed by: Ronny T. Lampert, 2005, for VERMONT
 */
#ifndef FILTER_H
#define FILTER_H


#include "Packet.h"
#include "PacketReceiver.h"
#include "PacketProcessor.h"
#include "SystematicSampler.h"
#include "RandomSampler.h"

#include "common/msg.h"
#include "common/Thread.h"
#include "common/ConcurrentQueue.h"

#include <vector>


class Filter : public PacketReceiver
{
public:
        Filter() : thread(Filter::filterProcess), hasReceiver_(false), exitFlag(false),  pktIn(0), pktOut(0)
        {
#if defined(DEBUG)
	    queue->debugSetOwner("Filter");
#endif
        }

        ~Filter()
        {
        }

        void startFilter()
        {
                msg(MSG_DEBUG, "now starting Filter thread");
                thread.run(this);
        }

        void terminate()
        {
                exitFlag = true;
        }

        // adds a new output queue to the receivers
        void setReceiver(PacketReceiver *recv)
        {
		hasReceiver_ = true;
                receiver = recv->getQueue();
        };

	bool hasReceiver() { return hasReceiver_; }

        // add a new filter or sampler
        void addProcessor(PacketProcessor *p)
        {
                processors.push_back(p);
        };

        /* in case we have to do some things with 'em */
        std::vector<PacketProcessor *> getProcessors()
        {
                return processors;
        }

protected:
        // pthread object
        Thread thread;
        static void *filterProcess(void *);

        // vector of all sub-filters, called PacketProcessor
        std::vector<PacketProcessor *> processors;

        // the next in line that gets our output
        ConcurrentQueue<Packet*> *receiver;
	bool hasReceiver_;

public:
        bool exitFlag;

        // statistics of packets in and the packets passing the filter routines
        unsigned long pktIn;
        unsigned long pktOut;

};

#endif
