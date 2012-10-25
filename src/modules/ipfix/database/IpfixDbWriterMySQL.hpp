/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007, 2008 Gerhard Muenz
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

/* Some constants that are common to IpfixDbWriterMySQL and IpfixDbReader */
#ifdef DB_SUPPORT_ENABLED

#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "IpfixDbCommon.hpp"
#include "IpfixDbWriterSQL.hpp"
#include "../IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include <mysql.h>
#include <netinet/in.h>
#include <time.h>
#include <sstream>

#define EXPORTERID 0

/**
 * IpfixDbWriterMySQL powered the communication to the database server
 * also between the other structs
 */
class IpfixDbWriterMySQL 
	: public IpfixDbWriterSQL
{
	public:
		IpfixDbWriterMySQL(const char* host, const char* db,
				const char* user, const char* pw,
				unsigned int port, uint16_t observationDomainId, // FIXME: observationDomainId
				int maxStatements);
		~IpfixDbWriterMySQL();

		virtual void connectToDB();
		virtual bool writeToDb();
		virtual int createExporterTable();
		virtual int getExporterID(IpfixRecord::SourceID* sourceID);
		virtual bool createDBTable(const char* partitionname, uint64_t starttime, uint64_t endtime);
		virtual Column* fillColumnStructure();

	private:
		MYSQL* conn;
};


#endif


#endif
