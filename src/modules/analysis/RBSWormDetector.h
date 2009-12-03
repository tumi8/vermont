/*
 * VERMONT 
 * Copyright (C) 2008 David Eckhoff <sidaeckh@informatik.stud.uni-erlangen.de>
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

#if !defined(RBSWORMDETECTOR_H)
#define RBSWORMDETECTOR_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <list>
#include <string>
#include <map>

using namespace std;

class RBSWormDetector
	: public Module, 
	  public IpfixRecordDestination, 
	  public Source<IDMEFMessage*>
{
	public:
		RBSWormDetector(uint32_t hashbits, uint32_t texppend, uint32_t texpworm, 
				uint32_t texpben, uint32_t tadaptint,uint32_t tcleanupint, float lambda_ratio, string analyzerid, string idmeftemplate,map<uint32_t,uint32_t> subNets);
		virtual ~RBSWormDetector();
		
		virtual void onDataRecord(IpfixDataRecord* record);

	private:
		enum RBSDecision { PENDING, WORM, BENIGN };
		struct RBSEntry {
			uint32_t srcIP;
			uint32_t numFanouts;
			uint64_t startTime;
			uint64_t lastPacket;
			uint32_t totalSSDur;
			uint32_t totalSSNum;
			bool switched;
			double mean;
			int32_t timeExpire;
			list<uint32_t> accessedHosts;
			RBSDecision decision;
		};

		uint32_t hashSize;
		uint32_t hashBits;	/**< amount of bits used for hashtable */
		uint32_t timeExpirePending; // time in seconds until pending entries are expired
		uint32_t timeExpireWorm; // time in seconds until worm entries are expired
		uint32_t timeExpireBenign; // time in seconds until benign entries are expired
		uint32_t timeCleanupInterval; // time in seconds of interval when hashtable with source hosts is cleaned up (rbsEntries)
		uint32_t timeAdaptInterval; // time in seconds of interval when lamdbas (expected benign & worm frequencies) are changed
		string analyzerId;	/**< analyzer id for IDMEF messages */
		string idmefTemplate;	/**< template file for IDMEF messages */
		float lambda_ratio;
		map<uint32_t,uint32_t> subnets;
		// idmef parameters
		const static char* PAR_FAN_OUT; // = "FAN_OUT";
		const static char* PAR_TOTALTIME; // = "FAN_OUT";
		const static char* PAR_HOSTS; // = "FAN_OUT";
		

		list<RBSEntry*>* rbsEntries;
		uint32_t statEntriesAdded;
		uint32_t statEntriesRemoved;
		uint32_t statNumWorms;
		uint32_t statCurBenign;
		float lambda_0,lambda_1;
		float slope_0a,slope_0b,slope_1a,slope_1b;
		time_t lastCleanup,lastAdaption;
		
		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;

		RBSEntry* createEntry(Connection* conn);
		RBSEntry* getEntry(Connection* conn);
		void addConnection(Connection* conn);
		static bool comp_entries(RBSEntry*,RBSEntry*);
		virtual string getStatistics();
		virtual std::string getStatisticsXML(double);
		void cleanupEntries();
		void adaptFrequencies();
};

#endif
