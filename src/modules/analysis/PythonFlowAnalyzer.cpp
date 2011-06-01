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
	pyClass = PyDict_GetItemString(pyDict, "FlowRecord");
	if (!pyClass) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code (3.5)");
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

	enum TimeIdentifier { NONE = 1, SEC = 1, mSEC = 2, nSEC = 3 };
	TimeIdentifier flowstart = NONE;
	TimeIdentifier flowend = NONE;
	TimeIdentifier revflowstart = NONE;
	TimeIdentifier revflowend = NONE;

	PyObject* pydict = PyDict_New();
	for (uint32_t i = 0; i < record->templateInfo->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &record->templateInfo->fieldInfo[i];
		if (fi->type.enterprise==0) {
			uint64_t time;
			switch (fi->type.id) {
				// TODO: call Py_DECREF for each passed value to PyDict_SetItemString
				case IPFIX_TYPEID_sourceIPv4Address:
					PyDict_SetItemString(pydict, "sourceIPv4Address",
							PyLong_FromLong(ntohl(*(uint32_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_destinationIPv4Address:
					PyDict_SetItemString(pydict, "destinationIPv4Address",
							PyLong_FromLong(ntohl(*(uint32_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_sourceTransportPort:
					PyDict_SetItemString(pydict, "sourceTransportPort",
							PyLong_FromLong(ntohs(*(uint16_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_destinationTransportPort:
					PyDict_SetItemString(pydict, "destinationTransportPort",
							PyLong_FromLong(ntohs(*(uint16_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_protocolIdentifier:
					PyDict_SetItemString(pydict, "protocolIdentifier",
							PyLong_FromLong(*(uint8_t*)(record->data + fi->offset)));
					break;
				case IPFIX_TYPEID_flowStartNanoSeconds:
					flowstart = nSEC;
					convertNtp64(*(uint64_t*)(record->data + fi->offset), time);
					PyDict_SetItemString(pydict, "flowStartMilliSeconds", PyLong_FromUnsignedLongLong(time));
					break;
				case IPFIX_TYPEID_flowStartMilliSeconds:
					if (flowstart>=mSEC) break;
					flowstart = mSEC;
					PyDict_SetItemString(pydict, "flowStartMilliSeconds",
							PyLong_FromUnsignedLongLong(ntohll(*(uint64_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_flowStartSeconds:
					if (flowstart>=SEC) break;
					flowstart = SEC;
					PyDict_SetItemString(pydict, "flowStartMilliSeconds",
							PyLong_FromLong(ntohl(*(uint32_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_flowEndNanoSeconds:
					flowend = nSEC;
					convertNtp64(*(uint64_t*)(record->data + fi->offset), time);
					PyDict_SetItemString(pydict, "flowEndMilliSeconds", PyLong_FromUnsignedLongLong(time));
					break;
				case IPFIX_TYPEID_flowEndMilliSeconds:
					if (flowend>=mSEC) break;
					flowend = mSEC;
					PyDict_SetItemString(pydict, "flowEndMilliSeconds",
							PyLong_FromUnsignedLongLong(ntohll(*(uint64_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_flowEndSeconds:
					if (flowend>=SEC) break;
					flowend = SEC;
					PyDict_SetItemString(pydict, "flowEndMilliSeconds",
							PyLong_FromLong(ntohl(*(uint32_t*)(record->data + fi->offset))));
					break;
			}
		} else if (fi->type.enterprise==IPFIX_PEN_reverse) {
			uint64_t time;
			switch (fi->type.id) {
				case IPFIX_TYPEID_flowStartNanoSeconds:
					revflowstart = nSEC;
					convertNtp64(*(uint64_t*)(record->data + fi->offset), time);
					PyDict_SetItemString(pydict, "revFlowStartMilliSeconds", PyLong_FromUnsignedLongLong(time));
					break;
				case IPFIX_TYPEID_flowStartMilliSeconds:
					if (revflowstart>=mSEC) break;
					revflowstart = mSEC;
					PyDict_SetItemString(pydict, "revFlowStartMilliSeconds",
							PyLong_FromUnsignedLongLong(ntohll(*(uint64_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_flowStartSeconds:
					if (revflowstart>=SEC) break;
					revflowstart = SEC;
					PyDict_SetItemString(pydict, "revFlowStartMilliSeconds",
							PyLong_FromLong(ntohl(*(uint32_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_flowEndNanoSeconds:
					revflowend = nSEC;
					convertNtp64(*(uint64_t*)(record->data + fi->offset), time);
					PyDict_SetItemString(pydict, "revFlowEndMilliSeconds", PyLong_FromUnsignedLongLong(time));
					break;
				case IPFIX_TYPEID_flowEndMilliSeconds:
					if (revflowend>=mSEC) break;
					revflowend = mSEC;
					PyDict_SetItemString(pydict, "revFlowEndMilliSeconds",
							PyLong_FromUnsignedLongLong(ntohll(*(uint64_t*)(record->data + fi->offset))));
					break;
				case IPFIX_TYPEID_flowEndSeconds:
					if (revflowend>=SEC) break;
					revflowend = SEC;
					PyDict_SetItemString(pydict, "revFlowEndMilliSeconds",
							PyLong_FromLong(ntohl(*(uint32_t*)(record->data + fi->offset))));
					break;
			}

		}
	}



	/*char buf[100];
	snprintf(buf, ARRAY_SIZE(buf), "FlowRecord(%u, %u, %u, %u)\n", ntohl(conn.srcIP), ntohl(conn.dstIP), ntohs(conn.srcPort), ntohs(conn.dstPort));
	PyObject* fr = PyRun_String(buf, Py_eval_input, pyDict, pyDict);
	if (!fr) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute given Python code, function 'onDataRecord' not found");
	}*/
	PyObject* t = PyTuple_New(1);
	PyTuple_SetItem(t, 0, pydict);
	PyObject* instance = PyObject_CallObject(pyClass, t);
	if (!instance) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to instantiate class 'FlowRecord'");
	}
	Py_DECREF(t);
	t = PyTuple_New(1);
	PyTuple_SetItem(t, 0, instance);
	PyObject* r = PyObject_CallObject(pyFunc, t);
	if (!r) {
		PyErr_Print();
		THROWEXCEPTION("PythonFlowAnalyzer: failed to execute function 'onDataRecord'");
	}
	Py_DECREF(t);
	Py_DECREF(r);

	record->removeReference();
}
