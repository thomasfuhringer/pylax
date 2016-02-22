// ComboBoxObject.c  | Pylax © 2015 by Thomas Führinger
#include "Pylax.h"

static LRESULT CALLBACK PxComboBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static PyObject*
PxComboBox_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxComboBoxObject* self = (PxComboBoxObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyItems = NULL;
		self->bNoneSelectable = TRUE;
		return (PyObject*)self;
	}
	else
		return NULL;
}

//extern PyTypeObject PxWidgetType;

//static BOOL PxComboBox_SetLabel(PxComboBoxObject* self, PyObject* pyLabel);

static int
PxComboBox_init(PxComboBoxObject *self, PyObject *args, PyObject *kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);

	int iAlignment = 0;
	if (self->pyDataType != &PyUnicode_Type)
		iAlignment = ES_RIGHT;

	self->hWin = CreateWindowExW(WS_EX_CLIENTEDGE, WC_COMBOBOX, L"",
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_TABSTOP | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | iAlignment,
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXCOMBOBOX, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	self->fnOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxComboBoxProc);
	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));

	//SendMessage(self->hWin, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)L"Bla bla");
	//SendMessage(self->hWin, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);

	if ((self->pyItems = PyList_New(0)) == NULL)
		return -1;

	return 0;
}

static PyObject*
PxComboBox_append(PxComboBoxObject* self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "value", "key", NULL };
	PyObject* pyValue = NULL, *pyKey = NULL, *pyItem = NULL, *pyRepr = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
		&pyValue,
		&pyKey))
		return NULL;

	if (pyKey == NULL)
		pyKey = pyValue;

	pyItem = PyTuple_Pack(2, pyValue, pyKey);
	if (PyList_Append(self->pyItems, pyItem) == -1)
		return NULL;

	if (PyUnicode_Check(pyValue)) {
		pyRepr = pyValue;
		Py_INCREF(pyRepr);
	}
	else
		pyRepr = PyObject_Repr(pyValue);

	LPCSTR strText = PyUnicode_AsUTF8(pyRepr);
	LPWSTR szText = toW(strText);
	SendMessage(self->hWin, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szText);
	PyMem_RawFree(szText);
	Py_DECREF(pyRepr);

	Py_RETURN_NONE;
}

BOOL
PxComboBox_Selected(PxComboBoxObject* self, int iItem)
{
	//MessageBox(0, L"PxComboBoxObject", TEXT("Item Selected"), MB_OK);  
	PyObject* pyItem, *pyData;
	if (iItem == CB_ERR)
		pyData = Py_None;
	else {
		pyItem = PyList_GetItem(self->pyItems, iItem);
		pyData = PyTuple_GET_ITEM(pyItem, 1);
	}

	PyObject* tmp = self->pyData;
	Py_INCREF(pyData);
	self->pyData = pyData;
	Py_XDECREF(tmp);

	return TRUE;
}

/*
BOOL
PxComboBox_RenderData(PxComboBoxObject* self, BOOL bFormat)
{
//PyObject_ShowRepr(self->pyFormat);
PyObject* pyText = PxFormatData(self->pyData, bFormat ? self->pyFormat : (self->pyFormatEdit ? self->pyFormatEdit : Py_None));
if (pyText == NULL) {
return FALSE;
}

LPCSTR strText = PyUnicode_AsUTF8(pyText);
LPWSTR szText = toW(strText);
if (SendMessage(self->hWin, WM_SETTEXT, 0, (LPARAM)szText)) {
//HFree(szText);
PyMem_RawFree(szText);
return TRUE;
}
else {
//HFree(szText);
PyMem_RawFree(szText);
PyErr_SetFromWindowsErr(0);
return FALSE;
}
}*/

BOOL
PxComboBox_SetData(PxComboBoxObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return TRUE;

	Py_ssize_t nIndex;
	if (self->bNoneSelectable && pyData == Py_None)
		nIndex = -1;
	else {
		Py_ssize_t nLen = PySequence_Size(self->pyItems);
		BOOL bFound = FALSE;
		PyObject* pyItem, *pyKey;
		for (nIndex = 0; nIndex < nLen; nIndex++) {
			pyItem = PyList_GetItem(self->pyItems, nIndex);
			pyKey = PyTuple_GET_ITEM(pyItem, 1);
			if (PyObject_RichCompareBool(pyKey, pyData, Py_EQ)) {
				bFound = TRUE;
				break;
			}
		}

		if (!bFound) {
			PyObject* pyRepr = PyObject_Repr(pyData);
			PyErr_Format(PyExc_AttributeError, "Value %s is not among selectable items in ComboBox.", PyUnicode_AsUTF8(pyRepr));
			Py_XDECREF(pyRepr);
			return FALSE;
		}
	}

	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return FALSE;

	SendMessage(self->hWin, CB_SETCURSEL, (WPARAM)nIndex, (LPARAM)0);
	return TRUE;
}

