#include "AnomalyDetectorCfg.h"
#include <netinet/in.h>
#include <arpa/inet.h>


AnomalyDetectorCfg* AnomalyDetectorCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new AnomalyDetectorCfg(e);
}

AnomalyDetectorCfg::AnomalyDetectorCfg(XMLElement* elem)
    : CfgHelper<AnomalyDetector, AnomalyDetectorCfg>(elem, "anomalyDetector"),
    subnetmask(0),
    packetRateThreshold(0),
    emaTimeout(0)
    
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
        for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
             it != set.end();
             it++) {
                XMLElement* e = *it;
		
                if (e->matches("subnet")) {
                    string ipstring = e->getFirstText();
                    subnet = 0;
                    subnetmask = inet_addr("255.255.255.255");
                    int last_pos = ipstring.find("/");
                    if (last_pos)
                    {
                        string tempstr = ipstring.substr(0,last_pos);
                        subnet = inet_addr(tempstr.c_str());
                        tempstr = ipstring.substr(last_pos+1,ipstring.length());
                        subnetmask <<= (32-atoi(tempstr.c_str()));
                    }
                    subnetmask = htonl(subnetmask);
                } else if (e->matches("packetRateThreshold")) {
                    packetRateThreshold = getDouble("packetRateThreshold");
                } else if (e->matches("emaTimeout")) {
                    emaTimeout = getDouble("emaTimeout");
		            } else if (e->matches("analyzerid")) {
			              analyzerId = e->getFirstText();
		            } else if (e->matches("idmeftemplate")) {
			              idmefTemplate = e->getFirstText();
                } else if (e->matches("next")) { // ignore next
                } else {
                    msg(MSG_FATAL, "Unknown AnomalyDetector config statement %s\n", e->getName().c_str());
                    continue;
                }
            }
            if (subnetmask == 0) THROWEXCEPTION("AnomalyDetectorCfg: subnetmask not set in configuration!");
            if (packetRateThreshold == 0) THROWEXCEPTION("AnomalyDetectorCfg: packetRateThreshold not set in configuration!");
            if (emaTimeout == 0) THROWEXCEPTION("AnomalyDetectorCfg: emaTimeout not set in configuration!");
            if (analyzerId=="") THROWEXCEPTION("AnomalyDetectorCfg: analyzerid not set in configuration!");
            //if (idmefTemplate=="") THROWEXCEPTION("AnomalyDetectorCfg: idmeftemplate not set in configuration!");
}

AnomalyDetectorCfg::~AnomalyDetectorCfg()
{
}

AnomalyDetector* AnomalyDetectorCfg::createInstance()
{
    instance = new AnomalyDetector(subnet, subnetmask, packetRateThreshold, emaTimeout, analyzerId, idmefTemplate);
    return instance;
}

bool AnomalyDetectorCfg::deriveFrom(AnomalyDetectorCfg* old)
{
    return false;
}
