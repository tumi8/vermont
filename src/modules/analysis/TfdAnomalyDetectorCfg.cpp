#include "TfdAnomalyDetectorCfg.h"
#include <netinet/in.h>
#include <arpa/inet.h>


TfdAnomalyDetectorCfg* TfdAnomalyDetectorCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new TfdAnomalyDetectorCfg(e);
}

TfdAnomalyDetectorCfg::TfdAnomalyDetectorCfg(XMLElement* elem)
    : CfgHelper<TfdAnomalyDetector, TfdAnomalyDetectorCfg>(elem, "tfdAnomalyDetector"),
    subnetmask(0)
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
                } else if (e->matches("tfdThreshold")) {
                    tfdThreshold = getDouble("tfdThreshold");
                } else if (e->matches("binSize")) {
                    binSize = getDouble("binSize");
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
            if (binSize == 0) THROWEXCEPTION("AnomalyDetectorCfg: binSize not set in configuration!");
            if (tfdThreshold == 0) THROWEXCEPTION("AnomalyDetectorCfg: tfdThreshold not set in configuration!");
            if (subnetmask == 0) THROWEXCEPTION("AnomalyDetectorCfg: subnetmask not set in configuration!");
            if (analyzerId=="") THROWEXCEPTION("AnomalyDetectorCfg: analyzerid not set in configuration!");
            //if (idmefTemplate=="") THROWEXCEPTION("AnomalyDetectorCfg: idmeftemplate not set in configuration!");
}

TfdAnomalyDetectorCfg::~TfdAnomalyDetectorCfg()
{
}

TfdAnomalyDetector* TfdAnomalyDetectorCfg::createInstance()
{
    instance = new TfdAnomalyDetector(subnet, subnetmask, tfdThreshold, binSize, analyzerId, idmefTemplate);
    return instance;
}

bool TfdAnomalyDetectorCfg::deriveFrom(TfdAnomalyDetectorCfg* old)
{
    return false;
}
