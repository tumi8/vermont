/*
 * IPFIX Database Reader/Writer for Oracle DBs
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006-2013 Lothar Braun <braun@net.in.tum.de>
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

#ifndef IPFIXDBREADERORACLE_H
#define IPFIXDBREADERORACLE_H

#include "../IpfixRecord.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include "core/Module.h"
#include "IpfixDbReader.hpp"

#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <boost/smart_ptr.hpp>

#include <occi.h>

/**
 *      IpfixDbReader powered the communication to the database server
 *      also between the other structs
 */
class IpfixDbReaderOracle : public IpfixDbReader
{
public:
	IpfixDbReaderOracle(const string& dbType, const string& hostname, const string& dbname,
			const string& username, const string& password,
			uint16_t port, uint16_t observationDomainId);
	~IpfixDbReaderOracle();
	
protected:
	bool dbError; // error flag 
	oracle::occi::Environment *env;
	oracle::occi::Connection *con;

	virtual int connectToDb();
	virtual int dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName);
	virtual int getColumns(const string& tableName);
	virtual int getTables();

};

#endif


#endif