static PyObject *
PxComboBox_refresh(PxComboBoxObject* self)
{
	PyObject *pyData = PxWidget_PullData((PxWidgetObject*)self);
	if (pyData) {
		if (PxComboBox_SetData(self, pyData))
			Py_RETURN_TRUE;
		else
			return NULL;
	}
	Py_RETURN_FALSE;
}
/*
BOOL
PxComboBox_Entering(PxComboBoxObject* self)
{
return PxComboBox_RenderData(self, FALSE);
}

BOOL
PxComboBox_Changed(PxComboBoxObject* self)
{
//MessageBox(NULL, L"Changed!", L"Error", MB_ICONERROR);
if (self->pyDataSet)
PxDataSet_Freeze(self->pyDataSet);
self->bDirty = TRUE;
return TRUE;// MessageBox(NULL, L"Changed!", L"Error", MB_ICONERROR);
}

BOOL
PxComboBox_Left(PxComboBoxObject* self)
{
//PyObject_ShowRepr(self);// pyData->ob_type);
if (self->bDirty) {
PyObject* pyData, *pyString;
TCHAR szText[1024];
GetWindowText(self->hWin, szText, 1024);
LPCSTR strText = toU8(szText);
PyDateTime_IMPORT;

if (self->pyDataType == &PyUnicode_Type){
pyData = PyUnicode_FromString(strText);
}
else if (self->pyDataType == &PyLong_Type) {
pyData = PyLong_FromString(strText, NULL, 0);
}
else if (self->pyDataType == &PyFloat_Type) {
pyString = PyUnicode_FromString(strText);
pyData = PyFloat_FromString(pyString);
Py_XDECREF(pyString);
}
else if (self->pyDataType == PyDateTimeAPI->DateTimeType) {
//pyData = PyObject_CallMethod(PyDateTimeAPI->DateTimeType, "strptime", "(sO)", strText, PyUnicode_FromString("%Y-%m-%d"));
DATE date;
SYSTEMTIME sysTime;
HRESULT hr = VarDateFromStr(szText, NULL, LOCALE_NOUSEROVERRIDE, &date);
if (hr != S_OK) {
PyErr_Format(PyExc_TypeError, "Can not convert to date/time: '%s'.", strText);
goto error;
}
VariantTimeToSystemTime(date, &sysTime);
pyData = PyDateTime_FromDateAndTime(sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, 0);
if (!pyData)
goto error;
}
else {
PyErr_Format(PyExc_TypeError, "Unsupported data type in Edit: '%s'.", PyUnicode_AsUTF8(PyObject_Repr(&PyDateTimeAPI->DateTimeType)));
goto error;
}
PyMem_RawFree(strText);
if (!PxComboBox_SetData(self, pyData))
goto error;
if (!PxWidget_PostData((PxWidgetObject*)self, self->pyData))
goto error;
self->bDirty = FALSE;
PyMem_RawFree(strText);
}
else
return PxComboBox_RenderData(self, TRUE);
error:
return FALSE;
//MessageBox(NULL, L"Got left!", L"Error", MB_ICONERROR);
}
*/

static int
PxComboBox_setattro(PxComboBoxObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		/*
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "label") == 0) {
		return PxComboBox_SetLabel((PxComboBoxObject*)self, pyValue) ? 0 : -1;
		}*/
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			if (!PxComboBox_SetData(self, pyValue))
				return -1;
			return 0;
		}
	}
	return PyObject_GenericSetAttr((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxComboBox_getattro(PxComboBoxObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) 	{
			PyErr_Clear();
			Py_INCREF(self->pyData);
			return self->pyData;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
	//return pyResult;
}

static void
PxComboBox_dealloc(PxComboBoxObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxComboBox_members[] = {
	{ "noneSelectable", T_BOOL, offsetof(PxComboBoxObject, bNoneSelectable), 0, "It is possible to make no selection." },
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxComboBox_methods[] = {
	{ "refresh", (PyCFunction)PxComboBox_refresh, METH_NOARGS, "Pull fresh data" },
	{ "append", (PyCFunction)PxComboBox_append, METH_VARARGS | METH_KEYWORDS, "Append selectable item." },
	{ NULL }  /* Sentinel */
};

PyTypeObject PxComboBoxType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.ComboBox",          /* tp_name */
	sizeof(PxComboBoxObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxComboBox_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	PxComboBox_getattro,       /* tp_getattro */
	PxComboBox_setattro,       /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"ComboBox objects",        /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxComboBox_methods,        /* tp_methods */
	PxComboBox_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxComboBox_init, /* tp_init */
	0,                         /* tp_alloc */
	PxComboBox_new,            /* tp_new */
};

static LRESULT CALLBACK
PxComboBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PxComboBoxObject* self = (PxComboBoxObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_SETFOCUS:
		if (PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)self) == -1) {
			PyErr_Print();
			MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
		}
		return 0;

	case OCM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			if (!PxComboBox_Selected(self, ItemIndex)) {
				PyErr_Print();
				MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
			}
			return 0;
		}
		break;

	default:
		break;
	}
	return CallWindowProcW(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
}