/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2007 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
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


#ifndef IPFIXRECORDSOURCE_H
#define IPFIXRECORDSOURCE_H

/**
 * used by IpfixCollector to pass ability to send IpfixRecords to IpfixParser
 * (reduced needed interface to a minimum)
 */
class IpfixRecordSender
{
public:
	virtual ~IpfixRecordSender() {}
	virtual bool send(IpfixRecord* ipfixRecord) = 0;
};


#endif
