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
#include "common/crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>
#include <sys/stat.h>





/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortscanDetector
 * does not dare to delete it, in case it's used
 */
IpfixPayloadWriter::IpfixPayloadWriter(string path, string prefix, uint32_t noconns,
		bool ignoreEmptyPayload, bool ignoreIncompleteTCP, uint64_t startidx)
	: path(path),
	  filenamePrefix(prefix),
	  noConnections(noconns),
	  connectionID(startidx),
	  filewarningIssued(false),
	  ignoreEmptyPayload(ignoreEmptyPayload),
	  ignoreIncompleteTCP(ignoreIncompleteTCP),
	  statEmptyPayloadDropped(0),
	  statIncompleteTCPDropped(0)
{
	msg(MSG_INFO, "IpfixPayloadWriter started with following parameters:");
	msg(MSG_INFO, "  - path=%s", path.c_str());
	msg(MSG_INFO, "  - filenamePrefix=%s", filenamePrefix.c_str());
	msg(MSG_INFO, "  - noConnections=%u", noConnections);
	msg(MSG_INFO, "  - startIndex=%llu", connectionID);
	msg(MSG_INFO, "  - ignoreEmptyPayload=%u", ignoreEmptyPayload);
	msg(MSG_INFO, "  - ignoreIncompleteTCP=%u", ignoreIncompleteTCP);
}


IpfixPayloadWriter::~IpfixPayloadWriter()
{
}


void IpfixPayloadWriter::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	// convert ipfixrecord to connection struct
	Connection* conn = new Connection(record);
	conn->swapIfNeeded();

	bool inserted = false;

	if (!ignoreEmptyPayload || conn->srcPayloadLen>0 || conn->dstPayloadLen>0) {
		if (!ignoreIncompleteTCP || conn->protocol!=6 ||
				((conn->srcTcpControlBits&2)==2 && ((conn->dstTcpControlBits&2)==2))) { // check if both directions have SYN flag set
			if (noConnections>0) {
				// insert entry into sorted list
				list<Connection*>::iterator iter = connections.begin();

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
			} else {
				// write entry directly to filesystem
				dumpEntry(conn);
			}
		} else {
			statIncompleteTCPDropped++;
		}
	} else {
		statEmptyPayloadDropped++;
	}

	if (!inserted) delete conn;

	record->removeReference();
}

