// Utilities.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"
#include <datetime.h>   // for PyDateTime_IMPORT

char*
StringArrayCat(char** sStrings, int iElements)
{
	int iCharacters = 1;
	for (int i = 0; i < iElements; i++) {
		iCharacters += strlen(sStrings[i]);
	}

	char* result = (char*)PyMem_RawMalloc(sizeof(char) * iCharacters);

	strcpy(result, sStrings[0]);
	for (int i = 1; i < iElements; i++) {
		strcat(result, sStrings[i]);
	}
	return result;
}

char*
StringAppend(char* sMain, char* sAppex)
// sMain must be on the heap!
{
	size_t nLen;
	if (sAppex == NULL || sAppex[0] == '\0')
		return sMain;

	if (sMain == NULL) {
		nLen = strlen(sAppex) + 1;
		sMain = (char*)PyMem_RawMalloc(nLen);
		strcpy(sMain, sAppex);
	}
	else {
		nLen = strlen(sMain) + strlen(sAppex) + 1;
		sMain = (char*)PyMem_RawRealloc(sMain, nLen);
		strcat(sMain, sAppex);
	}
	return sMain;
}

char*
StringAppend2(char* sMain, char* sAppex1, char* sAppex2)
// sMain must be on the heap!
{
	if ((sAppex1 == NULL || sAppex1[0] == '\0') && (sAppex2 == NULL || sAppex2[0] == '\0'))
		return sMain;

	size_t nLen = (sMain == NULL ? 0 : strlen(sMain)) + (sAppex1 == NULL ? 0 : strlen(sAppex1)) + (sAppex2 == NULL ? 0 : strlen(sAppex2)) + 1;
	sMain = (sMain == NULL) ? (char*)PyMem_RawMalloc(nLen) : (char*)PyMem_RawRealloc(sMain, nLen);
	if (sAppex1 != NULL)
		strcat(sMain, sAppex1);
	if (sAppex2 != NULL)
		strcat(sMain, sAppex2);

	return sMain;
}

PyObject* // new ref
PxFormatData(PyObject* pyData, PyObject* pyFormat)
{
	PyObject* pyText = NULL;
	if (pyData == NULL || pyData == Py_None) {
		pyText = PyUnicode_New(0, 0);
		return pyText;
	}

	if (PyUnicode_Check(pyData)) {
		pyText = pyData;
		Py_INCREF(pyText);
	}

	PyDateTime_IMPORT;
	if (PyDateTime_Check(pyData)) {
		if (pyFormat && pyFormat != Py_None) {
			if (!(pyText = PyObject_CallMethod(pyFormat, "format", "(O)", pyData))) {
				return NULL;
			}
		}
		else
			if (!(pyText = PyObject_CallMethod(g.pyStdDateTimeFormat, "format", "(O)", pyData)))
				return NULL;
	}
	else if (PyLong_Check(pyData) || PyFloat_Check(pyData)) {
		if (pyFormat && pyFormat != Py_None) {
			if (!(pyText = PyObject_CallMethod(pyFormat, "format", "(O)", pyData))) {
				return NULL;
			}
		}
		else
			if (!(pyText = PyObject_Str(pyData)))
				return NULL;
	}

	if (pyText == NULL) {
		PyErr_Format(PyExc_TypeError, "This data type can not be formatted: '%s'.", PyUnicode_AsUTF8(PyObject_Repr((PyObject*)pyData->ob_type)));
		return NULL;
	}

	return pyText;
}

PyObject* // new ref
PxParseString(char* sText, PyTypeObject* pyDataType, PyObject* pyFormat)
{
	PyObject* pyData = NULL, *pyString;
	PyDateTime_IMPORT;
	if (pyDataType == &PyUnicode_Type) {
		pyData = PyUnicode_FromString(sText);
	}
	else if (pyDataType == &PyLong_Type) {
		pyData = PyLong_FromString(sText, NULL, 0);
	}
	else if (pyDataType == &PyFloat_Type) {
		pyString = PyUnicode_FromString(sText);
		pyData = PyFloat_FromString(pyString);
		Py_XDECREF(pyString);
	}
	else if (pyDataType == PyDateTimeAPI->DateTimeType) {
		pyData = PyObject_CallMethod((PyObject*)pyDataType, "strptime", "ss", sText, "%Y-%m-%d");
	}
	else {
		pyString = PyObject_Repr((PyObject*)pyDataType);
		PyErr_Format(PyExc_TypeError, "Unsupported data type in Edit: '%s'.", PyUnicode_AsUTF8(pyString));
		Py_XDECREF(pyString);
	}
	return pyData;
}

PyTupleObject* // new ref
PyTuple_Duplicate(PyTupleObject* pyTuple)
// see: static PyObject *tupleslice(PyTupleObject *a, Py_ssize_t ilow, Py_ssize_t ihigh) in: cpython/Objects/tupleobject.c
{
	PyTupleObject* pyNewTuple;
	PyObject** pySrc, **pyDest;
	PyObject* pyItem;
	Py_ssize_t nIndex, nLen = PyTuple_Size(pyTuple);

	pyNewTuple = (PyTupleObject*)PyTuple_New(nLen);
	pySrc = pyTuple->ob_item;
	pyDest = pyNewTuple->ob_item;
	for (nIndex = 0; nIndex < nLen; nIndex++) {
		pyItem = pySrc[nIndex];
		Py_INCREF(pyItem);
		pyDest[nIndex] = pyItem;
	}
	return pyNewTuple;
}

void
ErrorDialog(char* sMessage)
{
	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", sMessage);
	gtk_dialog_run(GTK_DIALOG(gtkDialog));
	gtk_widget_destroy(gtkDialog);
}

void
PythonErrorDialog()
{
	//PyErr_Print();
	PyObject *pyType, *pyValue, *pyTraceback;
	PyErr_Fetch(&pyType, &pyValue, &pyTraceback);
	if (pyValue){
	char* sMessage = PyUnicode_AsUTF8(pyValue);

	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow,
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		"Error in Python script:\n\n≫%s≪", sMessage);
	gtk_dialog_run(GTK_DIALOG(gtkDialog));
	gtk_widget_destroy(gtkDialog);}
}

bool
PxAttachObject(PyObject** ppyMember, PyObject* pyObject, bool bStrong)
{
	PyObject* tmp = *ppyMember;
	if (bStrong)
		Py_INCREF(pyObject);
	*ppyMember = pyObject;
	Py_XDECREF(tmp);
	return true;
}

void
XX(PyObject* pyObject)
{
	if (pyObject) {
		PyObject* pyRepresentation = PyObject_Repr(pyObject);
		const char* sRepresentation = PyUnicode_AsUTF8(pyRepresentation);
		g_debug("PyObject_Repr %s Ref Count: %d", sRepresentation, pyObject->ob_refcnt);
		Py_DECREF(pyRepresentation);
	}
	else {
		g_debug("NULL");
	}
}

void
Xx(char* sText, PyObject* pyObject)
{
	if (pyObject) {
		PyObject* pyRepresentation = PyObject_Repr(pyObject);
		const char* sRepresentation = PyUnicode_AsUTF8(pyRepresentation);
		g_debug("%s | %s Ref Count: %d", sText, sRepresentation, pyObject->ob_refcnt);
		Py_DECREF(pyRepresentation);
	}
	else {
		g_debug("NULL");
	}
}
