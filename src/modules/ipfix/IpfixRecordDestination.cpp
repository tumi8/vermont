/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2007 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
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

#include "IpfixRecordDestination.h"


IpfixRecordDestination::IpfixRecordDestination()
{
}

IpfixRecordDestination::~IpfixRecordDestination()
{
}


/**
 * tries to determine the received record type and calls the corresponding handler function
 */
void IpfixRecordDestination::receive(IpfixRecord* ipfixRecord)
{
	IpfixDataRecord* rec = dynamic_cast<IpfixDataRecord*>(ipfixRecord);
	if (rec) {
		onDataRecord(rec);
	} else {
		IpfixTemplateRecord* rec = dynamic_cast<IpfixTemplateRecord*>(ipfixRecord);
		if (rec) {
			onTemplate(rec);
		} else {
			IpfixTemplateDestructionRecord* rec = dynamic_cast<IpfixTemplateDestructionRecord*>(ipfixRecord);
			if (rec) {
				onTemplateDestruction(rec);
			}
		}
	}
}

/**
 * Callback function invoked when a new Template arrives.
 * @param sourceID SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 */
void IpfixRecordDestination::onTemplate(IpfixTemplateRecord* record)
{
	record->removeReference();
}

/**
 * Callback function invoked when a new Data Record arrives.
 * @param sourceID SourceID of the exporter that sent this Record
 * @param templateInfo Pointer to a structure defining the Template used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 */
void IpfixRecordDestination::onDataRecord(IpfixDataRecord* record)
{
	record->removeReference();
}

/**
 * Callback function invoked when a Template is being destroyed.
 * @param sourceID SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 */
void IpfixRecordDestination::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	record->removeReference();
}

