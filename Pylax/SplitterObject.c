#include "Pylax.h"

static LRESULT CALLBACK PxSplitterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static int PxSplitter_ResizeBoxes(PxSplitterObject* self);

static PyObject *
PxSplitter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxSplitterObject* self = PxSplitterType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->bVertical = TRUE;
		self->iPosition = 100;
		self->iSpacing = 4;
		return (PyObject*)self;
	}
	else
		return NULL;
}

extern PyTypeObject PxWidgetType;

static int
PxSplitter_init(PxSplitterObject* self, PyObject* args, PyObject *kwds)
{
	if (PxSplitterType.tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;

	PyObject* pyArgList = Py_BuildValue("Oiiii", (PyObject*)self, 0, 0, 0, 0);
	self->pyBox1 = PyObject_CallObject((PyObject *)&PxBoxType, pyArgList);
	if (self->pyBox1 == NULL)
		return -1;
	SetWindowLongPtr(self->pyBox1->hWin, GWL_EXSTYLE, WS_EX_CLIENTEDGE);

	self->pyBox2 = PyObject_CallObject((PyObject*)&PxBoxType, pyArgList);
	if (self->pyBox2 == NULL)
		return -1;
	Py_DECREF(pyArgList);
	SetWindowLongPtr(self->pyBox2->hWin, GWL_EXSTYLE, WS_EX_CLIENTEDGE);
	PxSplitter_ResizeBoxes(self);

	self->fnOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxSplitterProc);
	return 0;
}

static int
PxSplitter_setattro(PxSplitterObject* self, PyObject* pyAttributeName, PyObject* pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "position") == 0) {
			self->iPosition = PyLong_AsLong(pyValue);
			PxSplitter_ResizeBoxes(self);
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "spacing") == 0) {
			self->iSpacing = PyLong_AsLong(pyValue);
			PxSplitter_ResizeBoxes(self);
			return 0;
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PxWidgetObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxSplitter_getattro(PxSplitterObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "position") == 0) {
			PyErr_Clear();
			return PyLong_FromLong(self->iPosition);
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "spacing") == 0) {
			PyErr_Clear();
			return PyLong_FromLong(self->iSpacing);
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PxWidgetObject*)self, pyAttributeName);
}


static int
PxSplitter_ResizeBoxes(PxSplitterObject* self)
{
	self->pyBox1->rc = (RECT){ 0, 0, self->bVertical ? self->iPosition - self->iSpacing / 2 : 0, self->bVertical ? 0 : self->iPosition - self->iSpacing / 2 };
	self->pyBox2->rc = (RECT){ self->bVertical ? self->iPosition + self->iSpacing / 2 : 0, self->bVertical ? 0 : self->iPosition + self->iSpacing / 2, 0, 0 };
	EnumChildWindows(self->hWin, PxWidgetSizeEnumProc, 0);
	return 0;
}

static void
PxSplitter_dealloc(PxSplitterObject* self)
{
	Py_XDECREF(self->pyBox1);
	Py_XDECREF(self->pyBox2);
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxSplitter_members[] = {
	{ "box1", T_OBJECT_EX, offsetof(PxSplitterObject, pyBox1), READONLY, "Box 1 widget (for use as parent)" },
	{ "box2", T_OBJECT_EX, offsetof(PxSplitterObject, pyBox2), READONLY, "Box 2 widget (for use as parent)" },
	{ "vertical", T_BOOL, offsetof(PxSplitterObject, bVertical), 0, "Split vertically." },
	{ NULL }
};

static PyMethodDef PxSplitter_methods[] = {
	{ NULL }
};

PyTypeObject PxSplitterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Splitter",          /* tp_name */
	sizeof(PxSplitterObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxSplitter_dealloc, /* tp_dealloc */
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
	PxSplitter_getattro,       /* tp_getattro */
	PxSplitter_setattro,       /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Splits the client area in two panes, one of which is elastic",  /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxSplitter_methods,        /* tp_methods */
	PxSplitter_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxSplitter_init, /* tp_init */
	0,                         /* tp_alloc */
	PxSplitter_new,            /* tp_new */
};

static LRESULT CALLBACK
PxSplitterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL	bSplitterMoving = FALSE;
	//static DWORD dwSplitterPos;
	PxSplitterObject* self = (PxSplitterObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		if ((self->bVertical ? LOWORD(lParam) : HIWORD(lParam)) > 10)
		{
			SetCursor(self->bVertical ? g.hCursorWestEast : g.hCursorNorthSouth);
			RECT rect;
			if ((wParam == MK_LBUTTON) && bSplitterMoving) {
				if (self->bVertical) {
					GetClientRect(hwnd, &rect);
					if (LOWORD(lParam) > rect.right)
						return 0;
					self->iPosition = self->iPosition > 0 ? LOWORD(lParam) : LOWORD(lParam) - rect.right;
				}
				else{
					GetClientRect(hwnd, &rect);
					if (HIWORD(lParam) > rect.bottom)
						return 0;
					self->iPosition = (self->iPosition > 0) ? HIWORD(lParam) : HIWORD(lParam) - rect.bottom;
				}
				PxSplitter_ResizeBoxes(self);
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		bSplitterMoving = TRUE;
		SetCapture(hwnd);
		return 0;

	case WM_LBUTTONUP:
		ReleaseCapture();
		bSplitterMoving = FALSE;
		return 0;
	}
	return CallWindowProc(self->fnOldWinProcedure, hwnd, uMsg, wParam, lParam);
}