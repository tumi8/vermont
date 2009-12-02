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

#include "common/msg.h"
#include "IpfixRecord.hpp"


namespace InformationElement {

	/**
	 * Gets standard length for a standard information element id
	 * TODO: tidy up, this function is equivalent to ipfix_names.c:ipfix_id_lookup(int)
	 * @param id Standard Information Element ID (registered by IANA)
	 * @returns Standard length of the Information Element (65535 = variable length by default)
	 */
	IeLength getFieldLength(IeId id) {
		switch (id) {
			case IPFIX_TYPEID_protocolIdentifier:
			case IPFIX_TYPEID_tcpControlBits:
			case IPFIX_ETYPEID_revTcpControlBits:
				return 1;

			case IPFIX_TYPEID_icmpTypeCode:
			case IPFIX_TYPEID_sourceTransportPort:
			case IPFIX_TYPEID_destinationTransportPort:
				return 2;

			case IPFIX_TYPEID_flowStartSeconds:
			case IPFIX_TYPEID_flowEndSeconds:
			case IPFIX_TYPEID_sourceIPv4Address:
			case IPFIX_TYPEID_destinationIPv4Address:
			case IPFIX_ETYPEID_maxPacketGap:
			case IPFIX_ETYPEID_revFlowStartSeconds:
			case IPFIX_ETYPEID_revFlowEndSeconds:
			case IPFIX_ETYPEID_revMaxPacketGap:
			case IPFIX_ETYPEID_frontPayloadPktCount:
				return 4;

			case IPFIX_TYPEID_flowStartMilliSeconds:
			case IPFIX_TYPEID_flowEndMilliSeconds:
			case IPFIX_TYPEID_octetDeltaCount:
			case IPFIX_TYPEID_packetDeltaCount:
			case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			case IPFIX_ETYPEID_revFlowEndMilliSeconds:
			case IPFIX_ETYPEID_revFlowStartNanoSeconds:
			case IPFIX_ETYPEID_revFlowEndNanoSeconds:
			case IPFIX_ETYPEID_revOctetDeltaCount:
			case IPFIX_ETYPEID_revPacketDeltaCount:
				return 8;
		}
		THROWEXCEPTION("IpfixRecord: unknown typeid, failed to determine field length");
		return 0;
	}

	/**
	 * Checks whether the given Field carries reverse flow information in a bi-flow record
	 * TODO: According to the standard, reversef low IEs should use enterprise number 29305
	 * @param field Data Record field
	 * @returns true if this IE carries reverse flow information
	 */
	bool isBiflowField(const IeInfo& type)
	{
		// TODO: Bi-flow standard: if(field.type ==29305) return true;
		switch (type.id) {
			case IPFIX_ETYPEID_revTcpControlBits:
			case IPFIX_ETYPEID_revFlowStartSeconds:
			case IPFIX_ETYPEID_revFlowEndSeconds:
			case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			case IPFIX_ETYPEID_revFlowEndMilliSeconds:
			case IPFIX_ETYPEID_revFlowStartNanoSeconds:
			case IPFIX_ETYPEID_revFlowEndNanoSeconds:
			case IPFIX_ETYPEID_revOctetDeltaCount:
			case IPFIX_ETYPEID_revPacketDeltaCount:
			case IPFIX_ETYPEID_revMaxPacketGap:
				return true;
		}

		return false;
	}


