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


#include "PythonFlowAnalyzer.h"
#include "modules/ipfix/Connection.h"
#include "common/Misc.h"
#include "common/Time.h"

#include <netdb.h>


/**
 * @param ipSubnet subnet of regarded hosts
 * @param ipMask subnet mask of regarded hosts
 * @param intervalLength length of the interval in milliseconds
 */
PythonFlowAnalyzer::PythonFlowAnalyzer(string code)
	: code(code)
{

}

void PythonFlowAnalyzer::performStart()
{
	Py_Initialize();
	PyObject* m = PyImport_AddModule("__main__");
	if (!m) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (1)");
	}
	PyRun_SimpleString("import sys; sys.path.append('.')\n");
	msg(MSG_ERROR, "PYTHONPATH=%s", Py_GetPath());
	PyObject* pymodule = PyImport_ImportModule("PythonFlowAnalyzer");
	if (!pymodule) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (2.5)");
	}
	pyDict = PyModule_GetDict(pymodule);
	if (!pyDict) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (2)");
	}
	PyObject* env = PyRun_String(code.c_str(), Py_file_input, pyDict, pyDict);
	if (!env) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (3)");
	}
	PyObject* pyclass = PyDict_GetItemString(pyDict, "FlowAnalyzer");
	if (!pyclass) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (4)");
	}
	PyObject* pyinstance = PyObject_CallObject(pyclass, NULL);
	if (!pyinstance) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (5)");
	}
	//PyObject_Print(pyinstance, stdout, 0);
	pyFunc = PyObject_GetAttrString(pyinstance, "onDataRecord");
	if (!pyFunc) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code, function 'onDataRecord' not found");
	}
	if (!PyCallable_Check(pyFunc)) {
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code, function 'onDataRecord' is not callable");
	}
}

void PythonFlowAnalyzer::performShutdown()
{
	Py_Finalize();
}


void PythonFlowAnalyzer::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	Connection conn(record);

	char buf[100];
	snprintf(buf, ARRAY_SIZE(buf), "FlowRecord(%u, %u, %u, %u)\n", ntohl(conn.srcIP), ntohl(conn.dstIP), ntohs(conn.srcPort), ntohs(conn.dstPort));
	PyObject* fr = PyRun_String(buf, Py_eval_input, pyDict, pyDict);
	if (!fr) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code, function 'onDataRecord' not found");
	}
	PyObject* t = PyTuple_New(1);
	PyTuple_SetItem(t, 0, fr);
	PyObject* r = PyObject_CallObject(pyFunc, t);
	if (!r) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute function 'onDataRecord'");
	}

	record->removeReference();
}
