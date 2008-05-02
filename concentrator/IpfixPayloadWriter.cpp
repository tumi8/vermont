/*
 * VERMONT 
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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
 */

#include "IpfixPayloadWriter.h"
#include "crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>
#include <sys/stat.h>





/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortscanDetector
 * does not dare to delete it, in case it's used
 */
IpfixPayloadWriter::IpfixPayloadWriter(string path, string prefix, uint32_t noconns)
	: path(path),
	  filenamePrefix(prefix),
	  noConnections(noconns),
	  filewarningIssued(false)
{
}

IpfixPayloadWriter::~IpfixPayloadWriter()
{
}

void IpfixPayloadWriter::onDataDataRecord(IpfixDataDataRecord* record)
{	
	// convert ipfixrecord to connection struct
	Connection* conn = new Connection(record);	
	conn->swapIfNeeded();
	
	// insert entry into sorted list
	list<Connection*>::iterator iter = connections.begin();
	bool inserted = false;
	uint32_t counter = 0;
	while (iter != connections.end() && counter<noConnections) {
		if ((*iter)->srcTimeStart>conn->srcTimeStart) {
			connections.insert(iter, conn);
			inserted = true;
			break;
		}
		counter++;
		iter++;
	}
	if ((!inserted) && (counter<noConnections)) {
		connections.push_back(conn);
		inserted = true;
	}
	
	if (!inserted) delete conn;
	
	record->removeReference();
}

void IpfixPayloadWriter::performShutdown()
{
	string fileinfo;	
	
	// write the first N entries in connection list to directory in files
	list<Connection*>::iterator iter = connections.begin();
	uint32_t i = 0;
	while (iter!=connections.end()) {
		if (i>=noConnections) break;
		Connection* conn = *iter;
		char filename[2][100];
		snprintf(filename[0], 100, "%s-%02d-%s.%d-%s.%d", filenamePrefix.c_str(), 
				i, IPToString(conn->srcIP).c_str(), ntohs(conn->srcPort), IPToString(conn->dstIP).c_str(), ntohs(conn->dstPort));
		snprintf(filename[1], 100, "%s-%02d-%s.%d-%s.%d", filenamePrefix.c_str(), 
				i, IPToString(conn->dstIP).c_str(), ntohs(conn->dstPort), IPToString(conn->srcIP).c_str(), ntohs(conn->srcPort));
		
		string filepayload[2] = { path + "/" + string(filename[0]) + ".payload", path + "/" + string(filename[1]) + ".payload" };
		fileinfo = path + "/" + string(filename[0]) + ".info";
		
		struct stat s;
		if (stat(filepayload[0].c_str(), &s) == 0 && !filewarningIssued) {
			msg(MSG_ERROR, "files in IpfixPayloadWriter destination directory already present, overwriting ...");
			filewarningIssued = true;
		}
	
		
		// save payload in two files
		FILE* f;
		char buf[100];
		uint32_t buflen = (conn->srcPayloadLen > 100 ? 100 : conn->srcPayloadLen);
		if (conn->srcPayload) {
			f = fopen(filepayload[0].c_str(), "w+");
			if (f == NULL) goto error;
			if (fwrite(conn->srcPayload, conn->srcPayloadLen, 1, f) != 1) 
				THROWEXCEPTION("failed to write to file '%s', error: %s", filepayload[0].c_str(), strerror(errno));
			if (fclose(f) != 0) 
				THROWEXCEPTION("failed to write to file '%s', error: %s", filepayload[0].c_str(), strerror(errno));
		}
		if (conn->dstPayload) {
			f = fopen(filepayload[1].c_str(), "w+");
			if (f == NULL) goto error;
			if (fwrite(conn->dstPayload, conn->dstPayloadLen, 1, f) != 1) 
				THROWEXCEPTION("failed to write to file '%s', error: %s", filepayload[1].c_str(), strerror(errno));
			if (fclose(f) != 0) 
				THROWEXCEPTION("failed to write to file '%s', error: %s", filepayload[1].c_str(), strerror(errno));
		}
	
		// save additional data
		f = fopen(fileinfo.c_str(), "w+");
		if (f == NULL) goto error;
		snprintf(buf, 100, "srcIP: %s\n", IPToString(conn->srcIP).c_str());
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "dstIP: %s\n", IPToString(conn->dstIP).c_str());
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "srcPort: %u\n", ntohs(conn->srcPort));
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "dstPort: %u\n", htons(conn->dstPort));
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "srcFlowTimes: %llu / %llu\n", conn->srcTimeStart, conn->srcTimeEnd);
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "revFlowTimes: %llu / %llu\n", conn->dstTimeStart, conn->dstTimeEnd);
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "srcOctets: %llu\n", htonll(conn->srcOctets));
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "revOctets: %llu\n", htonll(conn->dstOctets));
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "srcPackets: %llu\n", htonll(conn->srcPackets));
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "revPackets: %llu\n", htonll(conn->dstPackets));
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
		snprintf(buf, 100, "protocol: %d\n", conn->protocol);
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;
	
		if (conn->srcPayload != 0) {
			snprintf(buf, 100, "nicePayload: ");
			if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;	
			memcpy(buf, conn->srcPayload, buflen);
			for (uint32_t i=0; i<buflen && i<conn->srcPayloadLen; i++) {
				if (!isprint(buf[i])) buf[i] = '.';
			}
			if (fwrite(buf, buflen, 1, f) != 1) goto error;
			if (fwrite("\n", 1, 1, f) != 1) goto error;
		}
		
		if (conn->dstPayload != 0) {
			snprintf(buf, 100, "revNicePayload: ");
			if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) goto error;	
			memcpy(buf, conn->dstPayload, buflen);
			for (uint32_t i=0; i<buflen && i<conn->dstPayloadLen; i++) {
				if (!isprint(buf[i])) buf[i] = '.';
			}
			if (fwrite(buf, buflen, 1, f) != 1) goto error;
			if (fwrite("\n", 1, 1, f) != 1) goto error;
		}
		
		if (fclose(f) != 0) goto error;
		
		iter++;
		i++;
	}
	
	// delete entries in list
	while (!connections.empty()) {		
		Connection* c = connections.back();
		delete c;
		connections.pop_back();
	}
	
	return;

error:
	THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
}

