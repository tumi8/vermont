/*
 * VERMONT
 * Copyright (C) 2009 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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

#if !defined(FRONTPAYLOADSIGMATCHER_H_)
#define FRONTPAYLOADSIGMATCHER_H_

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

extern "C" {
	#include "fpsigmatcher/Matcher.h"
}

#include <list>
#include <string>

using namespace std;

class FrontPayloadSigMatcher
	: public Module,
	  public IpfixRecordDestination,
	  public Source<IpfixRecord*>
{
	public:
		FrontPayloadSigMatcher(string sigdir);
		virtual ~FrontPayloadSigMatcher();

		virtual void onDataRecord(IpfixDataRecord* record);

	private:
		string signatureDir;
		Matcher* sigmatcher;

		void matchConnection(Connection* conn);
		virtual string getStatisticsXML(double interval);
};

#endif
