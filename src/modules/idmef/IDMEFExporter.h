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

#if !defined(IDMEFEXPORTER_H)
#define IDMEFEXPORTER_H

#include "common/Mutex.h"
#include "IDMEFMessage.h"
#include "core/Module.h"

#include <string>

using namespace std;

/**
 * this class is used for exporting IDMEF messages
 * the received IDMEF message is stored in the destination directory with a generated filename
 */
class IDMEFExporter
	: public Module,
	  public Destination<IDMEFMessage*>,
	  public Source<NullEmitable*>
{
	public:
		IDMEFExporter(const string destdir, const string sendurl);
		virtual ~IDMEFExporter();

		virtual void receive(IDMEFMessage* m);

	private:
		string templateFilename;
		string destinationDir;
		string sendURL;
		string hostname;
		string ipaddress;
		string analyzerId;

		static uint32_t filenameCounter;
		static Mutex mutex;
		static bool filewarningIssued;

		string getFilename();
		void exportMessage(const string idmefmsg);
};

#endif
