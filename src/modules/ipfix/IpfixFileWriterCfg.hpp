#ifndef IPFIXFILEWRITERCFG_H_
#define IPFIXFILEWRITERCFG_H_
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixFileWriter.hpp"

#include <map>


class IpfixFileWriterCfg
	: public CfgHelper<IpfixFileWriter, IpfixFileWriterCfg>
{
public:
	friend class ConfigManager;

	virtual IpfixFileWriterCfg* create(XMLElement* e);

	virtual ~IpfixFileWriterCfg();

	virtual IpfixFileWriter* createInstance();

	virtual bool deriveFrom(IpfixFileWriterCfg* old);

protected:
	IpfixFileWriterCfg(XMLElement*);

private:
	// config variables
	std::string destinationPath;
	std::string filenamePrefix;
	uint32_t maximumFilesize;
	uint16_t observationDomainId;
};

#endif /*IPFIXFILEWRITERCFG_H_*/

