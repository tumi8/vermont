/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2008 Gerhard Muenz
 * Copyright (C) 2009-2013 Vermont Project
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

#if defined(DB_SUPPORT_ENABLED) || defined(MONGO_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED) || defined(REDIS_SUPPORT_ENABLED)

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include "IpfixDbReader.hpp"
#include "IpfixDbCommon.hpp"
#include "common/msg.h"


InstanceManager<IpfixTemplateRecord> IpfixDbReader::templateRecordIM("DbReaderIpfixTemplateRecord", 1);
InstanceManager<IpfixDataRecord> IpfixDbReader::dataRecordIM("DbReaderIpfixDataRecord", 50);
InstanceManager<IpfixTemplateDestructionRecord> IpfixDbReader::templateDestructionRecordIM("DbReaderIpfixTemplateDestructionRecord", 1);

/***** Internal Functions ****************************************************/


/**
 * First send a a new template, then send the dataTemplates for all stored
 * tables.
 */
void* IpfixDbReader::readFromDB(void* ipfixDbReader_)
{
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)ipfixDbReader_;

	ipfixDbReader->registerCurrentThread();

	if (ipfixDbReader->connectToDb()) {
		THROWEXCEPTION("IpfixDbReader creation failed");
	}

	/** get tables of the database*/
	if(ipfixDbReader->getTables() != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Error in function getTables");
		THROWEXCEPTION("IpfixDbReader creation failed");
	}

	msg(MSG_DIALOG, "IpfixDbReader: Start sending tables");
	for(vector<string>::iterator i = ipfixDbReader->tables.begin(); i != ipfixDbReader->tables.end() && !ipfixDbReader->exitFlag; i++) {
		boost::shared_ptr<TemplateInfo> templateInfo(new TemplateInfo);
		templateInfo->setId = TemplateInfo::IpfixTemplate;
		if(ipfixDbReader->dbReaderSendNewTemplate(templateInfo, *i) != 0)
		{
		    msg(MSG_ERROR, "IpfixDbReader: Template error, skip table");
		    continue;
		}
		ipfixDbReader->dbReaderSendTable(templateInfo, *i);
		ipfixDbReader->dbReaderDestroyTemplate(templateInfo);

	}

	ipfixDbReader->unregisterCurrentThread();
	
	msg(MSG_DIALOG,"IpfixDbReader: Sending from database is done");
	return 0;
}
/**
 * Constructs a template from the table data and sends it to all connected
 * modules.
 */
int IpfixDbReader::dbReaderSendNewTemplate(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
{
	// reset record length 
	recordLength  = 0;

	/**get columnsname of the table*/
	if(getColumns(tableName) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Could not get columns for template");
		return 1;
	}

	for(vector<struct ipfix_identifier>::iterator i = columns.begin(); i != columns.end(); i++) {			
		templateInfo->fieldCount++;
		templateInfo->fieldInfo = (TemplateInfo::FieldInfo*)realloc(templateInfo->fieldInfo,
						      sizeof(TemplateInfo::FieldInfo)*templateInfo->fieldCount);
		TemplateInfo::FieldInfo* fi = &templateInfo->fieldInfo[templateInfo->fieldCount - 1];	
		fi->type.id = i->id;
		fi->type.length = i->length;
		fi->type.enterprise = i->pen;
		fi->offset = recordLength;
		recordLength = recordLength + fi->type.length; 
	}

	/* Pass Data Template to flowSinks */
	IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
	ipfixRecord->sourceID = srcId;
	ipfixRecord->templateInfo = templateInfo;
	send(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReader: sent template for table %s", tableName.c_str());
	return 0;
}


void IpfixDbReader::copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, InformationElement::IeInfo type) {
        switch (type.length) {
        case 1:
		*(uint8_t*)dest = *(uint8_t*)src;
                return;
        case 2:
		*(uint16_t*)dest = htons(*(uint16_t*)src);
                return;
        case 4:
		*(uint32_t*)dest = htonl(*(uint32_t*)src);
                return;
        case 8:
		*(uint64_t*)dest = htonll(*(uint64_t*)src);
                return;
        default:
                msg(MSG_ERROR, "IpfixDbReader: Uint with length %d unparseable", type.length);
                return;
        }
}


int IpfixDbReader::dbReaderDestroyTemplate(boost::shared_ptr<TemplateInfo> templateInfo)
{
	IpfixTemplateDestructionRecord* ipfixRecord = templateDestructionRecordIM.getNewInstance();
	ipfixRecord->sourceID = srcId;
	ipfixRecord->templateInfo = templateInfo;
	send(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReader: Template destroyed");

	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Starts or resumes database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
void IpfixDbReader::performStart() 
{
	thread.run(this);
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
void IpfixDbReader::performShutdown() 
{
	thread.join();
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
IpfixDbReader::~IpfixDbReader() {
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReader::IpfixDbReader(const string& dbType, const string& Hostname, const string& Dbname,
				const string& Username, const string& Password,
				uint16_t Port, uint16_t ObservationDomainId)
	: hostname(Hostname), dbname(Dbname), username(Username), password(Password), port(Port), observationDomainId(ObservationDomainId), thread(readFromDB, "IpfixDbReader")
{
	srcId.reset(new IpfixRecord::SourceID);
	srcId->observationDomainId = observationDomainId;
	srcId->exporterAddress.len = 0;
	srcId->exporterPort = 0;
	srcId->receiverPort = 0;
	srcId->protocol = 0;
	srcId->fileDescriptor = 0;
}

#endif
