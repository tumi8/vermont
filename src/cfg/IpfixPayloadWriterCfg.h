#ifndef IPFIXPAYLOADWRITERCFG_H_
#define IPFIXPAYLOADWRITERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

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
	bool ignoreIncompleteTCP; /**< ignores all TCP flows without set TCP flag in both directions */
	uint64_t startIdx; 		/**< start index of flows, important if dump needs to be continued */

	IpfixPayloadWriterCfg(XMLElement*);
};


#endif /*IPFIXPAYLOADWRITERCFG_H_*/
