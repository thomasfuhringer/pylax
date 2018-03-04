#include "Pylax.h"

static gboolean NotifyPositionEventCB(GObject* gObject, GParamSpec* gParamSpec, gpointer pUserData);
static int PxSplitter_ResizeBoxes(PxSplitterObject* self);

static PyObject *
PxSplitter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxSplitterObject* self = PxSplitterType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->bVertical = true;
		self->bBox1Resizes = true;
		self->bBox2Resizes = false;
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

	self->gtk = gtk_paned_new(self->bVertical ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);

	PyObject* pyArgList = Py_BuildValue("Oiiii", (PyObject*)self, 0, 0, 0, 0);
	self->pyBox1 = PyObject_CallObject((PyObject *)&PxBoxType, pyArgList);
	if (self->pyBox1 == NULL)
		return -1;
	gtk_paned_add1(self->gtk, self->pyBox1->gtk);

	self->pyBox2 = PyObject_CallObject((PyObject*)&PxBoxType, pyArgList);
	if (self->pyBox2 == NULL)
		return -1;
	gtk_paned_add2(self->gtk, self->pyBox2->gtk);
	Py_DECREF(pyArgList);

	PxSplitter_ResizeBoxes(self);

	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	//gtk_container_check_resize(GTK_CONTAINER(self->gtk));
	//gtk_container_check_resize(GTK_CONTAINER(g.gtkNotebook));
	gtk_widget_show_all(self->gtk);
	//PxWidget_Reposition(self);
	g_signal_connect(G_OBJECT(self->gtk), "notify::position", G_CALLBACK(NotifyPositionEventCB), self);
	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}

static int
PxSplitter_setattro(PxSplitterObject* self, PyObject* pyAttributeName, PyObject* pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "position") == 0) {
			self->iPosition = PyLong_AsLong(pyValue);
			gtk_paned_set_position(self->gtk, self->iPosition);
			PxSplitter_ResizeBoxes(self);
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "spacing") == 0) {
			self->iSpacing = PyLong_AsLong(pyValue);
			//gtk_paned_set_handle_size(self->gtk, self->iSpacing);
			PxSplitter_ResizeBoxes(self);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "box1Resizes") == 0) {
			self->bBox1Resizes = (pyValue == Py_True);
			gtk_container_child_set(GTK_CONTAINER(self->gtk), self->pyBox1->gtk, "resize", self->bBox1Resizes, NULL);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "box2Resizes") == 0) {
			self->bBox2Resizes = (pyValue == Py_True);
			gtk_container_child_set(GTK_CONTAINER(self->gtk), self->pyBox2->gtk, "resize", self->bBox2Resizes, NULL);
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
	gtk_paned_set_position(self->gtk, self->iPosition);
	//PxWidget_RepositionChildren(self);
	return 0;
}

bool
PxSplitter_RepositionChildren(PxSplitterObject* self)
{
	PyObject* pyResult;
	if (!PxWidget_RepositionChildren(self->pyBox1))
		return false;
	if (!PxWidget_RepositionChildren(self->pyBox2))
		return false;
	return true;
}

static gboolean
NotifyPositionEventCB(GObject* gObject, GParamSpec* gParamSpec, gpointer pUserData)
{
	((PxSplitterObject*)pUserData)->iPosition = gtk_paned_get_position(GTK_PANED(gObject));
	return PxSplitter_RepositionChildren((PxSplitterObject*)pUserData);
}

static void
PxSplitter_dealloc(PxSplitterObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	Py_XDECREF(self->pyBox1);
	Py_XDECREF(self->pyBox2);
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxSplitter_members[] = {
	{ "box1", T_OBJECT_EX, offsetof(PxSplitterObject, pyBox1), READONLY, "Box 1 widget (for use as parent)" },
	{ "box2", T_OBJECT_EX, offsetof(PxSplitterObject, pyBox2), READONLY, "Box 2 widget (for use as parent)" },
	{ "vertical", T_BOOL, offsetof(PxSplitterObject, bVertical), 0, "Split vertically." },
	{ "box1Resize", T_BOOL, offsetof(PxSplitterObject, bBox1Resizes), READONLY,  "Box 1 resizes with window." },
	{ "box2Resize", T_BOOL, offsetof(PxSplitterObject, bBox2Resizes), READONLY,  "Box 2 resizes with window." },
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
