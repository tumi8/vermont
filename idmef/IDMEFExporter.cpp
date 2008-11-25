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


#include "IDMEFExporter.h"

#include <sys/stat.h>
#include <cstring>

uint32_t IDMEFExporter::filenameCounter = 0;

bool IDMEFExporter::filewarningIssued = false;
Mutex IDMEFExporter::mutex;



/**
 * initializes a new instance of IDMEFExporter
 * @param destdir destination directory, where all exported IDMEF messages are to be stored
 * @param sendurl destination URL which is to be used by the perl script which processes the
 *                destination directory
 */
IDMEFExporter::IDMEFExporter(const string destdir, const string sendurl)
	: destinationDir(destdir),
	  sendURL(sendurl)
{
}

IDMEFExporter::~IDMEFExporter()
{
}


/**
 * get a filename in the destination directory which is not used
 * filenames are started with 00000000, then counting upwards until the 32 bit value is wrapped around
 * we issue a warning, if files are already present in the directory
 */
string IDMEFExporter::getFilename()
{
	struct stat s;
	char number[100];
	snprintf(number, 100, "%010d", filenameCounter);
	string filename = destinationDir + "/" + number + ".xml";
	uint32_t counter = 0;
	while (stat(filename.c_str(), &s) == 0) {
		if (!filewarningIssued) {
			msg(MSG_ERROR, "files in IDMEF destination directory are already present, either two processes are writing there simultaneously (VERY BAD, may result in lost or corrupt events), or files from previous run have not been processed yet (ALSO BAD)");
			filewarningIssued = true;
		}
		if (counter == 0xFFFFFFFF) {
			THROWEXCEPTION("failed to find an accessible file in IDMEF destination directory %s", destinationDir.c_str());
		}
		filenameCounter++;
		counter++;
		snprintf(number, 100, "%010d", filenameCounter);
		filename = destinationDir + "/" + number + ".xml";
	}

	filenameCounter++;

	return filename;
}

/**
 * exports an IDMEF message
 * to do this, the internally stored IDMEF template is processed with all keys of the internal
 * parameterMap and then the file is saved in the destination directory
 * the external process which sends the IDMEF messages uses the URL given in the first line of
 * the saved message
 */
void IDMEFExporter::exportMessage(const string idmefmsg)
{
	DPRINTF("sending IDMEF message");

	string filename = getFilename();

	// save message to destination directory
	FILE* f = fopen(filename.c_str(), "w+");
	if (f == NULL) goto error;
	flockfile(f);
	// first line is URL where processing script should send event to
	if (fwrite(sendURL.c_str(), 1, sendURL.size(), f) != sendURL.size()) goto error;
	if (fwrite("\n", 1, 1, f) != 1) goto error;
	if (fwrite(idmefmsg.c_str(), 1, idmefmsg.size(), f) != idmefmsg.size()) goto error;
	if (fwrite("\n", 1, 1, f) != 1) goto error;
	funlockfile(f);
	if (fclose(f) != 0) goto error;

	mutex.unlock();

	return;

error:
	THROWEXCEPTION("failed to write to file %s, error: %s", filename.c_str(), strerror(errno));
}

void IDMEFExporter::receive(IDMEFMessage* m)
{
	exportMessage(m->getMessage());
	m->removeReference();
}