void IpfixPayloadWriter::dumpEntry(Connection* conn)
{
	char filename[2][100];
	char idxpath[2][100];
	snprintf(filename[0], 100, "%s-%04llu-%s.%d-%s.%d", filenamePrefix.c_str(),
			(long long unsigned)connectionID, IPToString(conn->srcIP).c_str(), ntohs(conn->srcPort), IPToString(conn->dstIP).c_str(), ntohs(conn->dstPort));
	snprintf(filename[1], 100, "%s-%04llu-%s.%d-%s.%d", filenamePrefix.c_str(),
			(long long unsigned)connectionID, IPToString(conn->dstIP).c_str(), ntohs(conn->dstPort), IPToString(conn->srcIP).c_str(), ntohs(conn->srcPort));
	snprintf(idxpath[0], ARRAY_SIZE(idxpath[0]), "/%04llX", (long long unsigned)(connectionID>>16));
	snprintf(idxpath[1], ARRAY_SIZE(idxpath[1]), "/%02llX/", (long long unsigned)(connectionID>>8));
	connectionID++;

	// create paths, if needed
	struct stat s;
	string mkpath = path+string(idxpath[0]);
	if (stat(mkpath.c_str(), &s) != 0) {
		if (mkdir(mkpath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0)
			THROWEXCEPTION("error while creating directory '%s': %s", mkpath.c_str(), strerror(errno));
	}
	mkpath += string(idxpath[1]);
	if (stat(mkpath.c_str(), &s) != 0) {
		if (mkdir(mkpath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)!=0)
			THROWEXCEPTION("error while creating directory '%s': %s", mkpath.c_str(), strerror(errno));
	}

	string filepayload[2] = { mkpath + string(filename[0]) + ".payload", mkpath + string(filename[1]) + ".payload" };
	string fileinfo = mkpath + string(filename[0]) + ".info";

	msg(MSG_VDEBUG, "writing files for connection %s", filename[0]);

	if (stat(filepayload[0].c_str(), &s) == 0 && !filewarningIssued) {
		msg(MSG_DIALOG, "files in IpfixPayloadWriter destination directory already present, overwriting ...");
		filewarningIssued = true;
	}
	// save payload in two files
	FILE* f;
	char buf[500];
	if (conn->srcPayload) {
		f = fopen(filepayload[0].c_str(), "w+");
		if (f == NULL) THROWEXCEPTION("failed to open file '%s', error: %s", filepayload[0].c_str(), strerror(errno));
		if (conn->srcPayloadLen && fwrite(conn->srcPayload, conn->srcPayloadLen, 1, f) != 1)
			THROWEXCEPTION("failed to write to file '%s', error: %s, 1", filepayload[0].c_str(), strerror(errno));
		if (fclose(f) != 0)
			THROWEXCEPTION("failed to close file '%s', error: %s, 2", filepayload[0].c_str(), strerror(errno));
	}
	if (conn->dstPayload) {
		f = fopen(filepayload[1].c_str(), "w+");
		if (f == NULL) THROWEXCEPTION("failed to open file '%s', error: %s", filepayload[1].c_str(), strerror(errno));
		if (conn->dstPayloadLen && fwrite(conn->dstPayload, conn->dstPayloadLen, 1, f) != 1)
			THROWEXCEPTION("failed to write to file '%s', error: %s, 3", filepayload[1].c_str(), strerror(errno));
		if (fclose(f) != 0)
			THROWEXCEPTION("failed to close file '%s', error: %s, 4", filepayload[1].c_str(), strerror(errno));
	}

	// save additional data
	f = fopen(fileinfo.c_str(), "w+");
	if (f == NULL) THROWEXCEPTION("failed to open file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "srcIP: %s\n", IPToString(conn->srcIP).c_str());
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "dstIP: %s\n", IPToString(conn->dstIP).c_str());
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "srcPort: %u\n", ntohs(conn->srcPort));
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "dstPort: %u\n", htons(conn->dstPort));
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "srcFlowTimes: %llu / %llu\n", (long long unsigned)conn->srcTimeStart, (long long unsigned)conn->srcTimeEnd);
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "revFlowTimes: %llu / %llu\n", (long long unsigned)conn->dstTimeStart, (long long unsigned)conn->dstTimeEnd);
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "srcOctets: %llu\n", (long long unsigned)htonll(conn->srcOctets));
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "revOctets: %llu\n", (long long unsigned)htonll(conn->dstOctets));
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "srcPackets: %llu\n", (long long unsigned)htonll(conn->srcPackets));
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "revPackets: %llu\n", (long long unsigned)htonll(conn->dstPackets));
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "protocol: %d\n", conn->protocol);
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "srcPayloadLen: %d\n", conn->srcPayloadLen);
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	snprintf(buf, 100, "dstPayloadLen: %d\n", conn->dstPayloadLen);
	if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));

	if (conn->srcPayload != 0) {
		snprintf(buf, 100, "nicePayload: ");
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
		if (conn->srcPayloadLen>0) {
			uint32_t buflen = (conn->srcPayloadLen > ARRAY_SIZE(buf) ? ARRAY_SIZE(buf) : conn->srcPayloadLen);
			memcpy(buf, conn->srcPayload, buflen);
			for (uint32_t i=0; i<buflen && i<conn->srcPayloadLen; i++) {
				if (!isprint(buf[i])) buf[i] = '.';
			}
			if (fwrite(buf, buflen, 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
		}
		if (fwrite("\n", 1, 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	}

	if (conn->dstPayload != 0) {
		snprintf(buf, 100, "revNicePayload: ");
		if (fwrite(buf, strnlen(buf, 100), 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
		if (conn->dstPayloadLen>0) {
			uint32_t buflen = (conn->dstPayloadLen > ARRAY_SIZE(buf) ? ARRAY_SIZE(buf) : conn->dstPayloadLen);
			memcpy(buf, conn->dstPayload, buflen);
			for (uint32_t i=0; i<buflen && i<conn->dstPayloadLen; i++) {
				if (!isprint(buf[i])) buf[i] = '.';
			}
			if (fwrite(buf, buflen, 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
		}
		if (fwrite("\n", 1, 1, f) != 1) THROWEXCEPTION("failed to write to file '%s', error: %s", fileinfo.c_str(), strerror(errno));
	}

	if (fclose(f) != 0) THROWEXCEPTION("failed to close file '%s', error: %s", fileinfo.c_str(), strerror(errno));
}


void IpfixPayloadWriter::performShutdown()
{
	if (noConnections==0) return;

	// write the first N entries in connection list to directory in files
	list<Connection*>::iterator iter = connections.begin();
	while (iter!=connections.end()) {
		if (connectionID>=noConnections) break;
		Connection* conn = *iter;
		dumpEntry(conn);
		iter++;
	}

	// delete entries in list
	while (!connections.empty()) {
		Connection* c = connections.back();
		delete c;
		connections.pop_back();
	}

	return;
}

string IpfixPayloadWriter::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<incompleteTCPDropped>" << statIncompleteTCPDropped << "</incompleteTCPDropped>";
	oss << "<emptyPayloadDropped>" << statEmptyPayloadDropped << "</emptyPayloadDropped>";
	return oss.str();
}

