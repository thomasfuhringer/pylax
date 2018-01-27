// ButtonObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

static gboolean GtkButton_ClickedCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer pUserData);

static PyObject*
PxButton_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
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
PxButton_init(PxButtonObject* self, PyObject* args, PyObject* kwds)
{
	if (PxButtonType.tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;

	if (gArgs.pyCaption == NULL)
		self->gtk = gtk_button_new();
	else
		self->gtk = gtk_button_new_with_label(PyUnicode_AsUTF8(gArgs.pyCaption));

	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	gtk_widget_show(self->gtk);
	g_object_set_qdata(G_OBJECT(self->gtk), g.gQuark, self);
	g_signal_connect(G_OBJECT(self->gtk), "clicked", G_CALLBACK(GtkButton_ClickedCB), self);
	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);

	return 0;
}

static gboolean
GtkButton_ClickedCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer pUserData)
{
	PxButtonObject* pyButton = g_object_get_qdata(gtkWidget, g.gQuark);
	if (!PxButton_Clicked(pyButton))
		PythonErrorDialog();
	return true;
}

bool
PxButton_Clicked(PxButtonObject* self)
{
	int iR = PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)NULL);
	if (iR == -1)
		return false;
	else if (iR == false)
		return true;

	if (self->pyOnClickCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		Py_INCREF(self);
		PyObject* pyResult = PyObject_CallObject(self->pyOnClickCB, pyArgs);
		if (pyResult == NULL)
			return false;
		Py_DECREF(pyResult);
	}
	return true;
}

static void
PxButton_dealloc(PxButtonObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	Py_XDECREF(self->pyOnClickCB);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject*)self);
}

static int
PxButton_setattro(PxButtonObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			gtk_button_set_label(self->gtk, PyUnicode_AsUTF8(pyValue));
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click") == 0) {
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
			// todo
			return 0;
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject*
PxButton_getattro(PxButtonObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click") == 0) {
			PyErr_Clear();
			Py_INCREF(self->pyOnClickCB);
			return self->pyOnClickCB;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static PyMemberDef PxButton_members[] = {
	{ NULL }
};

static PyMethodDef PxButton_methods[] = {
	{ NULL }
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
