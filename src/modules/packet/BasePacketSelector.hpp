#ifndef BASE_PACKET_SELECTOR_HPP
#define BASE_PACKET_SELECTOR_HPP

#include "modules/packet/Packet.h"

#define SRC_ADDRESS_OFFSET 12
#define DST_ADDRESS_OFFSET 16
#define TCP_UDP_SRC_PORT_OFFSET 0
#define TCP_UDP_DST_PORT_OFFSET 4

class Packet;

class BasePacketSelector {
	public:
		BasePacketSelector();
		~BasePacketSelector();
		virtual int decide(Packet *p) = 0;
		void setNumberOfQueues(int n);
		
	protected:
		int numberOfQueues;	

};

#endif
