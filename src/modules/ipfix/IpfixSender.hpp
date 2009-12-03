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

#ifndef SNDIPFIX_H
#define SNDIPFIX_H

#include "IpfixParser.hpp"
#include "IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ConcurrentQueue.h"
#include "core/Notifiable.h"
#include <queue>


using namespace std;

/**
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib" 
 */
class IpfixSender : public Module, public Source<NullEmitable*>, public IpfixRecordDestination, public Notifiable
{
public:
	IpfixSender(uint32_t observationDomainId, uint32_t maxRecordRate, uint32_t sctpDataLifetime, uint32_t sctpReconnectInterval,
			uint32_t templateRefreshInterval, uint32_t templateRefreshRate);
	IpfixSender(uint32_t observationDomainId);
	virtual ~IpfixSender();

	void addCollector(const char *ip, uint16_t port, ipfix_transport_protocol proto);
	void flushPacket();

	// inherited from IpfixRecordDestination
	virtual void onDataRecord(IpfixDataRecord* record) {
		// FIXME: not sure how to deal with this
		msg(MSG_DEBUG, "%s:%u Ignoring Template", __FILE__, __LINE__);
	}
	
	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);

	virtual void onReconfiguration1();
	
	// inherited from Notifiable
	virtual void onTimeout(void* dataPtr);


	virtual void onReconfiguration2();
	virtual string getStatisticsXML(double interval);
	
protected:
	ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. */
	uint16_t lastTemplateId; /**< Template ID of last created Template */
	uint32_t statSentDataRecords; /**< Statistics: Total number of data records sent since last statistics were polled */
	uint32_t statSentPackets; /**< Statistics: total number of packets sent over the network */
	uint32_t statPacketsInFlows; /**< Statistics: total number of packets within flows */
		
	void performShutdown(); 
	uint32_t sentRecords; /**< Statistics: Total number of records sent since last statistics were polled */

	timespec nextTimeout;

	queue<IpfixRecord*> recordsToRelease;

	static void* threadWrapper(void* instance);
	void processLoop();
	void removeRecordReferences();
	void endAndSendDataSet();

	void startDataSet(uint16_t templateId);
	bool isTemplateRegistered(TemplateInfo* ti);
	void removeRegisteredTemplate(TemplateInfo* ti);
	void addRegisteredTemplate(boost::shared_ptr<TemplateInfo> ti);
	void sendRecords(bool forcesend = false);
	void registerTimeout();


private:

	uint16_t ringbufferPos; /**< Pointer to next free slot in @c conversionRingbuffer. */
	uint8_t conversionRingbuffer[65536]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */
	uint16_t currentTemplateId; /**< Template ID of the unfinished data set */
	uint16_t noCachedRecords; /**< number of records already passed to ipfixlob, should be equal to recordsToRelease.size() */
	list<boost::shared_ptr<TemplateInfo> > registeredTemplates; /**< contains all templates which were already registered in ipfixlolib */
	
	uint16_t recordCacheTimeout; /**< how long may records be cached until sent, milliseconds */
	bool timeoutRegistered; /**< true if next timeout was already registered in timer */
	bool recordsAlreadySent; /**< true if records were sent to the network as the packet was full */
	struct timeval curTimeStep; /**< current time used for determining packet rate */
	uint32_t recordsSentStep; /**< number of records sent in timestep (usually 100ms)*/
	uint32_t maxRecordRate;  /** maximum number of records per seconds to be sent over the wire */

	// Set up time after that Templates are going to be resent
	bool setTemplateTransmissionTimer(uint32_t timer){
		ipfix_set_template_transmission_timer(ipfixExporter, timer);
		
		return true;
	}

};

#endif
