// FormObject.c  | Pylax © 2017 by Thomas Führinger
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
	if (PxFormType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	self->gtkFixed = gtk_fixed_new();
	self->gtk = self->gtkFixed;

	char* sCaption;
	if (gArgs.pyCaption != NULL)
		sCaption = PyUnicode_AsUTF8(gArgs.pyCaption);
	else
		sCaption = "<new>";

	self->gtkLabel = gtk_label_new(sCaption);
	gint iIndex = gtk_notebook_append_page(GTK_NOTEBOOK(g.gtkNotebook), self->gtkFixed, self->gtkLabel);
	gtk_widget_show_all(self->gtkFixed);
	gtk_container_check_resize(GTK_CONTAINER(g.gtkNotebook));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(g.gtkNotebook), iIndex);
    
	gint x, y, width, height;
	gtk_window_get_position(GTK_WINDOW(g.gtkMainWindow), &x, &y);
	gtk_window_move(GTK_WINDOW(g.gtkMainWindow), x, y);   // trigger a configure-event

	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}

static PyObject*
PxForm_set_icon_from_file(PxFormObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "fileName", NULL };
	const char* sFileName;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist,
		&sFileName))
		return NULL;

	PyErr_Format(PyExc_RuntimeError, "Setting Form icon to '%.200s' still not implemented.", sFileName);
	return NULL;

	Py_RETURN_TRUE;
}

static bool
PxForm_SetCaption(PxFormObject* self, PyObject* pyText)
{
	if (self->bNameInCaption) {
		if (pyText == Py_None || PyUnicode_GetLength(pyText) == 0) {
			pyText = self->pyName;
			Py_INCREF(pyText);
		}
		else {
			PyObject* pyNameTuple = Py_BuildValue("OsO", self->pyName, " - ", pyText);
			pyText = PyUnicode_Join(NULL, pyNameTuple);
			Py_DECREF(pyNameTuple);
		}
	}
	else
		Py_INCREF(pyText);

	char* sText = PyUnicode_AsUTF8(pyText);
	gtk_label_set_text(self->gtkLabel, sText);
	Py_DECREF(pyText);
	return true;
}

bool
PxForm_Close(PxFormObject* self)
{
	// todo: ask for save, clean up...
	gtk_notebook_remove_page(g.gtkNotebook, gtk_notebook_page_num(g.gtkNotebook, self->gtk));
	return true;
}

static void
PxForm_dealloc(PxFormObject* self)
{
	PxFormType.tp_base->tp_dealloc((PyObject*)self);
}

static int
PxForm_setattro(PxFormObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			return PxForm_SetCaption(self, pyValue) ? 0 : -1;
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
			Py_RETURN_TRUE;
		}
	}
	return PxFormType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static PyMemberDef PxForm_members[] = {
	// Duplication from PxWindowObject. For some reason inheritance of these from immediate super class (base type) did not work.
	{ "cnx", T_OBJECT, offsetof(PxWindowObject, pyConnection), READONLY, "Database connection" },
	{ "minWidth", T_INT, offsetof(PxWindowObject, iMinX), 0, "Window can not be resized smaller" },
	{ "minHeight", T_INT, offsetof(PxWindowObject, iMinY), 0, "Window can not be resized smaller" },
	{ "nameInCaption", T_BOOL, offsetof(PxWindowObject, bNameInCaption), 0, "Show name of the Window in the caption" },
	//{ "buttonCancel", T_OBJECT, offsetof(PxWindowObject, pyCancelButton), 0, "Close the dialog" },
	//{ "buttonOK", T_OBJECT, offsetof(PxWindowObject, pyOkButton), 0, "Close the dialog" },
	{ NULL }
};

//PyObject* PxWindow_set_focus(PxWindowObject* self, PyObject *args, PyObject *kwds);
static PyMethodDef PxForm_methods[] = {
	// Duplication from PxWindowObject. For some reason inheritance of these from immediate super class (base type) did not work.
	{ "set_icon_from_file", (PyCFunction)PxForm_set_icon_from_file, METH_VARARGS | METH_KEYWORDS, "Set the Form icon from filename given." },
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
