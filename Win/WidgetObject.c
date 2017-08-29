// WidgetObject.c  | Pylax © 2015 by Thomas Führinger
#include "Pylax.h"

PxWidgetAddArgs gArgs;

static PyObject *
PxWidget_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxWidgetObject *self = (PxWidgetObject*)PxWidgetType.tp_alloc(type, 0);
	//OutputDebugString(L"\n*---- wid äpxp ");
	if (self != NULL) {
		self->hWin = 0;
		self->fnOldWinProcedure = NULL;
		self->pyParent = NULL;
		self->rc.left = CW_USEDEFAULT;
		self->rc.top = CW_USEDEFAULT;
		self->rc.right = CW_USEDEFAULT;
		self->rc.bottom = CW_USEDEFAULT;
		self->bReadOnly = FALSE;
		//self->bDirty = FALSE;
		self->bTable = FALSE;
		self->bPointer = FALSE;
		self->pyWindow = NULL;
		self->pyData = Py_None;
		Py_INCREF(Py_None);
		self->pyDataType = NULL;
		self->pyFormat = NULL;
		self->pyFormatEdit = NULL;
		self->pyDynaset = NULL;
		self->pyDataColumn = NULL;
		self->pyLabel = NULL;
		self->pyAlignHorizontal = NULL;
		self->pyAlignVertical = NULL;
		self->pyVerifyCB = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxWidget_init(PxWidgetObject* self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "parent", "left", "top", "right", "bottom", "caption", "dynaset", "column", "dataType", "format", "label", "visible", NULL };
	PyObject* pyParent = NULL, *pyCaption = NULL, *pyDataColumn = NULL, *pyFormat = NULL, *pyLabel = NULL, *tmp = NULL;
	PyTypeObject* pyDataType = NULL;
	PxDynasetObject* pyDynaset = NULL;
	gArgs.bVisible = PyObject_TypeCheck(self, &PxWindowType) ? FALSE : TRUE;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiiOOOOOOp", kwlist,
		&pyParent,
		&self->rc.left,
		&self->rc.top,
		&self->rc.right,
		&self->rc.bottom,
		&pyCaption,
		&pyDynaset,
		&pyDataColumn,
		&pyDataType,
		&pyFormat,
		&pyLabel,
		&gArgs.bVisible))
		return -1;

	if (pyParent && pyParent != Py_None) {
		if (!PyObject_TypeCheck(pyParent, &PxWidgetType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 1 ('parent') must be a widget");
			return -1;
		}
		self->pyParent = (PxWidgetObject*)pyParent;
		self->pyWindow = ((PxWidgetObject*)pyParent)->pyWindow;
		gArgs.hwndParent = ((PxWidgetObject*)pyParent)->hWin;
	}
	else
		gArgs.hwndParent = g.hWin;

	if (pyCaption) {
		if (PyUnicode_Check(pyCaption)) {
			gArgs.pyCaption = pyCaption;
		}
		else  {
			PyErr_SetString(PyExc_TypeError, "Parameter 2 ('caption') must be a string.");
			return -1;
		}
	}
	else
		gArgs.pyCaption = NULL;

	if (pyDynaset) {
		tmp = self->pyDynaset;
		if (!PyObject_TypeCheck(pyDynaset, &PxDynasetType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 3 ('dynaset') must be a Dynaset.");
			return -1;
		}
		if (pyDataColumn == NULL && !PyObject_TypeCheck(self, &PxTableType) && !PyObject_TypeCheck(self, &PxFormType) && !PyObject_TypeCheck(self, &PxWindowType)) {
			PyErr_SetString(PyExc_RuntimeError, "No data column provided.");
			return -1;
		}
		Py_INCREF(pyDynaset);
		self->pyDynaset = (PxDynasetObject*)pyDynaset;
		Py_XDECREF(tmp);
		if (!PxDynaset_AddWidget(self->pyDynaset, self))
			return -1;
	}

	tmp = self->pyDataColumn;
	if (pyDataColumn){
		if (!PyUnicode_Check(pyDataColumn)) {
			PyErr_Format(PyExc_TypeError, "Parameter 4 ('column') must be string, not '%.200s'.", pyDataColumn->ob_type->tp_name);
			return -1;
		}
		else {
			PyObject* pyColumn = PyDict_GetItem(self->pyDynaset->pyColumns, pyDataColumn);
			if (pyColumn == NULL) {
				PyErr_SetString(PyExc_TypeError, "Parameter 4 ('column') does not exist.");
				PyErr_Format(PyExc_TypeError, "Column '%.200s' does not exist in Dynaset '%.200s'.", PyUnicode_AsUTF8(pyDataColumn), PyUnicode_AsUTF8(self->pyDynaset));
				return -1;
			}
			self->pyDataColumn = pyColumn;
			self->pyDataType = (PyTypeObject*)PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_TYPE);
			self->pyFormat = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_FORMAT);
			Py_INCREF(self->pyDataColumn);
			Py_INCREF(self->pyDataType);
			Py_INCREF(self->pyFormat);
			Py_XDECREF(tmp);
		}
	}
	if (pyDataType){
		if (!PyObject_TypeCheck(pyDataType, &PyType_Type)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 5 ('dataType') must be a data type.");
			return -1;
		}
		else {
			if (self->pyDataColumn && self->pyDataType != pyDataType)  {
				PyErr_SetString(PyExc_TypeError, "Parameter 5 ('dataType') is different from bound data column.");
				return -1;
			}
		}
	}
	else
		pyDataType = &PyUnicode_Type;
	tmp = self->pyDataType;
	Py_INCREF(pyDataType);
	self->pyDataType = pyDataType;
	Py_XDECREF(tmp);

	Py_INCREF(Py_None);
	self->pyData = Py_None;

	if (pyFormat){
		if (!PyUnicode_Check(pyFormat))  {
			PyErr_SetString(PyExc_TypeError, "Parameter 6 ('format') must be a string.");
			return -1;
		}
		else {
			tmp = self->pyFormat;
			Py_INCREF(pyFormat);
			self->pyFormat = pyFormat;
			Py_XDECREF(tmp);
		}
	}
	if (!self->pyFormat){
		Py_INCREF(Py_None);
		self->pyFormat = Py_None;
	}

	if (pyLabel){
		if (!PyObject_TypeCheck(pyLabel, &PxLabelType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 7 ('label') must be a Label.");
			return -1;
		}
		else {
			tmp = self->pyLabel;
			Py_INCREF(pyLabel);
			self->pyLabel = (PxLabelObject*)pyLabel;
			Py_XDECREF(tmp);
			tmp = self->pyLabel->pyAssociatedWidget;
			Py_INCREF(self);
			self->pyLabel->pyAssociatedWidget = (PxWidgetObject*)self;
			Py_XDECREF(tmp);
		}
	}
	Py_INCREF(self); // reference for parent
	self->hBkgBrush = (BOOL)GetSysColorBrush(PxWINDOWBKGCOLOR);
	return 0;
}

