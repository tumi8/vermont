#ifndef IDMEFEXPORTERCFG_H_
#define IDMEFEXPORTERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/idmef/IDMEFExporter.h"

#include <string>

using namespace std;

class IDMEFExporterCfg
	: public CfgHelper<IDMEFExporter, IDMEFExporterCfg>
{
public:
	friend class ConfigManager;
	
	virtual IDMEFExporterCfg* create(XMLElement* e);
	virtual ~IDMEFExporterCfg();
	
	virtual IDMEFExporter* createInstance();
	virtual bool deriveFrom(IDMEFExporterCfg* old);
	
protected:	
	string destDirectory;	/**< directory where idmef message are to be saved temporarily */
	string sendURL;			/**< URL where IDMEF messages are to be sent */
	
	IDMEFExporterCfg(XMLElement*);
};



#endif /*IDMEFEXPORTERCFG_H_*/
