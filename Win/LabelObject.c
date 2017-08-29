#include "Pylax.h"

static PyObject *
PxLabel_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxLabelObject* self = (PxLabelObject*)PxLabelType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyCaptionClient = NULL;
		self->textColor = 0;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static LRESULT CALLBACK PxLabelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static int
PxLabel_init(PxLabelObject *self, PyObject *args, PyObject *kwds)
{
	if (PxLabelType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);
	self->hWin = CreateWindowEx(0, L"STATIC", L"",
		WS_CHILD | (gArgs.bVisible ? WS_VISIBLE : 0),
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXLABEL, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	//if (!gArgs.bVisible)
	//    ShowWindow(self->hWin,  SW_HIDE);
	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));
	self->fnOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxLabelProc);
	return 0;
}

BOOL
PxLabel_SetData(PxLabelObject* self, PyObject* pyData)
{
	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return FALSE;

	if (self->pyCaptionClient)
		return PyObject_SetAttrString(self->pyCaptionClient, "caption", pyData) == -1 ? FALSE : TRUE;
	else
		return PxWidget_SetCaption((PxWidgetObject*)self, pyData);
}

static int
PxLabel_setattro(PxLabelObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			return PxWidget_SetCaption((PxWidgetObject*)self, pyValue) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			return PxLabel_SetData(self, pyValue) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "alignHoriz") == 0) {
			LONG_PTR pAlign = GetWindowLongPtr(self->hWin, GWL_STYLE);
			switch (PyLong_AsLong(PyObject_GetAttrString(pyValue, "value")))
			{
			case ALIGN_LEFT:
				pAlign = pAlign | SS_LEFT;
				break;
			case ALIGN_RIGHT:
				pAlign = pAlign | SS_RIGHT;
				break;
			case ALIGN_CENTER:
				pAlign = pAlign | SS_CENTER;
				break;
			}
			SetWindowLongPtr(self->hWin, GWL_STYLE, pAlign);
			self->pyAlignHorizontal = pyValue;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "alignVert") == 0) {

			LONG_PTR pAlign = GetWindowLongPtr(self->hWin, GWL_STYLE);

			switch (PyLong_AsLong(PyObject_GetAttrString(pyValue, "value")))
			{
			case ALIGN_TOP:
				//pAlign = pAlign | SS_TOP ; 
				break;
			case ALIGN_BOTTOM:
				//pAlign = pAlign | SS_BOTTOM ; 
				break;
			case ALIGN_CENTER:
				pAlign = pAlign | SS_CENTERIMAGE;
				break;
			}
			SetWindowLongPtr(self->hWin, GWL_STYLE, pAlign);
			self->pyAlignVertical = pyValue;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "textColor") == 0) 	{
			if (PyTuple_Check(pyValue)) {
				int iR, iG, iB;
				PyObject* pyColorComponent = PyTuple_GetItem(pyValue, 0);
				if (pyColorComponent == NULL)
					return -1;
				iR = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 1);
				if (pyColorComponent == NULL)
					return -1;
				iG = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 2);
				if (pyColorComponent == NULL)
					return -1;
				iB = PyLong_AsLong(pyColorComponent);

				self->textColor = RGB(iR, iG, iB);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
	}
	//return PyObject_GenericSetAttr((PyObject*)self, pyAttributeName, pyValue);
	return Py_TYPE(self)->tp_base->tp_setattro((PxWidgetObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxLabel_getattro(PxLabelObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) 	{
			TCHAR szText[1024];
			GetWindowText(self->hWin, szText, 1024);
			LPCSTR strText = toU8(szText);
			PyObject* pyText = PyUnicode_FromString(strText);
			PyMem_RawFree(strText);
			PyErr_Clear();
			return pyText;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PxWidgetObject*)self, pyAttributeName);
	//return pyResult;
}

static PyObject *
PxLabel_refresh(PxLabelObject* self)
{
	PyObject *pyData = PxWidget_PullData((PxWidgetObject*)self);
	if (pyData) {
		if (PxLabel_SetData(self, pyData))
			Py_RETURN_TRUE;
		else
			return NULL;
	}
	Py_RETURN_FALSE;
}

static void
PxLabel_dealloc(PxLabelObject* self)
{
	Py_XDECREF(self->pyAssociatedWidget);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxLabel_members[] = {
	{ "captionClient", T_OBJECT, offsetof(PxLabelObject, pyCaptionClient), 0, "Pass any assigment to property 'data' on to property 'caption' of this." },
	{ NULL }
};

static PyMethodDef PxLabel_methods[] = {
	{ "refresh", (PyCFunction)PxLabel_refresh, METH_NOARGS, "Pull fresh data" },
	{ NULL }
};

PyTypeObject PxLabelType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Label",             /* tp_name */
	sizeof(PxLabelObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxLabel_dealloc, /* tp_dealloc */
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
	PxLabel_getattro,          /* tp_getattro */
	PxLabel_setattro,          /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Label objects",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxLabel_methods,           /* tp_methods */
	PxLabel_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxLabel_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxLabel_new,               /* tp_new */
};

static LRESULT CALLBACK
PxLabelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PxLabelObject* self = (PxLabelObject*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case OCM_CTLCOLORSTATIC: {
		HDC hDC = (HDC)wParam;
		SetBkMode(hDC, TRANSPARENT);
		if (self->textColor) {
			SetTextColor(hDC, self->textColor);
		}
		return self->pyParent->hBkgBrush;
	}
	default:
		break;
	}
	return CallWindowProcW(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
}