static BOOL CALLBACK
CloseEnumProc(HWND hwnd, LPARAM lParam)
{
	EnumChildWindows(hwnd, CloseEnumProc, 0);
	PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (pyWidget != NULL)
		Py_DECREF(pyWidget);
	return TRUE;
}

static void
PxWidget_dealloc(PxWidgetObject* self)
{
	if (self->hWin) {
		EnumChildWindows(self->hWin, CloseEnumProc, 0);
		DestroyWindow(self->hWin);
		self->hWin = 0;
	}

	Py_XDECREF(self->pyData);
	Py_XDECREF(self->pyDataType);
	Py_XDECREF(self->pyFormat);
	Py_XDECREF(self->pyFormatEdit);
	Py_XDECREF(self->pyAlignHorizontal);
	Py_XDECREF(self->pyAlignVertical);
	Py_XDECREF(self->pyLabel);
	Py_XDECREF(self->pyDynaset);
	Py_XDECREF(self->pyDataColumn);

	Py_TYPE(self)->tp_free((PyObject*)self);
}

BOOL
PxWidget_SetCaption(PxWidgetObject* self, PyObject* pyText)
{
	LPCSTR strText = PyUnicode_AsUTF8(pyText);
	LPWSTR szText = toW(strText);
	if (SendMessage(self->hWin, WM_SETTEXT, (WPARAM)0, (LPARAM)szText)) {
		PyMem_RawFree(szText);
		return TRUE;
	}
	else {
		return TRUE; // something's not working with SendMessage
		PyErr_SetFromWindowsErr(0);
		PyMem_RawFree(szText);
		return FALSE;
	}
}

static PyObject* // new ref
PxWidget_focus_in(PxWidgetObject* self)
{
	HWND hOld = SetFocus(self->hWin);
	if (hOld == NULL) {
		PyErr_SetFromWindowsErr(0);
		return NULL;
	}
	else
		return PyLong_FromLong((long)hOld);
}

static PyObject*
PxWidget_pass(PxWidgetObject* self)
// for empty method
{
	Py_RETURN_TRUE;
}

