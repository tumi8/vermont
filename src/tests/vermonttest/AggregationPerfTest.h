/*
 * Vermont Testsuite
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

#if !defined(AGGREGATIONPERFTEST_H)
#define AGGREGATIONPERFTEST_H

#include "modules/ipfix/aggregator/IpfixAggregator.hpp"
#include "core/InstanceManager.h"

#include "TestSuiteBase.h"

class AggregationPerfTest : public Test
{
	public:
		AggregationPerfTest(bool fast);
		~AggregationPerfTest();
		
		virtual TestResult execTest();
	private:
		static InstanceManager<Packet> packetManager;

		Rule::Field* createRuleField(const std::string& typeId);
		Rules* createRules();
		void sendPacketsTo(Destination<Packet*>* dest, uint32_t numpackets);

		int numPackets;
};

#endif
