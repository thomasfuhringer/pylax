#include "Pylax.h"

static PyObject *
PxTab_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxTabObject* self = (PxTabObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->iPages = 0;
		self->pyCurrentPage = NULL;
		self->pyPages = PyList_New(0);
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxTab_init(PxTabObject *self, PyObject *args, PyObject *kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	self->gtk = gtk_notebook_new();
	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	PxWidget_Reposition(self);
	gtk_widget_show(self->gtk);
	//gtk_container_check_resize(GTK_CONTAINER(self->gtk));
	g_object_set_qdata(self->gtk, g.gQuark, self);

	if (gArgs.pyCaption != NULL)
		if (!PxLabel_SetCaption(self, gArgs.pyCaption))
			return -1;
	return 0;
}

bool
PxTab_RepositionChildren(PxWidgetObject* self)
{
	GtkWidget* gtkPage;
	PxWidgetObject* pyPage;
	gint nPage, nPages = gtk_notebook_get_n_pages(self->gtk);

	for (nPage = 0; nPage < nPages; nPage++) {
		gtkPage = gtk_notebook_get_nth_page(self->gtk, nPage);
		pyPage = g_object_get_qdata(gtkPage, g.gQuark);
		if (!PxWidget_RepositionChildren(pyPage))
			return false;
	}
	return true;
}

static void
PxTab_dealloc(PxTabObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxTab_members[] = {
	{ NULL }
};

static PyMethodDef PxTab_methods[] = {
	{ NULL }
};

PyTypeObject PxTabType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Tab",               /* tp_name */
	sizeof(PxTabObject),       /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxTab_dealloc, /* tp_dealloc */
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
	"Tab widget objects",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxTab_methods,             /* tp_methods */
	PxTab_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxTab_init,      /* tp_init */
	0,                         /* tp_alloc */
	PxTab_new,                 /* tp_new */
};


/* TabPage ----------------------------------------------------------------------- */

static PyObject *
PxTabPage_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxTabPageObject* self = (PxTabPageObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->iIndex = -1;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxTabPage_init(PxTabPageObject* self, PyObject* args, PyObject* kwds)
{
	GtkWidget* gtkTabLabel;

	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	if (gArgs.pyCaption != NULL)
		gtkTabLabel = gtk_label_new(PyUnicode_AsUTF8(gArgs.pyCaption));
	else
		gtkTabLabel = gtk_label_new("New");

	if (!PyObject_TypeCheck(self->pyParent, &PxTabType)) {
		PyErr_SetString(PyExc_TypeError, "'parent' must be a Tab.");
		return -1;
	}

	self->gtk = self->gtkFixed = gtk_fixed_new();
	self->iIndex = gtk_notebook_append_page(GTK_NOTEBOOK(self->pyParent->gtk), self->gtkFixed, gtkTabLabel);
	if (self->iIndex == -1) {
		PyErr_SetString(PyExc_RuntimeError, "Can not create GtkNotebook page.");
		return -1;
	}

	if (PyList_Append(((PxTabObject*)self->pyParent)->pyPages, self) == -1) {
		PyErr_SetString(PyExc_RuntimeError, "Can not append Tab Page.");
		return -1;
	}
	gtk_widget_show_all(self->gtkFixed);
	//gtk_container_check_resize(GTK_CONTAINER(self->pyParent->gtk));
	gtk_container_check_resize(GTK_CONTAINER(g.gtkNotebook));

	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}


static void
PxTabPage_dealloc(PxTabPageObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxTabPage_members[] = {
	{ NULL }
};

static PyMethodDef PxTabPage_methods[] = {
	{ NULL }
};

PyTypeObject PxTabPageType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.TabPage",           /* tp_name */
	sizeof(PxTabPageObject),   /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxTabPage_dealloc, /* tp_dealloc */
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
	"TabPage widget objects",  /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxTabPage_methods,         /* tp_methods */
	PxTabPage_members,         /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxTabPage_init,  /* tp_init */
	0,                         /* tp_alloc */
	PxTabPage_new,             /* tp_new */
};
