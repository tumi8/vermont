#ifndef AUTOFOCUSCFG_H_
#define AUTOFOCUSCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "AutoFocus.h"

#include <string>
#include <map>

using namespace std;


class AutoFocusCfg
	: public CfgHelper<AutoFocus, AutoFocusCfg>
{
public:
	friend class ConfigManager;
	
	virtual AutoFocusCfg* create(XMLElement* e);
	virtual ~AutoFocusCfg();
	
	virtual AutoFocus* createInstance();
	virtual bool deriveFrom(AutoFocusCfg* old);
	
protected:
	
	uint32_t hashBits;	/**< amount of bits used for hashtable */
	uint32_t timeTreeInterval; // time in seconds of interval when tree is being rebuilt
	uint32_t numMaxResults; //max number of results
	uint32_t numTrees;
	uint32_t minSubbits;
	string analyzerId;	/**< analyzer id for IDMEF messages */
	string reportfile;	/**< template file for IDMEF messages */
	
	AutoFocusCfg(XMLElement*);
};


#endif /*AUTOFOCUSCFG_H_*/
