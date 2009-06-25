#ifndef IPFIXRECEIVERFILECFG_H_
#define IPFIXRECEIVERFILECFG_H_

#include "core/Cfg.h"
#include <modules/ipfix/IpfixCollector.hpp>
#include <modules/ipfix/IpfixReceiverFile.hpp>

class IpfixReceiverFileCfg
	: public CfgHelper<IpfixCollector, IpfixReceiverFileCfg>
{
public:
	IpfixReceiverFileCfg(XMLElement* elem);
	virtual ~IpfixReceiverFileCfg();
	
	virtual IpfixReceiverFileCfg* create(XMLElement* elem);
	
	virtual IpfixCollector* createInstance();
	
	bool deriveFrom(IpfixReceiverFileCfg* old);
	
private:
        IpfixCollector* ipfixCollector;
        uint16_t observationDomainId;
		std::string packetFileBasename;
		std::string packetFileDirectory;
		int c_from;
		int c_to;
		bool ignore;
		float offlinespeed;
};

#endif /*IPFIXRECEIVERFILECFG_H_*/
