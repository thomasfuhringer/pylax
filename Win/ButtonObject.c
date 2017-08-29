// ButtonObject.c  | Pylax © 2015 by Thomas Führinger
#include "Pylax.h"

static LRESULT CALLBACK PxButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static PyObject *
PxButton_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxButtonObject* self = (PxButtonObject*)PxButtonType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyOnClickCB = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxButton_init(PxButtonObject *self, PyObject *args, PyObject *kwds)
{
	if (PxButtonType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);

	self->hWin = CreateWindowEx(0, L"BUTTON", L"OK",
		WS_TABSTOP | WS_CHILD | BS_TEXT | BS_PUSHBUTTON | (gArgs.bVisible ? WS_VISIBLE : 0), // | BS_NOTIFY
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXBUTTON, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	self->fnOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxButtonProc);
	SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));

	return 0;
}

BOOL
PxButton_Clicked(PxButtonObject* self)
{
	/*int iR = PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)NULL);
	if (iR == -1)
	return FALSE;
	else if (iR == FALSE)
	return TRUE;*/

	if (self->pyOnClickCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		Py_INCREF(self);
		PyObject* pyResult = PyObject_CallObject(self->pyOnClickCB, pyArgs);
		if (pyResult == NULL)
			return FALSE;
		Py_DECREF(pyResult);
	}
	return TRUE;
}

static void
PxButton_dealloc(PxButtonObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static int
PxButton_setattro(PxButtonObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click") == 0) 	{
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyOnClickCB);
				self->pyOnClickCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "defaultEnter") == 0) {
			//int iFlag =  pyValue == Py_True ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;
			if (SetWindowLongPtrW(self->hWin, GWL_STYLE, GetWindowLongPtrW(self->hWin, GWL_STYLE) & ~(BS_PUSHBUTTON | BS_DEFPUSHBUTTON) | (pyValue == Py_True ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON)) == NULL) {
				PyErr_SetFromWindowsErr(0);
				return -1;
			}
			return 0;
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxButton_getattro(PxButtonObject* self, PyObject* pyAttributeName)
{
	OutputDebugString(L"\nPx+Button_GETattro **** \n");
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click") == 0) 	{
			PyErr_Clear();
			Py_INCREF(self->pyOnClickCB);
			return self->pyOnClickCB;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
	//return pyResult;
}

static PyMemberDef PxButton_members[] = {
	//{ "title", T_OBJECT_EX, offsetof(PxWindowObject, title), 0, "window title" },
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxButton_methods[] = {
	//{ "show", (PyCFunction)PxWindow_Show, METH_NOARGS, "Make it visible" },
	{ NULL }  /* Sentinel */
};

PyTypeObject PxButtonType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Button",            /* tp_name */
	sizeof(PxButtonObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxButton_dealloc, /* tp_dealloc */
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
	PxButton_getattro,         /* tp_getattro */
	PxButton_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Button objects",          /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxButton_methods,          /* tp_methods */
	PxButton_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxButton_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxButton_new,             /* tp_new */
};

static LRESULT CALLBACK
PxButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PxButtonObject* self = (PxButtonObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{/*
	case WM_SETFOCUS:
	if (PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)NULL) == -1)
	ShowPythonError();
	//ShowInts(L"B", 0, self->pyWindow->pyFocusWidget);
	OutputDebugString(L"\n*---- Button focus");
	return 0;*/

	case OCM_COMMAND:
		switch (HIWORD(wParam))
		{
			/*case BN_SETFOCUS:
					OutputDebugString(L"\n*---- Button focus");
					if (PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)NULL) == -1)
					ShowPythonError();
					//ShowInts(L"B", 0, self->pyWindow->pyFocusWidget);
					return 0;

					case BN_KILLFOCUS:
					OutputDebugString(L"\n*---- Button kill focus");
					return 0;*/

		case BN_CLICKED:
			//OutputDebugString(L"\n*---- Button clicked");
			if (PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)NULL) == -1) {
				ShowPythonError();
				return 0;
			}
			if (self == self->pyWindow->pyOkButton) {
				self->pyWindow->bOkPressed = TRUE;
				SendMessage(self->pyWindow->hWin, WM_CLOSE, 0, 0);
				return 0;
			}
			else if (self == self->pyWindow->pyCancelButton) {
				self->pyWindow->bOkPressed = FALSE;
				SendMessage(self->pyWindow->hWin, WM_CLOSE, 0, 0);
				return 0;
			}
			else if (!PxButton_Clicked(self))
				ShowPythonError();
			return 0;
		}
		break;

	default:
		break;
	}
	return CallWindowProc(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
}