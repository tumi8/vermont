/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2006-2013 Vermont Project
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

#ifndef IPFIXDBREADER_H
#define IPFIXDBREADER_H

#include "common/ipfixlolib/ipfix.h"
#include "../IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include "core/Module.h"

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <boost/smart_ptr.hpp>

/**
 *      IpfixDbReader powered the communication to the database server
 *      also between the other structs
 */
class IpfixDbReader : public Module, public Source<IpfixRecord*>, public Destination<NullEmitable*> 
{
	public:
		IpfixDbReader(const string& dbType, const string& hostname, const string& dbname,
				const string& username, const string& password,
				uint16_t port, uint16_t observationDomainId);
		~IpfixDbReader();

		virtual void performStart();
		virtual void performShutdown();

		boost::shared_ptr<IpfixRecord::SourceID> srcId;

	protected:
		void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, InformationElement::IeInfo type);
		vector<string> tables;
		vector<struct ipfix_identifier> columns;
		string columnNames; 
		string orderBy; 
		unsigned recordLength;

		std::string hostname;
		std::string dbname;
		std::string username;
		std::string password;
		uint16_t port;
		uint16_t observationDomainId;

		Thread thread;
		
		static InstanceManager<IpfixTemplateRecord> templateRecordIM;
		static InstanceManager<IpfixDataRecord> dataRecordIM;
		static InstanceManager<IpfixTemplateDestructionRecord> templateDestructionRecordIM;

		static void* readFromDB(void* ipfixDbReader_);
		int dbReaderSendNewTemplate(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName);
		int dbReaderDestroyTemplate(boost::shared_ptr<TemplateInfo> templateInfo);

		virtual int connectToDb() = 0;
		virtual int dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName) = 0;
		virtual int getColumns(const string& tableName) = 0 ;
		virtual int getTables() = 0;
};

        

#endif


#endif
