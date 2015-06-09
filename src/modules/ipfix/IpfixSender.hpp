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

#include "IpfixRecord.hpp"
#include "IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ConcurrentQueue.h"
#include "common/Mutex.h"
#include "core/Notifiable.h"
#include <queue>
#include <map>



using namespace std;

/**
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib" 
 */
class IpfixSender : public Module, public Source<NullEmitable*>, public IpfixRecordDestination, public Notifiable
{
public:
	IpfixSender(uint32_t observationDomainId, uint32_t maxRecordRate,
			uint32_t sctpDataLifetime,
			uint32_t sctpReconnectInterval,
			uint32_t templateRefreshInterval,
			const std::string &certificateChainFile,
			const std::string &privateKeyFile,
			const std::string &caFile,
		        const std::string &caPath,
		        export_protocol_version export_protocol = IPFIX_PROTOCOL);
	IpfixSender(uint32_t observationDomainId, uint32_t maxRecordRate = IS_DEFAULT_MAXRECORDRATE);
	virtual ~IpfixSender();

	void addCollector(const char *ip, uint16_t port,
			ipfix_transport_protocol proto, void *aux_config);
	void flushPacket();

	virtual void notifyQueueRunning();

	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);

	virtual void onReconfiguration1();
	virtual void onReconfiguration2();
	
	// inherited from Notifiable
	virtual void onTimeout(void* dataPtr);


	virtual string getStatisticsXML(double interval);
	
protected:
	ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. */


private:
	enum SendPolicy {
		IfNotEmpty,
		Always
	};
	void performShutdown(); 
	static void* threadWrapper(void* instance);
	void processLoop();
	void startDataSet(uint16_t templateId, uint16_t dataLength);
	void setTemplateId(TemplateInfo::TemplateId templateId,
			uint16_t dataLength);
	void endDataSet();
	void send();
	void sendRecords(SendPolicy policy);
	void removeRecordReferences();
	void registerTimeout();
	void onSendRecordsTimeout(void);
	void onBeatTimeout(void);
	void registerBeatTimeout();

	TemplateInfo::TemplateId getUnusedTemplateId();

	// Set up time after that Templates are going to be resent
	bool setTemplateTransmissionTimer(uint32_t timer){
		ipfix_set_template_transmission_timer(ipfixExporter, timer);
		
		return true;
	}

	uint32_t statSentDataRecords; /**< Statistics: Total number of data records sent since last statistics were polled */
	uint32_t statSentPackets; /**< Statistics: total number of packets sent over the network */
	uint32_t statPacketsInFlows; /**< Statistics: total number of packets within flows */
		
	uint32_t sentRecords; /**< Statistics: Total number of records sent since last statistics were polled */

	timespec nextTimeout;

	Mutex ipfixMessageLock;

	// send after timeout parameters
	queue<IpfixRecord*> recordsToRelease;
	uint16_t noCachedRecords; /**< number of records already passed to ipfixlob, should be equal to recordsToRelease.size() */
	uint16_t noRecordsInCurrentSet; /**< Number of records in current data set. */
	uint16_t recordCacheTimeout; /**< how long may records be cached until sent, milliseconds */
	bool timeoutRegistered; /**< true if next timeout was already registered in timer */
	uint16_t currentTemplateId; /**< Template ID of the unfinished data set */
	uint16_t remainingSpace; /**< Remaining space in current IPFIX message measured in bytes. */

	uint16_t ringbufferPos; /**< Pointer to next free slot in @c conversionRingbuffer. */
	uint8_t conversionRingbuffer[65536]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */

	// rate limiting paramemters 
	struct timeval curTimeStep; /**< current time used for determining packet rate */
	uint32_t recordsSentStep; /**< number of records sent in timestep (usually 100ms)*/
	uint32_t maxRecordRate;  /** maximum number of records per seconds to be sent over the wire */

	int timeoutSendRecords; /**< Dummy variable. Used as a pointer destination to distinguish between two dirrent types of timeout */
	int timeoutIpfixlolibBeat; /**< Dummy variable. Used as a pointer destination to distinguish between two dirrent types of timeout */

	// mapping of uniqueId to templateId and vice versa
	std::map<TemplateInfo::TemplateId, uint16_t> templateIdToUniqueId; /**< stores uniqueId for a give Template ID */
	std::map<uint16_t, TemplateInfo::TemplateId> uniqueIdToTemplateId; /**< stores Template ID for a give unique ID */
	export_protocol_version export_protocol; // Version of Flow record, V9 or IPFIX

};

#endif
