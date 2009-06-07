#ifndef IPFIXDBREADERCFG_H_
#define IPFIXDBREADERCFG_H_

#ifdef DB_SUPPORT_ENABLED

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include "concentrator/IpfixDbReader.hpp"

#include <string>

using namespace std;


class IpfixDbReaderCfg
	: public CfgHelper<IpfixDbReader, IpfixDbReaderCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbReaderCfg* create(XMLElement* e);
	virtual ~IpfixDbReaderCfg();
	
	virtual IpfixDbReader* createInstance();
	virtual bool deriveFrom(IpfixDbReaderCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	bool timeshift; /**< shift time stamps */
	bool fullspeed;  /**< reading in full speed */
	uint32_t observationDomainId;	/**< observation domain id */
	
	IpfixDbReaderCfg(XMLElement*);
};


#endif /*DB_SUPPORT_ENABLED*/

#endif /*IPFIXDBREADERCFG_H_*/
