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

#if !defined(IPFIXPAYLOADWRITER_H)
#define IPFIXPAYLOADWRITER_H

#include "modules/idmef/IDMEFExporter.h"
#include "IpfixRecordDestination.h"
#include "Connection.h"
#include "core/Source.h"

#include <list>
#include <string>

using namespace std;

class IpfixPayloadWriter
	: public Module,
	  public IpfixRecordDestination,
	  public Source<NullEmitable*>
{
	public:
		IpfixPayloadWriter(string path, string prefix, uint32_t noconns, bool ignoreEmptyPayload, bool ignoreIncompleteTCP, uint64_t startidx);
		virtual ~IpfixPayloadWriter();

		virtual void onDataRecord(IpfixDataRecord* record);
		string getStatisticsXML(double interval);

	protected:
		virtual void performShutdown();
		void dumpEntry(Connection* conn);

	private:
		string path;
		string filenamePrefix;
		uint32_t noConnections; // how many connections of a run should be recorded?
		uint64_t connectionID; // current connection id when storing files
		list<Connection*> connections; // sorted list of recorded connections
		bool filewarningIssued;
		bool ignoreEmptyPayload; // ignores all flows with empty payload when set
		bool ignoreIncompleteTCP; // ignores all flows where TCP SYN packets in both directions are missing
		uint64_t statEmptyPayloadDropped;
		uint64_t statIncompleteTCPDropped;
};

#endif
