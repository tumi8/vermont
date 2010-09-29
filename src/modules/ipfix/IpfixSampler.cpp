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

#include "IpfixSampler.h"
#include "common/Misc.h"

#include <math.h>


IpfixSampler::IpfixSampler(double flowrate)
	: flowRate(flowrate),
	  counter(0),
	  statDropped(0),
	  statTotalDropped(0)
{
	modulo = (uint64_t)round(1.0/flowRate);

	msg(MSG_INFO, "IpfixSampler started with following parameters:");
	msg(MSG_INFO, "  - flowRate=%f", flowRate);
	msg(MSG_INFO, "  - resulting modulo: %llu", modulo);
}

IpfixSampler::~IpfixSampler()
{
}

void IpfixSampler::onDataRecord(IpfixDataRecord* record)
{
	// do not sample Options Data Records
	if((record->templateInfo->setId == TemplateInfo::NetflowOptionsTemplate) || (record->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate)) {
		send(record);
	} else {
		counter++;
		if ((counter%modulo)==0) {
			send(record);
		} else {
			statDropped++;
			statTotalDropped++;
			record->removeReference();
		}
	}
}



string IpfixSampler::getStatisticsXML(double interval)
{
	ostringstream oss;
	uint64_t dropped = statDropped;
	statDropped -= dropped;
	oss << "<counter>" << counter << "</counter>";
	oss << "<dropped>" << dropped << "</dropped>";
	oss << "<totalDropped>" << statTotalDropped << "</totalDropped>";
	return oss.str();
}


