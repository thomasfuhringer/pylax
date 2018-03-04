#include "Pylax.h"

bool
Hinterland_Init()
{
	hl.pyModule = NULL;
	hl.pyClientType = NULL;
	hl.pyMsg = NULL;
	hl.MsgClass = NULL;
	hl.Msg_Type = NULL;
	hl.Msg_Get = NULL;

	PyObject* pyModuleDict = PyImport_GetModuleDict();
	// check if script imported Hinterland and obtain pointer to class 'Client'
	hl.pyModule = PyDict_GetItemString(pyModuleDict, "hinterland");
	if (hl.pyModule) {
		hl.pyClientType = PyObject_GetAttrString(hl.pyModule, "Client");
		if (!hl.pyClientType) {
			ErrorDialog("Can not load Python type 'Client'");
			return false;
		}
		//Xx("hinterland client", g.pyHinterlandClientType);
		hl.MsgClass = PyObject_GetAttrString(hl.pyModule, "Msg");
		hl.Msg_Type = PyObject_GetAttrString(hl.MsgClass, "Type");
		hl.Msg_Get = PyObject_GetAttrString(hl.MsgClass, "Get");
	}
	return true;
}

bool
Hinterland_Exchange(PyObject* pyConnection)
{
	PyObject* pyResult;
	if ((pyResult = PyObject_CallMethod(pyConnection, "exchange", "(O)", hl.pyMsg)) == NULL) {
		g_debug("Session_Exchange failed");
		//PythonErrorDialog("Can not exchange message.");
		PyErr_Print();
		return false;
	}
	else {
		Py_DECREF(hl.pyMsg);
		bool bResult = true ? pyResult == Py_True : false;
		Py_DECREF(pyResult);
		hl.pyResultMsg = PyObject_GetAttrString(pyConnection, "msg");
		Py_DECREF(hl.pyResultMsg);    // just borrow reference
		if (bResult) {
			hl.sStatusMessage = NULL;
			return true;
		}
		else {
			PyObject* pyStatusMessage = PyObject_GetAttrString(pyConnection, "status_message");
			if (pyStatusMessage == Py_None)
				hl.sStatusMessage = NULL;
			else
				hl.sStatusMessage = PyUnicode_AsUTF8(pyStatusMessage);
			Py_DECREF(pyStatusMessage);
			return false;
		}
	}
}
