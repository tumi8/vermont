#ifndef IPFIXDBWRITERPGCFG_H_
#define IPFIXDBWRITERPGCFG_H_

#ifdef PG_SUPPORT_ENABLED

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include "concentrator/IpfixDbWriterPg.hpp"

#include <string>

using namespace std;


class IpfixDbWriterPgCfg
	: public CfgHelper<IpfixDbWriterPg, IpfixDbWriterPgCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbWriterPgCfg* create(XMLElement* e);
	virtual ~IpfixDbWriterPgCfg();
	
	virtual IpfixDbWriterPg* createInstance();
	virtual bool deriveFrom(IpfixDbWriterPgCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	uint16_t bufferRecords;	/**< amount of records to buffer until they are written to database */

	
	IpfixDbWriterPgCfg(XMLElement*);
};


#endif /*DB_SUPPORT_ENABLED*/

#endif /*IPFIXDBWRITERCFG_H_*/
