/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
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

/* Some constants that are common to IpfixDbWriter and IpfixDbReader */
#ifdef DB_SUPPORT_ENABLED

#ifndef IPFIXDBREADER_H
#define IPFIXDBREADER_H

#include "IpfixDbCommon.hpp"
#include "IpfixParser.hpp"
#include "FlowSource.hpp"
#include "ipfix.hpp"
#include "ipfixlolib/ipfixlolib.h"

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <boost/smart_ptr.hpp>

#include <mysql.h>

/**
 *      IpfixDbReader powered the communication to the database server
 *      also between the other structs
 */
class IpfixDbReader : public FlowSource {
	public:
		IpfixDbReader(const char* hostname, const char* dbName,
				const char* username, const char* password,
				unsigned int port, uint16_t observationDomainId);
		~IpfixDbReader();

		int start();
		int stop();

		boost::shared_ptr<IpfixRecord::SourceID> srcId;

	protected:
		static const int MAX_TABLES = 10; /**< count of tables that will be send */
		static const int MAX_COL = 10; /**< max count of columns in the table */

		typedef struct {
			char* cname;       /**column name*/
			uint16_t ipfixId;  /**IPFIX_TYPEID*/
			uint8_t length;    /**IPFIX length*/
		} columnDB;

		typedef struct {
			char* tableNames[MAX_TABLES];
			int tableCount;
			IpfixDbReader::columnDB* columns[MAX_COL];
			int colCount;
		} DbData;

		typedef struct {
			IpfixDbReader::DbData* dbData;
		} DbReader;

		const char* hostName;    /** Hostname*/
		const char* dbName;      /**Name of the database*/
		const char* userName;    /**Username (default: Standarduser) */
		const char* password;    /** Password (default: none) */
		unsigned int portNum;    /** Portnumber (use default) */
		const char* socketName ;      /** Socketname (use default) */
		unsigned int flags;      /** Connectionflags (none) */
		MYSQL* conn;             /** pointer to connection handle */    
		IpfixDbReader::DbReader* dbReader;
		pthread_mutex_t mutex;   /** start/stop mutex for db replaying process */
		pthread_t thread;

		int getTables();
		int getColumns(int n);
		static void* readFromDB(void* ipfixDbReader_);
		int dbReaderSendNewTemplate(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo, int table_index);
		int dbReaderSendTable(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo, int n);
		int dbReaderDestroyTemplate(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo);
		int connectToDb(const char* hostName, const char* dbName, const char* username, const char* password, unsigned int port, uint16_t observationDomainId);
		IpfixDbReader::columnDB* getColumnByName(const char* name);
};

        

#endif


#endif
