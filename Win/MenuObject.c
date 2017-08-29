#include "Pylax.h"

static iNextIdentifier = FIRST_CUSTOM_MENU_ID;

static PyObject *
PxMenu_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxMenuObject* self = (PxMenuObject *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->hWin = 0;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxMenu_init(PxMenuObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "caption", NULL };
	PyObject* pyCaption = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist,
		&pyCaption))
		return -1;

	if (!PyUnicode_Check(pyCaption)) {
		PyErr_SetString(PyExc_TypeError, "First parameter ('caption') must be a string.");
		return -1;
	}

	self->hWin = CreateMenu();
	LPCSTR strCaption = PyUnicode_AsUTF8(pyCaption);
	self->szCaption = toW(strCaption);

	return 0;
}

static PyObject*
PxMenu_append(PxMenuObject* self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "item", NULL };
	PyObject* pyItem = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, // "|p" would probably be better
		&pyItem))
		return NULL;

	if (PyObject_TypeCheck(pyItem, &PxMenuItemType)) {
		AppendMenuW(self->hWin, MF_STRING, ((PxMenuItemObject*)pyItem)->iIdentifier, ((PxMenuItemObject*)pyItem)->szCaption);
	}
	else if (PyObject_TypeCheck(pyItem, &PxMenuType)) {
		if (AppendMenuW(self->hWin, MF_POPUP, (UINT_PTR)((PxMenuObject*)pyItem)->hWin, ((PxMenuObject*)pyItem)->szCaption) == 0)
			return NULL;
	}
	else{
		PyErr_SetString(PyExc_TypeError, "Parameter 1 'item' must be a MenuItem or Menu.");
		return NULL;
	}
	Py_INCREF(pyItem);
	Py_RETURN_NONE;
}

static void
PxMenu_dealloc(PxMenuObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxMenu_members[] = {
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxMenu_methods[] = {
	{ "append", (PyCFunction)PxMenu_append, METH_VARARGS | METH_KEYWORDS, "Append MenuItem." },
	{ NULL }  /* Sentinel */
};

PyTypeObject PxMenuType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Menu",              /* tp_name */
	sizeof(PxMenuObject),      /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxMenu_dealloc, /* tp_dealloc */
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
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Menu object",             /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxMenu_methods,            /* tp_methods */
	PxMenu_members,            /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxMenu_init,     /* tp_init */
	0,                         /* tp_alloc */
	PxMenu_new,                /* tp_new */
};

/* MenuItem -----------------------------------------------------------------------*/

static PyObject *
PxMenuItem_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxMenuItemObject* self = (PxMenuItemObject *)type->tp_alloc(type, 0);
	if (self != NULL) {
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxMenuItem_init(PxMenuItemObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "caption", "on_click", NULL };
	PyObject* pyCaption = NULL, *pyOnClickCB = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist,
		&pyCaption,
		&pyOnClickCB))
		return -1;

	if (!PyUnicode_Check(pyCaption)) {
		PyErr_SetString(PyExc_TypeError, "First parameter ('caption') must be a string.");
		return -1;
	}

	if (!PyCallable_Check(pyOnClickCB)) {
		PyErr_SetString(PyExc_TypeError, "Second parameter ('on_click') must be callable");
		return -1;
	}

	self->iIdentifier = iNextIdentifier++;
	if (self->iIdentifier > MAX_CUSTOM_MENU_ID) {
		PyErr_SetString(PyExc_RuntimeError, "No more MenuItem can be created.");
		return -1;
	}
	g.pyMenuItem[self->iIdentifier - FIRST_CUSTOM_MENU_ID] = self;
	Py_INCREF(self);

	LPCSTR strCaption = PyUnicode_AsUTF8(pyCaption);
	self->szCaption = toW(strCaption);

	Py_XINCREF(pyOnClickCB);
	self->pyOnClickCB = pyOnClickCB;

	return 0;
}


static void
PxMenuItem_dealloc(PxMenuItemObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxMenuItem_members[] = {
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxMenuItem_methods[] = {
	{ NULL }  /* Sentinel */
};

PyTypeObject PxMenuItemType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.MenuItem",          /* tp_name */
	sizeof(PxMenuItemObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxMenuItem_dealloc, /* tp_dealloc */
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
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"MenuItem object",         /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxMenuItem_methods,        /* tp_methods */
	PxMenuItem_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxMenuItem_init, /* tp_init */
	0,                         /* tp_alloc */
	PxMenuItem_new,            /* tp_new */
};

void
PxMenuItems_DeleteAll()
{
	for (UINT_PTR iIdentifier = FIRST_CUSTOM_MENU_ID; iIdentifier < iNextIdentifier; iIdentifier++) {
		DeleteMenu(g.hAppMenu, iIdentifier, MF_BYCOMMAND);
		Py_DECREF(g.pyMenuItem[iIdentifier - FIRST_CUSTOM_MENU_ID]);
	}
	iNextIdentifier = FIRST_CUSTOM_MENU_ID;
	EnableMenuItem(g.hMainMenu, IDM_WINDOWMENUPOS + 1, MF_BYPOSITION | MF_GRAYED);
	DrawMenuBar(g.hWin);
}