	/**
	 * Returns valid protocols for given field
	 * @param field Data Record field
	 * @param p pointer to raw packet
	 * @returns offset (in bytes) at which the data for the given field is located in the raw packet
	 */
	const Packet::IPProtocolType getValidProtocols(const IeInfo& type)
	{
		if(type.enterprise == 0) {
			switch (type.id) {
				case IPFIX_TYPEID_packetDeltaCount:
				case IPFIX_TYPEID_flowStartSeconds:
				case IPFIX_TYPEID_flowEndSeconds:
				case IPFIX_TYPEID_flowStartMilliSeconds:
				case IPFIX_TYPEID_flowEndMilliSeconds:
				case IPFIX_TYPEID_flowStartNanoSeconds:
				case IPFIX_TYPEID_flowEndNanoSeconds:
				case IPFIX_TYPEID_octetDeltaCount:
				case IPFIX_TYPEID_protocolIdentifier:
				case IPFIX_TYPEID_sourceIPv4Address:
				case IPFIX_TYPEID_destinationIPv4Address:
				case IPFIX_ETYPEID_revPacketDeltaCount:
				case IPFIX_ETYPEID_revFlowStartSeconds:
				case IPFIX_ETYPEID_revFlowEndSeconds:
				case IPFIX_ETYPEID_revFlowStartMilliSeconds:
				case IPFIX_ETYPEID_revFlowEndMilliSeconds:
				case IPFIX_ETYPEID_revFlowStartNanoSeconds:
				case IPFIX_ETYPEID_revFlowEndNanoSeconds:
				case IPFIX_ETYPEID_revOctetDeltaCount:
				case IPFIX_ETYPEID_maxPacketGap:
				case IPFIX_ETYPEID_revMaxPacketGap:
					return Packet::IPProtocolType(Packet::TCP|Packet::UDP|Packet::ICMP);

				case IPFIX_TYPEID_icmpTypeCode:
					return Packet::ICMP;

				case IPFIX_TYPEID_sourceTransportPort:
				case IPFIX_TYPEID_destinationTransportPort:
				case IPFIX_ETYPEID_frontPayload:
				case IPFIX_ETYPEID_frontPayloadLen:
				case IPFIX_ETYPEID_revFrontPayload:
				case IPFIX_ETYPEID_revFrontPayloadLen:
				case IPFIX_ETYPEID_frontPayloadPktCount:
					return Packet::IPProtocolType(Packet::UDP|Packet::TCP);

				case IPFIX_TYPEID_tcpControlBits:
				case IPFIX_ETYPEID_revTcpControlBits:
					return Packet::TCP;
			}
		}
		THROWEXCEPTION("received unknown field type id (%d)", type.id);
		return Packet::NONE;
	}

}



IpfixRecord::IpfixRecord()
{
}

IpfixRecord::~IpfixRecord() {
}

IpfixOptionsTemplateRecord::IpfixOptionsTemplateRecord(InstanceManager<IpfixOptionsTemplateRecord>* im)
	: ManagedInstance<IpfixOptionsTemplateRecord>(im)
{
}

IpfixTemplateRecord::IpfixTemplateRecord(InstanceManager<IpfixTemplateRecord>* im)
	: ManagedInstance<IpfixTemplateRecord>(im)
{
}

IpfixDataTemplateRecord::IpfixDataTemplateRecord(InstanceManager<IpfixDataTemplateRecord>* im)
	: ManagedInstance<IpfixDataTemplateRecord>(im)
{
}

IpfixDataRecord::IpfixDataRecord(InstanceManager<IpfixDataRecord>* im)
	: ManagedInstance<IpfixDataRecord>(im)
{
}

IpfixOptionsRecord::IpfixOptionsRecord(InstanceManager<IpfixOptionsRecord>* im)
	: ManagedInstance<IpfixOptionsRecord>(im)
{
}

IpfixDataDataRecord::IpfixDataDataRecord(InstanceManager<IpfixDataDataRecord>* im)
	: ManagedInstance<IpfixDataDataRecord>(im)
{
}

IpfixTemplateDestructionRecord::IpfixTemplateDestructionRecord(InstanceManager<IpfixTemplateDestructionRecord>* im)
	: ManagedInstance<IpfixTemplateDestructionRecord>(im)
{
}

IpfixOptionsTemplateDestructionRecord::IpfixOptionsTemplateDestructionRecord(InstanceManager<IpfixOptionsTemplateDestructionRecord>* im)
	: ManagedInstance<IpfixOptionsTemplateDestructionRecord>(im)
{
}

IpfixDataTemplateDestructionRecord::IpfixDataTemplateDestructionRecord(InstanceManager<IpfixDataTemplateDestructionRecord>* im)
	: ManagedInstance<IpfixDataTemplateDestructionRecord>(im)
{
}

