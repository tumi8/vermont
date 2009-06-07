#ifndef PACKETIDMEFREPORTERCFG_H_
#define PACKETIDMEFREPORTERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/idmef/PacketIDMEFReporter.h"

#include <string>

using namespace std;


class PacketIDMEFReporterCfg
	: public CfgHelper<PacketIDMEFReporter, PacketIDMEFReporterCfg>
{
public:
	friend class ConfigManager;
	
	virtual PacketIDMEFReporterCfg* create(XMLElement* e);
	virtual ~PacketIDMEFReporterCfg();
	
	virtual PacketIDMEFReporter* createInstance();
	virtual bool deriveFrom(PacketIDMEFReporterCfg* old);
	
protected:
	
	string analyzerId;	/**< analyzer id for IDMEF messages */
	string idmefTemplate;	/**< template file for IDMEF messages */
	uint16_t snapshotOffset;    /**< offset from start of payload where snapshot is taken */
	uint16_t snapshotLength;    /**< length of snapshot */

	
	PacketIDMEFReporterCfg(XMLElement*);
};


#endif /*PACKETIDMEFREPORTERCFG_H_*/
