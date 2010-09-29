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

#include "IpfixPacketProcessor.hpp"
#include "IpfixRecordSender.h"
#include "core/Module.h"
#include "IpfixReceiver.hpp"

#include <stdint.h>


/**
 * Represents a collector module
 * it always contains an IpfixReceiver (passed by constructor)
 * and one IpfixPacketProcessor, receiver and packet processor are
 * hard wired
 */
class IpfixCollector 
	: public Module, public Source<IpfixRecord*>, public Destination<NullEmitable*>, public IpfixRecordSender 
{
	public:
		IpfixCollector(IpfixReceiver* receiver);
		virtual ~IpfixCollector();

		virtual void performStart();
		virtual void performShutdown();

		virtual void postReconfigration();
		virtual void onReconfiguration1();
		virtual void onReconfiguration2();

		bool send(IpfixRecord* ipfixRecord);
		
		virtual string getStatisticsXML(double interval);

		void setTemplateLifetime(uint16_t time);

	private:
		IpfixReceiver* ipfixReceiver;
		IpfixPacketProcessor* ipfixPacketProcessor;
		uint64_t statSentRecords;

};

#endif
