/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#ifndef INCLUDED_IpfixCollector_hpp
#define INCLUDED_IpfixCollector_hpp

#include <list>
#include <pthread.h>
#include <stdint.h>
#include "FlowSink.hpp"
#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

/**
 * Represents a collector
 */
class IpfixCollector {
	public:
		IpfixCollector();
		~IpfixCollector();

		int start();
		int stop();

		void addIpfixPacketProcessor(IpfixPacketProcessor* packetProcessor);
		void addIpfixReceiver(IpfixReceiver* ipfixReceiver);

	protected:
		std::list<IpfixReceiver*> ipfixReceivers;
		std::list<IpfixPacketProcessor*> packetProcessors;

};

#endif
