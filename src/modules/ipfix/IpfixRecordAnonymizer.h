/*
 * IPFIX Record Anonymizer
 * Copyright (C) 2008 Lothar Braun
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

#ifndef _RECORD_ANONYMIZER_H_
#define _RECORD_ANONYMIZER_H_

#include "core/Source.h"
#include <modules/ipfix//IpfixRecordDestination.h>
#include <common/anon/AnonModule.h>

class IpfixRecordAnonymizer : public Source<IpfixRecord*>, public IpfixRecordDestination, public AnonModule, public Module  {
public:
	IpfixRecordAnonymizer() : copyMode(false) {}
	virtual ~IpfixRecordAnonymizer() {}

	void setCopyMode(bool mode);

protected:
	bool copyMode;	// if true, the anomymization is applied to a copy of the record

	static InstanceManager<IpfixDataRecord> dataRecordIM;

	virtual void onTemplate(IpfixTemplateRecord* record);
	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);
};

#endif
