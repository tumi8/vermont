#ifndef ANOMALYDETECTORCFG_H_
#define ANOMALYDETECTORCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/analysis/AnomalyDetector.h"

#include <string>

using namespace std;

class AnomalyDetectorCfg
        : public CfgHelper<AnomalyDetector, AnomalyDetectorCfg>
{
public:
        friend class ConfigManager;
        
        virtual AnomalyDetectorCfg* create(XMLElement* e);
        virtual ~AnomalyDetectorCfg();
        
        virtual AnomalyDetector* createInstance();
        virtual bool deriveFrom(AnomalyDetectorCfg* old);
        
protected:
        string analyzerId;
        string idmefTemplate;
        uint32_t subnetmask;
        uint32_t subnet;
        double packetRateThreshold;
        double alpha;

        AnomalyDetectorCfg(XMLElement*);
};


#endif /*ANOMALYDETECTORCFG_H_m*/