static int
PxWidget_setattro(PxWidgetObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			return PxWidget_SetCaption(self, pyValue) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			/*
			int iState = GetWindowLong(self->hWin, GWL_STYLE);
			if (PyObject_IsTrue(pyValue))
			iState = iState | WS_VISIBLE;
			else
			iState = iState  & WS_VISIBLE;
			if (SetWindowLong(self->hWin, GWL_STYLE, iState))
			return  0;
			else {
			PyErr_SetFromWindowsErr(0);
			return -1;
			}
			*/
			ShowWindow(self->hWin, PyObject_IsTrue(pyValue) ? SW_SHOW : SW_HIDE);
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "frame") == 0) {
			LONG lStyle = GetWindowLong(self->hWin, GWL_EXSTYLE);
			if (PyObject_IsTrue(pyValue))
				lStyle |= PX_FRAME;
			else
				lStyle &= ~PX_FRAME;
			SetWindowLong(self->hWin, GWL_EXSTYLE, lStyle);
			SetWindowPos(self->hWin, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "verify") == 0) 	{
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyVerifyCB);
				self->pyVerifyCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assigned object must be callable.");
				return -1;
			}
		}
	}
	if (PyObject_GenericSetAttr((PyObject*)self, pyAttributeName, pyValue) < 0)
		return -1;
	return  0;
}

static PyObject* // new ref
PxWidget_getattro(PxWidgetObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		/*if (PyUnicode_CompareWithASCIIString(pyAttributeName, "parent") == 0) {
			PyErr_Clear();
			HWND hParent = GetParent(self->hWin);
			//ShowInts(L"P", self->hWin, hParent);
			if (hParent) {
			PxWidgetObject* pyParent = (PxWidgetObject*)GetWindowLongPtr(hParent, GWLP_USERDATA);
			if (pyParent)
			return pyParent;
			}
			Py_RETURN_NONE;
			}*/
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			PyErr_Clear();
			wchar_t szText[1024];
			GetWindowText(self->hWin, szText, 1024);
			//OutputDebugString(szText);
			//SendMessage(self->hWin, WM_GETTEXT, 100, szText);
			char* strText = toU8(szText);
			PyObject* pyText = PyUnicode_FromString(strText);
			//HFree(strText);
			PyMem_RawFree(strText);
			return pyText;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			PyErr_Clear();
			if (GetWindowLong(self->hWin, GWL_STYLE) & WS_VISIBLE)
				Py_RETURN_TRUE;
			else
				Py_RETURN_FALSE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "frame") == 0) {
			PyErr_Clear();
			LONG lStyle = GetWindowLong(self->hWin, GWL_EXSTYLE);
			if (lStyle & PX_FRAME)
				Py_RETURN_TRUE;
			else
				Py_RETURN_FALSE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "position") == 0) 	{
			PyErr_Clear();
			RECT rcWin;
			GetWindowRect(self->hWin, &rcWin);
			MapWindowPoints(HWND_DESKTOP, GetParent(self->hWin), (LPPOINT)&rcWin, 2);
			PyObject* pyRectangle = PyTuple_Pack(4, PyLong_FromLong(rcWin.left), PyLong_FromLong(rcWin.top), PyLong_FromLong(rcWin.right), PyLong_FromLong(rcWin.bottom));
			return pyRectangle;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "size") == 0) 	{
			PyErr_Clear();
			RECT rcClient;
			GetClientRect(self->hWin, &rcClient);
			PyObject* pyRectangle = PyTuple_Pack(2, PyLong_FromLong(rcClient.right), PyLong_FromLong(rcClient.bottom));
			return pyRectangle;
		}
	}
	//return pyResult;
	return PxWidgetType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static PyObject* // new ref
PxWidget_move(PxWidgetObject* self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "left", "top", "right", "bottom", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iiii", kwlist,
		&self->rc.left,
		&self->rc.top,
		&self->rc.right,
		&self->rc.bottom))
		return NULL;

	if (PxWidget_MoveWindow(self))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

