#ifndef HASH_PACKET_SELECTOR_HPP
#define HASH_PACKET_SELECTOR_HPP

#include "BasePacketSelector.hpp"
#include "Packet.h"
#include <map>
#include <stdint.h>

class Packet;

class HashPacketSelector : public BasePacketSelector 
{
	public:
		~HashPacketSelector();
		HashPacketSelector();
		virtual int decide(Packet *p);
		void addDestinationIp(uint32_t dst, int queueno);
		void addSourceIp(uint32_t src, int queueno);
		
	private:
		std::map<uint32_t, int> dstips;
		std::map<uint32_t, int> srcips;

};

#endif
