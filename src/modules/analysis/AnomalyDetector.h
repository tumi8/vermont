/* VERMONT 
 * Copyright (C) 2010 Manuela Schinn <manuela.schinn@gmx.de>
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

#if !defined(ANOMALYDETECTOR_H)
#define ANOMALYDETECTOR_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <list>
#include <string>

using namespace std;

class AnomalyDetector 
        : public Module,                  // start/shutdown/config modules
          public IpfixRecordDestination,  // base class for all vermont modules which want to receive flows
	        public Source<IDMEFMessage*>
{
    public:
        AnomalyDetector(uint32_t subnet, uint32_t subnetmask, double packetRateThreshold, string analyzerid, string idmeftemplate);
                                
        virtual ~AnomalyDetector();
          
        virtual void onDataRecord(IpfixDataRecord* record);
          
    private:

        float alpha;          // smoothing factor for EMA

        map<uint32_t, float> emaMap;  // map for EMA values (host, ema)
        
        uint32_t subnet;
        uint32_t subnetmask;
        double packetRateThreshold;
	      string analyzerId;
	      string idmefTemplate;

        static InstanceManager<IDMEFMessage> idmefManager;

        void checkConnection(Connection* conn);
};

#endif
