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
#include "msg.h"
#include "Filter.h"

using namespace std;

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
	ConcurrentQueue<Packet> *in_q=filter->getQueue();
	ConcurrentQueue<Packet> *out_q=filter->receiver;

	msg(MSG_INFO, "Filter: now running the filter thread");
	while(!filter->exitFlag) {

		// get a packet
		p = in_q->pop();
		filter->pktIn++;

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
			out_q->push(p);
			filter->pktOut++;
		} else {
			// immediately drop the packet
			//DPRINTF("Filter: Drop-Freeing packet here");
			p->release();
		}
	}
	return 0;
};

