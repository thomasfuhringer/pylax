#include "Pylax.h"

static PyObject *
PxForm_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxFormObject* self = (PxFormObject*)PxFormType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		return (PyObject*)self;
	}
	return NULL;
}

static int
PxForm_init(PxFormObject *self, PyObject *args, PyObject *kwds)
{
	MDICREATESTRUCT MdiCreate;
	if (PxFormType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, g.hMDIClientArea, &rect);

	MdiCreate.szClass = L"PxWindowClass";
	MdiCreate.szTitle = L"<new>";
	MdiCreate.hOwner = g.hInstance;
	MdiCreate.x = rect.left; // CW_USEDEFAULT; 
	MdiCreate.y = rect.top;// CW_USEDEFAULT;
	MdiCreate.cx = rect.right;// CW_USEDEFAULT;
	MdiCreate.cy = rect.bottom;// CW_USEDEFAULT;
	MdiCreate.style = WS_EX_CLIENTEDGE | WS_EX_MDICHILD | WS_EX_CONTROLPARENT | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	MdiCreate.lParam = 0;

	self->hWin = (HWND)SendMessage(g.hMDIClientArea, WM_MDICREATE, 0, (LPARAM)(LPMDICREATESTRUCT)&MdiCreate);
	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}
	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);

	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	return 0;
}

static void
PxForm_dealloc(PxFormObject* self)
{
	PxFormType.tp_base->tp_dealloc((PyObject *)self);
}

static int
PxForm_setattro(PxFormObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			return  0; // MDI window can not be hidden
		}
	}
	return PxFormType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxForm_getattro(PxFormObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			PyErr_Clear();
			Py_RETURN_TRUE; // MDI window can not be hidden
		}
	}
	return PxFormType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static PyMemberDef PxForm_members[] = {
	// Duplication from PxWindowObject. For some reason inheritance of these from immediate super class (base type) did not work.
	{ "cnx", T_OBJECT, offsetof(PxWindowObject, pyConnection), READONLY, "Database connection" },
	{ "minWidth", T_INT, offsetof(PxWindowObject, cxMin), 0, "Window can not be resized smaller" },
	{ "minHeight", T_INT, offsetof(PxWindowObject, cyMin), 0, "Window can not be resized smaller" },
	{ "nameInCaption", T_BOOL, offsetof(PxWindowObject, bNameInCaption), 0, "Show name of the Window in the caption" },
	{ "buttonCancel", T_OBJECT, offsetof(PxWindowObject, pyCancelButton), 0, "Close the dialog" },
	{ "buttonOK", T_OBJECT, offsetof(PxWindowObject, pyOkButton), 0, "Close the dialog" },
	{ NULL }
};

//PyObject* PxWindow_set_focus(PxWindowObject* self, PyObject *args, PyObject *kwds);
static PyMethodDef PxForm_methods[] = {
	// Duplication from PxWindowObject. For some reason inheritance of these from immediate super class (base type) did not work.
	{ "create_timer", (PyCFunction)PxWindow_create_timer, METH_VARARGS | METH_KEYWORDS, "Create a new timer." },
	{ "delete_timer", (PyCFunction)PxWindow_delete_timer, METH_VARARGS | METH_KEYWORDS, "Delete a timer." },
	//{ "set_focus", (PyCFunction)PxWindow_set_focus, METH_VARARGS | METH_KEYWORDS, "Set the focus to given widget." },
	{ NULL }
};

PyTypeObject PxFormType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Form",              /* tp_name */
	sizeof(PxFormObject),      /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxForm_dealloc, /* tp_dealloc */
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
	PxForm_getattro,           /* tp_getattro */
	PxForm_setattro,           /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Form objects",            /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxForm_methods,            /* tp_methods */
	PxForm_members,            /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxForm_init,     /* tp_init */
	0,                         /* tp_alloc */
	PxForm_new,                /* tp_new */
};