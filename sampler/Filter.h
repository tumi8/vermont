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

#include "reconf/Source.h"

#include <vector>


class Filter : public PacketReceiver, Source<Packet*>
{
	public:

		Filter();
        virtual ~Filter();

        void startFilter();
        void terminate();
		void setReceiver(PacketReceiver *recv);
		bool hasReceiver();
        void addProcessor(PacketProcessor *p);
        std::vector<PacketProcessor *> getProcessors();
		virtual std::string getStatistics();

protected:
		// pthread object
		Thread thread;
		static void *filterProcess(void *);

		// vector of all sub-filters, called PacketProcessor
		std::vector<PacketProcessor *> processors;

		// the next in line that gets our output
		ConcurrentQueue<Packet*> *receiver;
		bool hasReceiver_;

		// statistics
		// ATTENTION: value may sometimes be incorrect caused by multithreading issues
		volatile uint64_t processedPackets;
		volatile uint64_t lastProcessedPackets;

public:
        bool exitFlag;
};



#endif
