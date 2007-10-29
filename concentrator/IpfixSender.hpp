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
#include "ipfixlolib/ipfixlolib.h"
#include "common/ConcurrentQueue.h"
#include <queue>


using namespace std;

/**
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib" 
 */
class IpfixSender : public Module, public Source<NullEmitable*>, public IpfixRecordDestination, public StatisticsModule
{
public:
	IpfixSender(uint16_t observationDomainId, const char* ip = 0, uint16_t port = 0); // FIXME: observationDomainId
	virtual ~IpfixSender();

	void addCollector(const char *ip, uint16_t port);
	void flushPacket();
	
	// inherited from Module
	virtual void performStart();
	virtual void performShutdown();

	// inherited from IpfixRecordDestination
	virtual void onDataTemplate(IpfixDataTemplateRecord* record);
	virtual void onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record);
	virtual void onDataDataRecord(IpfixDataDataRecord* record);

	// inherited from StatisticsModule
	virtual std::string getStatistics();

	class Collector
	{
	public:
		Collector() :
			port(0)
		{
			memset(&ip, 0, sizeof(ip));
		}
		~Collector() {}

		char ip[128]; /**< IP address of Collector */
		uint16_t port; /**< Port of Collector */
	};

protected:
	ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. */
	uint16_t lastTemplateId; /**< Template ID of last created Template */
	std::vector<Collector> collectors; /**< Collectors we export to */
	uint32_t statSentRecords; /**< Statistics: Total number of records sent since last statistics were polled */

	virtual void preReconfiguration2();

private:
	/**
	 * specifies, how long incoming flows are to be cached at most before they are
	 * sent in an IPFIX packet
	 * this value specifies the maximum latency in milliseconds
	 */
	uint32_t maxFlowLatency;

	uint16_t ringbufferPos; /**< Pointer to next free slot in @c conversionRingbuffer. */
	uint8_t conversionRingbuffer[65536]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */
	uint16_t currentTemplateId; /**< Template ID of the unfinished data set */
	uint16_t noCachedRecords; /**< number of records already passed to ipfixlob, should be equal to recordsToRelease.size() */
	list<boost::shared_ptr<IpfixRecord::TemplateInfo> > registeredTemplates; /**< contains all templates which were already registered in ipfixlolib */
	Thread thread;
	ConcurrentQueue<IpfixRecord*> incomingRecords;
	queue<IpfixRecord*> recordsToRelease;
	
	static void* threadWrapper(void* instance);
	void processLoop();
	void removeRecordReferences();
	void endAndSendDataSet();

	void startDataSet(uint16_t templateId);
	bool isTemplateRegistered(IpfixRecord::TemplateInfo* ti);
	void removeRegisteredTemplate(IpfixRecord::TemplateInfo* ti);
	void addRegisteredTemplate(boost::shared_ptr<IpfixRecord::TemplateInfo> ti);
};

#endif
