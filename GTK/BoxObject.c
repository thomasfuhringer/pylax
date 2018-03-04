#include "Pylax.h"

static PyObject *
PxBox_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxBoxObject* self = (PxBoxObject*)PxBoxType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxBox_init(PxBoxObject* self, PyObject* args, PyObject* kwds)
{
	if (PxBoxType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	self->gtkFixed = gtk_fixed_new();
	self->gtk = self->gtkFixed;
	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);
	if (self->pyParent->gtkFixed)
		gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	gtk_widget_show(self->gtk);
	//gtk_container_check_resize(GTK_CONTAINER(g.gtkNotebook));
	//PxWidget_Reposition(self);
	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}

static void
PxBox_dealloc(PxBoxObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxBox_members[] = {
	{ NULL }
};

static PyMethodDef PxBox_methods[] = {
	{ NULL }
};

PyTypeObject PxBoxType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Box",               /* tp_name */
	sizeof(PxBoxObject),       /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxBox_dealloc, /* tp_dealloc */
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
	"Box widget objects",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxBox_methods,             /* tp_methods */
	PxBox_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxBox_init,      /* tp_init */
	0,                         /* tp_alloc */
	PxBox_new,                 /* tp_new */
};
