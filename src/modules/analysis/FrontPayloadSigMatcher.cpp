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

#include "FrontPayloadSigMatcher.h"
#include "common/Misc.h"

#include <math.h>



FrontPayloadSigMatcher::FrontPayloadSigMatcher(string sigdir)
	: signatureDir(sigdir),
	  sigmatcher(NULL)
{
	msg(MSG_INFO, "FrontPayloadSigMatcher started with following parameters:");
	msg(MSG_INFO, "  - signature directory=%s", sigdir.c_str());

	sigmatcher = new_matcher(sigdir.c_str());

	msg(MSG_INFO, "Loaded %d signature classes", sigmatcher->numOfClasses);
}

FrontPayloadSigMatcher::~FrontPayloadSigMatcher()
{
	if (sigmatcher) destruct_matcher(sigmatcher);
}

void FrontPayloadSigMatcher::matchConnection(Connection* conn)
{
	int* results = match_single_flow(sigmatcher, conn->srcPayload, conn->srcPayloadLen, conn->dstPayload, conn->dstPayloadLen);

	for (int32_t j=0; j<sigmatcher->numOfClasses; j++) {
		if (results[j]==1) {
			msg(MSG_DIALOG, "SIGMATCHER: front payload matches signature '%s'", sigmatcher->signatures[j]->id);
			msg(MSG_DIALOG, "%s", conn->toString().c_str());
		}
	}

	free(results);
}

void FrontPayloadSigMatcher::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	Connection conn(record);
	conn.swapIfNeeded();

	matchConnection(&conn);

	record->removeReference();
}



string FrontPayloadSigMatcher::getStatisticsXML(double interval)
{
	ostringstream oss;
	//oss << "<counter>" << counter << "</counter>";
	return oss.str();
}


