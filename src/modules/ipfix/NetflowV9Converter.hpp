#ifndef _NETFLOWV9CONVERTER_H_
#define _NETFLOWV9CONVERTER_H_

#include "core/Source.h"
#include <modules/ipfix/IpfixRecordDestination.h>
#include <map>

class NetflowV9Converter : public Source<IpfixRecord*>, public IpfixRecordDestination, public Module  {
public:
	NetflowV9Converter(bool mode = true) : copyMode(mode) {}
	virtual ~NetflowV9Converter() {}

	void setCopyMode(bool mode);

protected:
	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);

	bool copyMode;	// if false, the conversion is applied on the original data records (not a copy)

	static InstanceManager<IpfixDataRecord> dataRecordIM;
	static InstanceManager<IpfixTemplateRecord> templateRecordIM;
	static InstanceManager<IpfixTemplateDestructionRecord> templateDestructionRecordIM;

	struct ConvInfo {
		// IPFIX Template
		boost::shared_ptr<TemplateInfo> templateInfo;
		// System up time in unix seconds calculated from the header of the NetflowV9 message which contains the Netflow Template 
		// Note: We can assume that the absolut system up time remains the same for the upcoming Data Records 
		// and that Templates are resent at system restart.
		uint32_t sysUpUnixSeconds; 
		// Indexes of Template fields which contain timestamps to be converted
		std::list<uint16_t> fieldIndexes;
	};

	std::map<uint16_t, ConvInfo> uniqueIdToConvInfo;
};

#endif
