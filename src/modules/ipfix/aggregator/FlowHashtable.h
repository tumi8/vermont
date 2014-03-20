/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
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

#ifndef FLOWHASHTABLE_H_
#define FLOWHASHTABLE_H_


#include "BaseHashtable.h"

#include <boost/smart_ptr.hpp>
#include <map>

class FlowHashtable : public BaseHashtable
{
public:
	FlowHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
			uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);
	virtual ~FlowHashtable();

	void aggregateDataRecord(IpfixDataRecord* record);


private:
	int aggregateField(TemplateInfo::FieldInfo* basefi, TemplateInfo::FieldInfo* deltafi, IpfixRecord::Data* base,
		  IpfixRecord::Data* delta);
	int aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow, bool reverse);
	uint32_t getHash(IpfixRecord::Data* data, bool reverse);
	int equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2, bool reverse);
	HashtableBucket* lookupBucket(uint32_t hash, IpfixRecord::Data* data, bool reverse, HashtableBucket** prevbucket);
	void bufferDataBlock(boost::shared_array<IpfixRecord::Data> data, uint32_t flowStartSeconds, uint32_t flowEndTimeSeconds);
	int equalRaw(InformationElement::IeInfo* data1Type, IpfixRecord::Data* data1,
			InformationElement::IeInfo* data2Type, IpfixRecord::Data* data2);
	void copyData(TemplateInfo::FieldInfo* dstFI, IpfixRecord::Data* dst,
			TemplateInfo::FieldInfo* srcFI, IpfixRecord::Data* src, Rule::Field::Modifier modifier);
	int compare4ByteField(IpfixRecord::Data* baseFlow, TemplateInfo::FieldInfo* baseFi,
			IpfixRecord::Data* flow, TemplateInfo::FieldInfo* deltaFi);
	int compare8ByteField(IpfixRecord::Data* baseFlow, TemplateInfo::FieldInfo* baseFi,
			IpfixRecord::Data* flow, TemplateInfo::FieldInfo* deltaFi);
};

#endif /*FLOWHASHTABLE_H_*/
