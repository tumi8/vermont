/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#ifndef INCLUDE_FlowSink_hpp
#define INCLUDE_FlowSink_hpp

#include <stdint.h>
#include <memory>
#include <stdexcept>
#include <boost/smart_ptr.hpp>
#include "IpfixRecord.hpp"
#include "../sampler/Thread.h"
#include "../sampler/ConcurrentQueue.h"

/*
 * IPFIX Flow Sink class
 *
 * The IPFIX Flow Sink class servers as a base class for all modules 
 * which can receive and act upon IPFIX flows.
 */
class FlowSink {

	public:
		FlowSink();
		virtual ~FlowSink();

		/**
		 * Push an IpfixRecord into the queue for later pickup by the FlowSink's thread
		 */
		void push(boost::shared_ptr<IpfixRecord> ipfixRecord);

		/**
		 * Start the FlowSink's flowSinkProcess thread
		 */
		bool runSink();

		/**
		 * Stop the FlowSink's flowSinkProcess thread
		 */
		bool terminateSink();

	protected:
		// Allow HookingFilter to directly call FlowSink::onDataRecord
		friend class HookingFilter;

		/**
		 * Callback function invoked when a new Template arrives.
		 * @param sourceID SourceID of the exporter that sent this Template
		 * @param templateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) { return -1; };

		/**
		 * Callback function invoked when a new DataTemplate arrives.
		 * @param sourceID SourceID of the exporter that sent this DataTemplate
		 * @param optionsTemplateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onOptionsTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) { return -1; };

		/**
		 * Callback function invoked when a new DataTemplate arrives.
		 * @param sourceID SourceID of the exporter that sent this DataTemplate
		 * @param dataTemplateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo) { return -1; };

		/**
		 * Callback function invoked when a new Data Record arrives.
		 * @param sourceID SourceID of the exporter that sent this Record
		 * @param templateInfo Pointer to a structure defining the Template used
		 * @param length Length of the data block supplied
		 * @param data Pointer to a data block containing all fields
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data) { return -1; };

		/**
		 * Callback function invoked when a new Options Record arrives.
		 * @param sourceID SourceID of the exporter that sent this Record
		 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
		 * @param length Length of the data block supplied
		 * @param data Pointer to a data block containing all fields
		 * @return 0 if packet handled successfully
		 */
		virtual int onOptionsRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, IpfixRecord::Data* data) { return -1; };

		/**
		 * Callback function invoked when a new Data Record with associated Fixed Values arrives.
		 * @param sourceID SourceID of the exporter that sent this Record
		 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
		 * @param length Length of the data block supplied
		 * @param data Pointer to a data block containing all variable fields
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data) { return -1; };

		/**
		 * Callback function invoked when a Template is being destroyed.
		 * Particularly useful for cleaning up userData associated with this Template
		 * @param sourceID SourceID of the exporter that sent this Template
		 * @param templateInfo Pointer to a structure defining this Template
		 * @return 0 if packet handled successfully
		 */
		virtual int onTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) { return -1; };

		/**
		 * Callback function invoked when a OptionsTemplate is being destroyed.
		 * Particularly useful for cleaning up userData associated with this Template
		 * @param sourceID SourceID of the exporter that sent this OptionsTemplate
		 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
		 * @return 0 if packet handled successfully
		 */
		virtual int onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) { return -1; };

		/**
		 * Callback function invoked when a DataTemplate is being destroyed.
		 * Particularly useful for cleaning up userData associated with this Template
		 * @param sourceID SourceID of the exporter that sent this DataTemplate
		 * @param dataTemplateInfo Pointer to a structure defining this DataTemplate
		 * @return 0 if packet handled successfully
		 */
		virtual int onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo) { return -1; };

	        static void* flowSinkProcess(void* flowSink);
	        virtual void flowSinkProcess();

		ConcurrentQueue< boost::shared_ptr<IpfixRecord> > ipfixRecords;
	        Thread thread;
        	bool exitFlag;
};

#endif

