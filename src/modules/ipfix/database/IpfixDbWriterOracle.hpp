/*
 * IPFIX Database Reader/Writer Oracle Connector
 * Copyright (C) 2011 Philipp Fehre <philipp.fehre@googlemail.com>
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
#ifdef ORACLE_SUPPORT_ENABLED

#ifndef IPFIXDBWRITERORACLE_H_
#define IPFIXDBWRITERORACLE_H_

#include "IpfixDbCommon.hpp"
#include "IpfixDbWriterSQL.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include <occi.h>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <time.h>
#include <sstream>

using namespace std;

#define EXPORTERID 0

/**
 * IpfixDbWriterMySQL powered the communication to the database server
 * also between the other structs
 */
class IpfixDbWriterOracle 
	: public IpfixDbWriterSQL
{
	public:
		IpfixDbWriterOracle(const char* dbType, const char* host, const char* db,
				const char* user, const char* pw,
				unsigned int port, uint16_t observationDomainId, // FIXME: observationDomainId
				int maxStatements, vector<string> columns, bool legacyNames, const char* prefix);
		~IpfixDbWriterOracle();

		virtual void connectToDB();
		virtual bool writeToDb();
		virtual int createExporterTable();
		virtual int getExporterID(IpfixRecord::SourceID* sourceID);
		virtual bool createDBTable(const char* partitionname, uint64_t starttime, uint64_t endtime);
		virtual string getInsertString(string tableName);
		virtual string insertRowPrefix();

	private:
		oracle::occi::Environment *env;
		oracle::occi::Connection *con;

};

#endif

#endif


