// DialogObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

static PyObject *
PxDialog_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxDialogObject* self = (PxDialogObject*)PxDialogType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		return (PyObject*)self;
	}
	return NULL;
}

static int
PxDialog_init(PxDialogObject *self, PyObject *args, PyObject *kwds)
{
	char* sCaption;
	GtkWidget* gtkContentArea;

	if (PxDialogType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	if (gArgs.pyCaption != NULL)
		sCaption = PyUnicode_AsUTF8(gArgs.pyCaption);
	else
		sCaption = "<new>";

	self->gtk = gtk_dialog_new_with_buttons(sCaption, g.gtkMainWindow, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);
    gtk_dialog_set_default_response(self->gtk, GTK_RESPONSE_REJECT);
    gtk_dialog_set_response_sensitive(self->gtk, GTK_RESPONSE_ACCEPT, FALSE);
    gtk_widget_hide_on_delete (self->gtk);

	//self->gtkOKButton = gtk_dialog_get_widget_for_response(self->gtk, GTK_RESPONSE_ACCEPT);

	self->gtkFixed = gtk_fixed_new();
	gtkContentArea = gtk_dialog_get_content_area(GTK_DIALOG(self->gtk));
	gtk_container_add(GTK_CONTAINER(gtkContentArea), self->gtkFixed);
	gtk_widget_show(self->gtkFixed);
	g_signal_connect(G_OBJECT(self->gtk), "configure-event", G_CALLBACK(GtkWindow_ConfigureEventCB), self);
    g_signal_connect_swapped (GTK_DIALOG(self->gtk), "response",(GCallback) gtk_widget_hide, self->gtk);
	gtk_window_move(GTK_WINDOW(self->gtk), self->rc.iLeft, self->rc.iTop);
	gtk_window_resize(GTK_WINDOW(self->gtk), self->rc.iWidth, self->rc.iHeight);
	//gtk_widget_show_all(self->gtk);

	if (self->pyDynaset != NULL)
		self->pyDynaset->pyDialog = self;

	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}
/*
static PyObject*
PxDialog_set_icon_from_file(PxDialogObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "fileName", NULL };
	const char* sFileName;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist,
		&sFileName))
		return NULL;

	PyErr_Format(PyExc_RuntimeError, "Setting Dialog icon to '%.200s' still not implemented.", sFileName);
	return NULL;

	Py_RETURN_TRUE;
}*/

static bool
PxDialog_SetCaption(PxDialogObject* self, PyObject* pyText)
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
	gtk_window_set_title(self->gtk, sText);
	Py_DECREF(pyText);
	return true;
}

static PyObject*
PxDialog_run(PxDialogObject* self)
{
	gint result = gtk_dialog_run(GTK_DIALOG(self->gtk));
	switch (result)
	{
	case GTK_RESPONSE_ACCEPT:
		Py_RETURN_TRUE;
		break;
	default:
		Py_RETURN_FALSE;
		break;
	}
}

static void
PxDialog_dealloc(PxDialogObject* self)
{
	gtk_widget_destroy(self->gtk);
	PxDialogType.tp_base->tp_dealloc((PyObject*)self);
}

static int
PxDialog_setattro(PxDialogObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {   // deactivate property
			return  0;
		}
		/*if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			return PxWindow_SetCaption(self, pyValue) ? 0 : -1;
		}*/
	}
	return PxDialogType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxDialog_getattro(PxDialogObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	/*if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			PyErr_Clear();
			Py_RETURN_TRUE;
		}
	}*/
	return PxDialogType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static PyMemberDef PxDialog_members[] = {
	// Duplication from PxWindowObject. For some reason inheritance of these from immediate super class (base type) did not work.
	{ "cnx", T_OBJECT, offsetof(PxWindowObject, pyConnection), READONLY, "Database connection" },
	{ "minWidth", T_INT, offsetof(PxWindowObject, iMinX), 0, "Window can not be resized smaller" },
	{ "minHeight", T_INT, offsetof(PxWindowObject, iMinY), 0, "Window can not be resized smaller" },
	{ "nameInCaption", T_BOOL, offsetof(PxWindowObject, bNameInCaption), 0, "Show name of the Window in the caption" },
	{ NULL }
};

static PyMethodDef PxDialog_methods[] = {
	{ "run", (PyCFunction)PxDialog_run, METH_NOARGS, "Show (modal)" },
	// Duplication from PxWindowObject. For some reason inheritance of these from immediate super class (base type) did not work.
	{ "set_icon_from_file", (PyCFunction)PxWindow_set_icon_from_file, METH_VARARGS | METH_KEYWORDS, "Set the Form icon from filename given." },
	{ NULL }
};

PyTypeObject PxDialogType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Dialog",            /* tp_name */
	sizeof(PxDialogObject),      /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxDialog_dealloc, /* tp_dealloc */
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
	PxDialog_getattro,         /* tp_getattro */
	PxDialog_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Dialog objects",          /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxDialog_methods,          /* tp_methods */
	PxDialog_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxDialog_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxDialog_new,              /* tp_new */
};
