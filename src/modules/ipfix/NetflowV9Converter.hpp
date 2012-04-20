/*
 * NetflowV9 to IPFIX Converter Module
 * Copyright (C) 2009 Gerhard Muenz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _NETFLOWV9CONVERTER_H_
#define _NETFLOWV9CONVERTER_H_

#include "core/Source.h"
#include <modules/ipfix/IpfixRecordDestination.h>
#include <map>

class NetflowV9Converter : public Source<IpfixRecord*>, public IpfixRecordDestination, public Module  {
public:
	NetflowV9Converter(bool mode = true, bool keepsysuptime = false);
	virtual ~NetflowV9Converter() {}

	virtual void onReconfiguration2();


protected:
	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);

	bool copyMode;	// if false, the conversion is applied on the original data records (not a copy)
	bool keepFlowSysUpTime; // if true, only a new field firstSwitchedSeconds is applied

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

		// list of fields that need to be appended (only contains fields if flowSysUpTime is valid)
		std::list<TemplateInfo::FieldInfo> newFields;
		// store data length of the new fields
		uint32_t additionalDataLength;
	};

	std::map<uint16_t, ConvInfo> uniqueIdToConvInfo;
};

#endif
