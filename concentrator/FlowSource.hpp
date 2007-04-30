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

#ifndef INCLUDED_FlowSource_hpp
#define INCLUDED_FlowSource_hpp

#include <list>
#include <boost/smart_ptr.hpp>
#include "FlowSink.hpp"
#include "IpfixRecord.hpp"

/*
 * IPFIX Flow Source class
 *
 * The IPFIX Flow Source class servers as a base class for all modules 
 * which produce an IpfixRecord and pass it to a FlowSink
 */
class FlowSource {
	public:
		FlowSource();
		virtual ~FlowSource();

		/**
		 * Add a FlowSink that receives flows we collect
		 */
		virtual void addFlowSink(FlowSink* flowSink);

		/**
		 * Push an IpfixRecord to all registered FlowSink objects
		 */
		void push(boost::shared_ptr<IpfixRecord> ipfixRecord);

	protected:
		typedef std::list<FlowSink*> FlowSinks;
		FlowSinks flowSinks; /**< List of FlowSink objects that receive flows we collect */

};

#endif
