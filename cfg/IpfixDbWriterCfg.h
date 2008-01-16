#ifndef IPFIXDBWRITERCFG_H_
#define IPFIXDBWRITERCFG_H_

#ifdef DB_SUPPORT_ENABLED

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include "concentrator/IpfixDbWriter.hpp"

#include <string>

using namespace std;


class IpfixDbWriterCfg
	: public CfgHelper<IpfixDbWriter, IpfixDbWriterCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbWriterCfg* create(XMLElement* e);
	virtual ~IpfixDbWriterCfg();
	
	virtual IpfixDbWriter* createInstance();
	virtual bool deriveFrom(IpfixDbWriterCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	uint16_t bufferRecords;	/**< amount of records to buffer until they are written to database */

	
	IpfixDbWriterCfg(XMLElement*);
};


#endif /*DB_SUPPORT_ENABLED*/

#endif /*IPFIXDBWRITERCFG_H_*/
