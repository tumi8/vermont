/*
 * VERMONT
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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
 */

#if !defined(IPFIXSAMPLER_H)
#define IPFIXSAMPLER_H

#include "IpfixRecordDestination.h"
#include "Connection.h"
#include "core/Source.h"

#include <list>
#include <string>

using namespace std;

class IpfixSampler
	: public Module,
	  public IpfixRecordDestination,
	  public Source<IpfixRecord*>
{
	public:
		IpfixSampler(double flowrate);
		virtual ~IpfixSampler();

		virtual void onDataRecord(IpfixDataRecord* record);

	private:
		double flowRate;
		uint64_t counter;
		uint64_t modulo;
		uint64_t statDropped;
		uint64_t statTotalDropped;

		virtual string getStatisticsXML(double interval);
};

#endif
