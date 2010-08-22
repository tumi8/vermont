#ifndef TFDANOMALYDETECTORCFG_H_
#define TFDANOMALYDETECTORCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/analysis/TfdAnomalyDetector.h"

#include <string>

using namespace std;

class TfdAnomalyDetectorCfg
        : public CfgHelper<TfdAnomalyDetector, TfdAnomalyDetectorCfg>
{
public:
        friend class ConfigManager;
        
        virtual TfdAnomalyDetectorCfg* create(XMLElement* e);
        virtual ~TfdAnomalyDetectorCfg();
        
        virtual TfdAnomalyDetector* createInstance();
        virtual bool deriveFrom(TfdAnomalyDetectorCfg* old);
        
protected:
        string analyzerId;
        string idmefTemplate;
        uint32_t subnetmask;
        uint32_t subnet;
        double tfdThreshold;
        double ivLength;

        TfdAnomalyDetectorCfg(XMLElement*);
};


#endif /*ANOMALYDETECTORCFG_H_m*/
