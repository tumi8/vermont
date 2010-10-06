#ifndef IP_PACKET_SELECTOR_HPP
#define IP_PACKET_SELECTOR_HPP

#include "BasePacketSelector.hpp"
#include <map>
#include <stdint.h>
#include <arpa/inet.h>



class Packet;

class IpPacketSelector : public BasePacketSelector 
{
	public:
		~IpPacketSelector();
		IpPacketSelector();
		void initializeConfig(std::map<uint32_t, int>&, std::map<uint32_t, int>&);
		virtual int decide(Packet *p);
		void addDestinationIp(uint32_t dst, int queueno);
		void addSourceIp(uint32_t src, int queueno);
		
	private:
		std::map<uint32_t, int> dstips;
		std::map<uint32_t, int> srcips;
}
;

#endif
