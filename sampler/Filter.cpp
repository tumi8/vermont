/*
 * PSAMP Reference Implementation
 *
 * Filter.cpp
 *
 * Main filtering loop. Accepts a packet and applies filters on it
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

/*
 changed by: Ronny T. Lampert, 2005, for VERMONT
 */
#include "common/msg.h"
#include "Filter.h"

#include <sstream>

using namespace std;

Filter::Filter() 
	: PacketReceiver("Filter"), thread(Filter::filterProcess), 
	hasReceiver_(false), processedPackets(0), lastProcessedPackets(0), exitFlag(false)
{
}

Filter::~Filter()
{
}

void Filter::startFilter()
{
	msg(MSG_DEBUG, "now starting Filter thread");
	thread.run(this);
}

void Filter::terminate()
{
	exitFlag = true;
}

/**
 * adds a new output queue to the receivers
 */
void Filter::setReceiver(PacketReceiver *recv)
{
	hasReceiver_ = true;

	//FIXME: ugly hack to demonstrate the reconf work
	receiver = NULL;
	Destination<Packet*>* dest = dynamic_cast< Destination<Packet*>* >(recv);
	if (dest != NULL) {
		connectTo(dest);
		return;
	}
	
	msg(MSG_ERROR, "Filter: cant set PacketDestination\n");
}

bool Filter::hasReceiver()
{ 
	return hasReceiver_; 
}

/**
 * add a new filter or sampler
 */
void Filter::addProcessor(PacketProcessor *p)
{
	processors.push_back(p);
}

std::vector<PacketProcessor *> Filter::getProcessors()
{
	return processors;
}

/*
 this is the main filter process, running within a thread
 each packet is run thru a series of PacketProcessors and then pushed
 to the next system in line (represented thru a ConcurrentQueue)
 */
void *Filter::filterProcess(void *arg)
{
	Packet *p;
	vector<PacketProcessor *>::iterator it;

	// get back the object from the arg
	Filter *filter = (Filter *)arg;

	/*
	 set to true so packet is always pushed to receiver
	 even if the pp-iterator below is NULL
	 */
	bool keepPacket=true;


	/* for dumb compilers, do CSE here to spare some cycles below */
	ConcurrentQueue<Packet*> *in_q=filter->getQueue();
	

	msg(MSG_INFO, "now running the filter thread");
	while(!filter->exitFlag) {

		//DPRINTF("Count is %d\n", in_q->getCount());

		// get a packet
		DPRINTFL(MSG_VDEBUG, "trying to get packet");
		if (!in_q->pop(&p)) break;

		DPRINTFL(MSG_VDEBUG, "got packet");
		filter->processedPackets++;

		// run packet through all packetProcessors
		for(it = filter->processors.begin(); it != filter->processors.end(); ++it) {
			keepPacket = (*it)->processPacket(p);

			if(!keepPacket) {
				break;
			}
		}

		// check if we passed all filters
		if(keepPacket) {
			// push packet to the receiver
			DPRINTF("pushing packet %d", p);
			
			filter->dest->receive(p);
		} else {
			// immediately drop the packet
			DPRINTF("releasing packet");
			p->removeReference();
		}
	}
	
	
	return 0;
};


/**
 * statistics function called by StatisticsManager
 */
std::string Filter::getStatistics()
{
	ostringstream oss;
	uint64_t diff = processedPackets-lastProcessedPackets;
	lastProcessedPackets += diff;
	oss << "Filter: processed packets: " << diff << endl;
	return PacketReceiver::getStatistics() + "\n" +  oss.str();
}
