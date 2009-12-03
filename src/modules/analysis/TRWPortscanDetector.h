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

#if !defined(TRWPORTSCANDETECTOR_H)
#define TRWPORTSCANDETECTOR_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <list>
#include <string>

using namespace std;

class TRWPortscanDetector 
	: public Module, 
	  public IpfixRecordDestination, 
	  public Source<IDMEFMessage*>
{
	public:
		TRWPortscanDetector(uint32_t hashbits, uint32_t texppend, uint32_t texpscan, 
				uint32_t texpben, uint32_t tcleanint, string analyzerid, string idmeftemplate);
		virtual ~TRWPortscanDetector();
		
		virtual void onDataRecord(IpfixDataRecord* record);

	private:
		enum TRWDecision { PENDING, SCANNER, BENIGN };
		struct TRWEntry {
			uint32_t srcIP;
			uint32_t dstSubnet;
			uint32_t dstSubnetMask;
			uint32_t numFailedConns;
			uint32_t numSuccConns;
			int32_t timeExpire;
			list<uint32_t> accessedHosts;
			float S_N;
			TRWDecision decision;
		};

		uint32_t hashSize;
		uint32_t hashBits;	/**< amount of bits used for hashtable */
		uint32_t timeExpirePending; // time in seconds until pending entries are expired
		uint32_t timeExpireScanner; // time in seconds until scanner entries are expired
		uint32_t timeExpireBenign; // time in seconds until benign entries are expired
		uint32_t timeCleanupInterval; // time in seconds of interval when hashtable with source hosts is cleaned up (trwEntries)
		string analyzerId;	/**< analyzer id for IDMEF messages */
		string idmefTemplate;	/**< template file for IDMEF messages */


		// idmef parameters
		const static char* PAR_SUCC_CONNS; // = "SUCC_CONNS";
		const static char* PAR_FAILED_CONNS; // = "FAILED_CONNS";

		list<TRWEntry*>* trwEntries;
		uint32_t statEntriesAdded;
		uint32_t statEntriesRemoved;
		uint32_t statNumScanners;
		float logeta_0, logeta_1;
		float X_0, X_1;
		time_t lastCleanup;
		
		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;

		TRWEntry* createEntry(Connection* conn);
		TRWEntry* getEntry(Connection* conn);
		void addConnection(Connection* conn);
		virtual string getStatisticsXML(double interval);
		void cleanupEntries();
};

#endif
