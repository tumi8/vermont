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

#ifndef INCLUDED_IpfixPacketProcessor_hpp
#define INCLUDED_IpfixPacketProcessor_hpp

#include "IpfixRecord.hpp"

#include <list>
#include <pthread.h>
#include <stdint.h>
#include <boost/smart_ptr.hpp>



class IpfixParser;
        
/**
 * Controls parsing of incoming packets.
 * Create witch @c createPacketProcessor()
 */
class IpfixPacketProcessor 
{
	public:
		virtual ~IpfixPacketProcessor() {};
		virtual int processPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) = 0; /**< process (e.g. parse and enqueue) the given raw network packet */
		
		virtual void performStart() {};
		virtual void performShutdown() {};
		virtual void onReconfiguration1() {};
		virtual void onReconfiguration2() {};
		virtual void postReconfiguration() {};

};

#endif