PyObject* // borrowed ref
PxWidget_PullData(PxWidgetObject* self)
{
	if (self->pyDynaset && self->pyDataColumn) {
		if (self->pyDynaset->nRow == -1)
			return Py_None;//Py_RETURN_NONE;
		else
			return PxDynaset_GetData(self->pyDynaset, self->pyDynaset->nRow, self->pyDataColumn);
	}
	return NULL;
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
	//XX(pyData);
	if (PyDateTime_Check(pyData)) {
		if (pyFormat && pyFormat != Py_None) {
			if (!(pyText = PyObject_CallMethod(pyFormat, "format", "(O)", pyData))) {
				//PyErr_PrintEx(1);
				return NULL;
			}
		}
		else
			if (!(pyText = PyObject_CallMethod(g.pyStdDateTimeFormat, "format", "(O)", pyData)))
				return NULL;
	}
	else if (PyLong_Check(pyData) || PyFloat_Check(pyData)) {
		if (pyFormat && pyFormat != Py_None) {
			if (!(pyText = PyObject_CallMethod(pyFormat, "format", "(O)", pyData))){
				//PyErr_PrintEx(1);
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
PxParseString(LPCSTR strText, PyTypeObject* pyDataType, PyObject* pyFormat)
{
	PyObject* pyData = NULL, *pyString;
	PyDateTime_IMPORT;
	if (pyDataType == &PyUnicode_Type){
		pyData = PyUnicode_FromString(strText);
	}
	else if (pyDataType == &PyLong_Type) {
		//pyString = PyUnicode_FromString(strText);
		pyData = PyLong_FromString(strText, NULL, 0);
		//Py_XDECREF(pyString);
	}
	else if (pyDataType == &PyFloat_Type) {
		pyString = PyUnicode_FromString(strText);
		pyData = PyFloat_FromString(pyString);
		Py_XDECREF(pyString);
	}
	else if (pyDataType == PyDateTimeAPI->DateTimeType) {
		pyData = PyObject_CallMethod((PyObject*)pyDataType, "strptime", "ss", strText, "%Y-%m-%d");
		/*
		DATE date;
		SYSTEMTIME sysTime;
		HRESULT hr = VarDateFromStr(szText, NULL, LOCALE_NOUSEROVERRIDE, &date);
		if (hr != S_OK) {
		PyErr_Format(PyExc_TypeError, "Can not convert to date/time: '%s'.", strText);
		goto error;
		}
		VariantTimeToSystemTime(date, &sysTime);
		pyData = PyDateTime_FromDateAndTime(sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, 0);
		*/
	}
	else {
		pyString = PyObject_Repr((PyObject*)pyDataType);
		PyErr_Format(PyExc_TypeError, "Unsupported data type in Edit: '%s'.", PyUnicode_AsUTF8(pyString));
		Py_XDECREF(pyString);
	}
	return pyData;
}

BOOL
PxWidget_SetData(PxWidgetObject* self, PyObject* pyData)
{
	if (PyObject_RichCompareBool(self->pyData, pyData, Py_EQ))
		return TRUE;

	/*PyObject* tmp = self->pyData;
	Py_INCREF(pyData);
	self->pyData = pyData;
	Py_XDECREF(tmp);*/
	PxAttachObject(&self->pyData, pyData, TRUE);
	OutputDebugString(self->pyDynaset && self->pyDynaset->bBroadcasting ? L"\n*---- PxWidget_SetData Broadcastin" : L"\n*---- PxWidget_SetData No BC");

	if (self->pyDynaset && !self->pyDynaset->bBroadcasting && self->pyDataColumn && self->pyDynaset->nRow != -1) {
		OutputDebugString(L"\n*---- PxWidget_SetData BCO");
		if (!PxDynaset_SetData(self->pyDynaset, self->pyDynaset->nRow, self->pyDataColumn, self->pyData))
			return FALSE;
		if (!PxDynaset_Thaw(self->pyDynaset))
			return FALSE;
	}
	return TRUE;
}

static PyObject* // new ref
PxWidget_str(PxWidgetObject* self)
{
	TCHAR szText[1024];
	GetWindowTextA(self->hWin, szText, 1024);
	return PyUnicode_FromFormat("%.50s - '%.50s'", Py_TYPE(self)->tp_name, szText);
}

static PyMemberDef PxWidget_members[] = {
	{ "nativeWidget", T_INT, offsetof(PxWidgetObject, hWin), READONLY, "Win32 handle of the MS-Windows window" },
	{ "left", T_INT, offsetof(PxWidgetObject, rc.left), READONLY, "Distance from left edge of parent, if negative from right" },
	{ "top", T_INT, offsetof(PxWidgetObject, rc.top), READONLY, "Distance from top edge of parent, if negative from bottom" },
	{ "right", T_INT, offsetof(PxWidgetObject, rc.right), READONLY, "Distance from left or, if zero or negative, from right edge of parent of right edge" },
	{ "bottom", T_INT, offsetof(PxWidgetObject, rc.bottom), READONLY, "Distance from top or, if zero or negative, from bottom edge of parent of bottom edge" },
	{ "dynaset", T_OBJECT, offsetof(PxWidgetObject, pyDynaset), READONLY, "Bound Dynaset" },
	{ "dataColumn", T_OBJECT, offsetof(PxWidgetObject, pyDataColumn), READONLY, "Connected DataColumn" },
	{ "dataType", T_OBJECT, offsetof(PxWidgetObject, pyDataType), READONLY, "Data type of connected DataColumn" },
	{ "editFormat", T_OBJECT, offsetof(PxWidgetObject, pyFormatEdit), 0, "Format when editing" },
	{ "format", T_OBJECT, offsetof(PxWidgetObject, pyFormat), 0, "Format for display" },
	{ "label", T_OBJECT, offsetof(PxWidgetObject, pyLabel), READONLY, "Associated Label widget" },
	{ "parent", T_OBJECT, offsetof(PxWidgetObject, pyParent), READONLY, "Parent widget" },
	{ "window", T_OBJECT, offsetof(PxWidgetObject, pyWindow), READONLY, "Parent window" },
	//{ "dirty", T_BOOL, offsetof(PxWidgetObject, bDirty), READONLY, "Set if widget has been edited." },
	{ "readOnly", T_BOOL, offsetof(PxWidgetObject, bReadOnly), 0, "Data can not be edited." },
	{ "alignHoriz", T_OBJECT, offsetof(PxWidgetObject, pyAlignHorizontal), READONLY, "Horizontal alignment" },
	{ "alignVert", T_OBJECT, offsetof(PxWidgetObject, pyAlignVertical), READONLY, "Vertical alignment" },
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxWidget_methods[] = {
	{ "move", (PyCFunction)PxWidget_move, METH_VARARGS | METH_KEYWORDS, "Reposition widget to given coordinates." },
	{ "refresh", (PyCFunction)PxWidget_pass, METH_NOARGS, "Ignore" },
	{ "focus_in", (PyCFunction)PxWidget_focus_in, METH_NOARGS, "Widget receives focus." },
	{ "refresh_cell", (PyCFunction)PxWidget_pass, METH_VARARGS | METH_KEYWORDS, "Ignore" },
	{ "refresh_row_pointer", (PyCFunction)PxWidget_pass, METH_NOARGS, "Ignore" },
	{ NULL }  /* Sentinel */
};

PyTypeObject PxWidgetType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Widget",            /* tp_name */
	sizeof(PxWidgetObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxWidget_dealloc, /* tp_dealloc */
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
	PxWidget_str,              /* tp_str */
	PxWidget_getattro,         /* tp_getattro */
	PxWidget_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,       /* tp_flags */
	"The parent of all widgets", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxWidget_methods,          /* tp_methods */
	PxWidget_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxWidget_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxWidget_new,              /* tp_new */
	0,                         /*tp_free*/
	0,                         /*tp_is_gc*/
};


BOOL PxWidget_MoveWindow(PxWidgetObject* self)
{
	RECT rect;
	TransformRectToAbs(&(self->rc), GetParent(self->hWin), &rect);
	if (MoveWindow(self->hWin, rect.left, rect.top, rect.right, rect.bottom, TRUE) == 0) {
		PyErr_SetFromWindowsErr(0);
		return FALSE;
	}
	else
		return TRUE;
}

void TransformRectToAbs(_In_ PRECT rcRel, _In_ HWND hwndParentWindow, _Out_ PRECT rcAbs)
{
	RECT rcParent;
	GetClientRect(hwndParentWindow, &rcParent);
	if (rcRel->left == PxWIDGET_CENTER)
		rcAbs->left = (rcParent.right - rcRel->right) / 2;
	else
		rcAbs->left = (rcRel->left >= 0 || rcRel->left == CW_USEDEFAULT ? rcRel->left : rcParent.right + rcRel->left);
	rcAbs->right = (rcRel->right > 0 || rcRel->right == CW_USEDEFAULT ? rcRel->right : rcParent.right + rcRel->right - rcAbs->left);
	if (rcRel->top == PxWIDGET_CENTER)
		rcAbs->top = (rcParent.bottom - rcRel->bottom) / 2;
	else
		rcAbs->top = (rcRel->top >= 0 || rcRel->top == CW_USEDEFAULT ? rcRel->top : rcParent.bottom + rcRel->top);
	rcAbs->bottom = (rcRel->bottom > 0 || rcRel->bottom == CW_USEDEFAULT ? rcRel->bottom : rcParent.bottom + rcRel->bottom - rcAbs->top);
}

BOOL CALLBACK
PxWidgetSizeEnumProc(HWND hwndChild, LPARAM lParam)
{
	PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr(hwndChild, GWLP_USERDATA);
	if (pyWidget != NULL)
		PxWidget_MoveWindow(pyWidget);

	EnumChildWindows(hwndChild, PxWidgetSizeEnumProc, (LPARAM)0);
	return TRUE;
}