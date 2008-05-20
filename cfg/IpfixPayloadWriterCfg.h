#ifndef IPFIXPAYLOADWRITERCFG_H_
#define IPFIXPAYLOADWRITERCFG_H_

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include "concentrator/IpfixPayloadWriter.h"

#include <string>

using namespace std;


class IpfixPayloadWriterCfg
	: public CfgHelper<IpfixPayloadWriter, IpfixPayloadWriterCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixPayloadWriterCfg* create(XMLElement* e);
	virtual ~IpfixPayloadWriterCfg();
	
	virtual IpfixPayloadWriter* createInstance();
	virtual bool deriveFrom(IpfixPayloadWriterCfg* old);
	
protected:
	
	string path;			/**< path where files are written to */
	string filenamePrefix;	/**< prefix of all filenames that are created */
	uint32_t noConnections; /**< how many connections of a run should be recorded? */
	bool ignoreEmptyPayload; /**< ignores all flows having empty payload in both directions when set */
	
	IpfixPayloadWriterCfg(XMLElement*);
};


#endif /*IPFIXPAYLOADWRITERCFG_H_*/
