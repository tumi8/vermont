/*
 * VERMONT
 * Copyright (C) 2010 Tobias Limmer <limmer@cs.fau.de>
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

#ifndef PYTHONFLOWANALYZER_H_
#define PYTHONFLOWANALYZER_H_

#include <Python.h>
#include <time.h>
#include <map>

#include "modules/ipfix/IpfixRecordDestination.h"



/**
 * Execute a Python program and feed all flows into this program
 */
class PythonFlowAnalyzer : public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
public:
	PythonFlowAnalyzer(string code);
	virtual void onDataRecord(IpfixDataRecord* record);

protected:
	virtual void performStart();
	virtual void performShutdown();

private:
	string code;
	PyObject* pyClass;
	PyObject* pyFunc;
	PyObject* pyDict;
};



#endif /* PythonFlowAnalyzer_H_ */
