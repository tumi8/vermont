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
	{
		IpfixDataRecord* rec = dynamic_cast<IpfixDataRecord*>(ipfixRecord);
		if (rec) onDataRecord(rec);
			//onDataRecord(rec->sourceID.get(), rec->templateInfo.get(), rec->dataLength, rec->data);
	}
	{
		IpfixDataDataRecord* rec = dynamic_cast<IpfixDataDataRecord*>(ipfixRecord);
		if (rec) onDataDataRecord(rec);
			//onDataDataRecord(rec->sourceID.get(), rec->dataTemplateInfo.get(), rec->dataLength,
				//	rec->data);
	}
	{
		IpfixOptionsRecord* rec = dynamic_cast<IpfixOptionsRecord*>(ipfixRecord);
		if (rec) onOptionsRecord(rec);
			//onOptionsRecord(rec->sourceID.get(), rec->optionsTemplateInfo.get(), rec->dataLength,
				//	rec->data);
	}
	{
		IpfixTemplateRecord* rec = dynamic_cast<IpfixTemplateRecord*>(ipfixRecord);
		if (rec) onTemplate(rec);
			//onTemplate(rec->sourceID.get(), rec->templateInfo.get());
	}
	{
		IpfixDataTemplateRecord* rec = dynamic_cast<IpfixDataTemplateRecord*>(ipfixRecord);
		if (rec) onDataTemplate(rec);
			//onDataTemplate(rec->sourceID.get(), rec->dataTemplateInfo.get());
	}
	{
		IpfixOptionsTemplateRecord* rec = dynamic_cast<IpfixOptionsTemplateRecord*>(ipfixRecord);
		if (rec) onOptionsTemplate(rec);
			//onOptionsTemplate(rec->sourceID.get(), rec->optionsTemplateInfo.get());
	}
	{
		IpfixTemplateDestructionRecord* rec = dynamic_cast<IpfixTemplateDestructionRecord*>(ipfixRecord);
		if (rec) onTemplateDestruction(rec);
			//onTemplateDestruction(rec->sourceID.get(), rec->templateInfo.get());
	}
	{
		IpfixDataTemplateDestructionRecord* rec = dynamic_cast<IpfixDataTemplateDestructionRecord*>(ipfixRecord);
		if (rec) onDataTemplateDestruction(rec);
			//onDataTemplateDestruction(rec->sourceID.get(), rec->dataTemplateInfo.get());
	}
	{
		IpfixOptionsTemplateDestructionRecord* rec = dynamic_cast<IpfixOptionsTemplateDestructionRecord*>(ipfixRecord);
		if (rec) onOptionsTemplateDestruction(rec);
			//onOptionsTemplateDestruction(rec->sourceID.get(), rec->optionsTemplateInfo.get());
	}
}

/**
 * Callback function invoked when a new Template arrives.
 * @param sourceID SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 */
void IpfixRecordDestination::onTemplate(IpfixTemplateRecord* record)
{
	THROWEXCEPTION("method not implemented");
}

/**
 * Callback function invoked when a new DataTemplate arrives.
 * @param sourceID SourceID of the exporter that sent this DataTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this Template
 */
void IpfixRecordDestination::onOptionsTemplate(IpfixOptionsTemplateRecord* record)
{
	THROWEXCEPTION("method not implemented");
}

/**
 * Callback function invoked when a new DataTemplate arrives.
 * @param sourceID SourceID of the exporter that sent this DataTemplate
 * @param dataTemplateInfo Pointer to a structure defining this Template
 */
void IpfixRecordDestination::onDataTemplate(IpfixDataTemplateRecord* record)
{
	THROWEXCEPTION("method not implemented");
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
	THROWEXCEPTION("method not implemented");
}


/**
 * Callback function invoked when a new Options Record arrives.
 * @param sourceID SourceID of the exporter that sent this Record
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return 0 if packet handled successfully
 */
void IpfixRecordDestination::onOptionsRecord(IpfixOptionsRecord* record)
{
	THROWEXCEPTION("method not implemented");
}

/**
 * Callback function invoked when a new Data Record with associated Fixed Values arrives.
 * @param sourceID SourceID of the exporter that sent this Record
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixRecordDestination::onDataDataRecord(IpfixDataDataRecord* record)
{
	THROWEXCEPTION("method not implemented");
}

/**
 * Callback function invoked when a Template is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param sourceID SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 */
void IpfixRecordDestination::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
}

/**
 * Callback function invoked when a OptionsTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param sourceID SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 */
void IpfixRecordDestination::onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record)
{
}

/**
 * Callback function invoked when a DataTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param sourceID SourceID of the exporter that sent this DataTemplate
 * @param dataTemplateInfo Pointer to a structure defining this DataTemplate
 */
void IpfixRecordDestination::onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record)
{
}


