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

#if defined(DB_SUPPORT_ENABLED)

#ifndef IPFIXDBREADERMYSQL_H
#define IPFIXDBREADERMYSQL_H

#include "IpfixDbReader.hpp"


#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <boost/smart_ptr.hpp>

#include <mysql.h>

/**
 *      IpfixDbReader powered the communication to the database server
 *      also between the other structs
 */
class IpfixDbReaderMySQL : public IpfixDbReader
{
	public:
		IpfixDbReaderMySQL(const string& dbType, const string& hostname, const string& dbname,
				const string& username, const string& password,
				uint16_t port, uint16_t observationDomainId);
		~IpfixDbReaderMySQL();
	protected:
		MYSQL* conn;             /** pointer to connection handle */    
		virtual int connectToDb();
		virtual int dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName);
		virtual int getColumns(const string& tableName);
		virtual int getTables();
};

        

#endif


#endif
