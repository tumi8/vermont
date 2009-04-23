/*
 * VERMONT 
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

#if !defined(P2PDETECTOR_H)
#define P2PDETECTOR_H

#include "idmef/IDMEFExporter.h"
#include "IpfixRecordDestination.h"
#include "Connection.h"
#include "reconf/Source.h"
#include "reconf/Notifiable.h"

#include <map>
#include <string>


using namespace std;

class P2PDetector 
	: public Module, 
	  public IpfixRecordDestination, 
	  public Source<IDMEFMessage*>,
	  public Notifiable
{
	public:
		P2PDetector(uint32_t intLength, uint32_t subnet, uint32_t subnetmask, string analyzerid, string tpl);
		virtual ~P2PDetector();
		
		virtual void onDataDataRecord(IpfixDataDataRecord* record);

	private: 
		//shutdown when module is not needed anymore
		void performShutdown();
		
		//returns true if a connection was successfull
		bool succConn(Connection& conn);
	
		//to calculate criteria after given timeouts
		void registerTimeout();
		void onTimeout();
		void onTimeout(void*);
		bool timeoutRegistered;
		timespec nextTimeout;
		
		uint32_t intLength; //interval length to be researched
		uint32_t subnet; //subnet to be researched
		uint32_t subnetmask; //subnetmask to be researched
		
		//to send idmef messages
		string analyzerId;
		string idmefTemplate;
		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;

		//private class for Hostentry in the hostList
		class P2PEntry {
			public:
				P2PEntry(){ 
					numUDPBiFlows = 0;
					numTCPBiFlows = 0;
				}
				uint32_t numUDPBiFlows; //for criteria 1
				map<uint32_t,bool> contactedUDPHosts; //for criteria 2
				uint32_t numTCPBiFlows; // for criteria 3
		};
		
		//list of all host entries to be researched
		map<uint32_t,P2PEntry> hostList;
		
};

#endif
