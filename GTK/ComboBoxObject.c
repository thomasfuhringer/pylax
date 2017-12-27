// ComboBoxObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

static void GtkComboBox_ChangedCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer pUserData);
static gboolean GtkComboBox_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData);

static PyObject*
PxComboBox_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxComboBoxObject* self = (PxComboBoxObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyItems = NULL;
		self->bNoneSelectable = true;
		return (PyObject*)self;
	}
	else
		return NULL;
}

//extern PyTypeObject PxWidgetType;

//static bool PxComboBox_SetLabel(PxComboBoxObject* self, PyObject* pyLabel);

static int
PxComboBox_init(PxComboBoxObject *self, PyObject *args, PyObject *kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	g_debug("PxComboBox_init %d", self->bClean);
	self->gtk = gtk_combo_box_text_new();
	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);

	if (self->pyDynaset)
		gtk_widget_set_sensitive(self->gtk, false);
	if (self->pyParent == NULL)
		gtk_widget_hide(self->gtk);
	else
		gtk_widget_show(self->gtk);

	if ((self->pyItems = PyList_New(0)) == NULL) {
		return -1;
	}

	gtk_widget_set_events(GTK_WIDGET(self->gtk), GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(self->gtk), "focus-in-event", G_CALLBACK(GtkComboBox_FocusInEventCB), (gpointer)self);
	g_signal_connect(G_OBJECT(self->gtk), "changed", G_CALLBACK(GtkComboBox_ChangedCB), self);
	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}

static PyObject*
PxComboBox_append(PxComboBoxObject* self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "value", "key", NULL };
	PyObject* pyValue = NULL, *pyKey = NULL, *pyItem = NULL, *pyRepr = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist,
		&pyValue,
		&pyKey))
		return NULL;

	if (pyKey == NULL)
		pyKey = pyValue;

	pyItem = PyTuple_Pack(2, pyValue, pyKey);
	if (PyList_Append(self->pyItems, pyItem) == -1)
		return NULL;

	if (PyUnicode_Check(pyValue)) {
		pyRepr = pyValue;
		Py_INCREF(pyRepr);
	}
	else
		pyRepr = PyObject_Repr(pyValue);

	gtk_combo_box_text_append(self->gtk, NULL, PyUnicode_AsUTF8(pyRepr));
	Py_DECREF(pyRepr);
	Py_RETURN_NONE;
}

static gboolean
GtkComboBox_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	PxEntryObject* self = (PxEntryObject*)gUserData;

	if (self->pyWindow->pyFocusWidget == (PxWidgetObject*)self)
		return FALSE;

	int iR = PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)self);
	if (iR == -1) {
		PythonErrorDialog();
		return TRUE;
	}
	else if (iR == false)
		return FALSE;

	return FALSE; // to propagate the event further
}

static void
GtkComboBox_ChangedCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer pUserData)
{
	PyObject* pyItem, *pyData;
	PxComboBoxObject* self = (PxComboBoxObject*)pUserData;
	gint iItem = gtk_combo_box_get_active(gtkWidget);
	if (iItem == -1)
		pyData = Py_None;
	else {
		pyItem = PyList_GetItem(self->pyItems, iItem);
		if (pyItem == NULL) {
			PythonErrorDialog();
			return;
		}
		pyData = PyTuple_GET_ITEM(pyItem, 1);
	}
	PxAttachObject(&self->pyData, pyData, true);
}

bool
PxComboBox_RenderData(PxComboBoxObject* self, bool bFormat)
{

	if (self->pyData == NULL || self->pyData == Py_None) {
		gtk_combo_box_set_active(self->gtk, -1);
		return true;
	}

	Py_ssize_t nIndex;
	Py_ssize_t nLen = PySequence_Size(self->pyItems);
	bool bFound = false;
	PyObject* pyItem, *pyKey;
	for (nIndex = 0; nIndex < nLen; nIndex++) {
		pyItem = PyList_GetItem(self->pyItems, nIndex);
		pyKey = PyTuple_GET_ITEM(pyItem, 1);
		if (PyObject_RichCompareBool(pyKey, self->pyData, Py_EQ)) {
			bFound = true;
			break;
		}
	}
	if (bFound) {
		gtk_combo_box_set_active(self->gtk, nIndex);
	}
	else {
		g_message("Warning - Key '%.200s' is not in the data of ComboBox.", self->pyData);
		gtk_combo_box_set_active(self->gtk, -1);
	}
	return true;
}

bool
PxComboBox_SetData(PxComboBoxObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return true;

	Py_ssize_t nIndex;
	if (self->bNoneSelectable && pyData == Py_None)
		nIndex = -1;
	else {
		Py_ssize_t nLen = PySequence_Size(self->pyItems);
		bool bFound = false;
		PyObject* pyItem, *pyKey;
		for (nIndex = 0; nIndex < nLen; nIndex++) {
			pyItem = PyList_GetItem(self->pyItems, nIndex);
			pyKey = PyTuple_GET_ITEM(pyItem, 1);
			if (PyObject_RichCompareBool(pyKey, pyData, Py_EQ)) {
				bFound = true;
				break;
			}
		}

		if (!bFound) {
			PyObject* pyRepr = PyObject_Repr(pyData);
			PyErr_Format(PyExc_AttributeError, "Value %s is not among selectable items in ComboBox.", PyUnicode_AsUTF8(pyRepr));
			Py_XDECREF(pyRepr);
			return false;
		}
	}

	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return false;

	gtk_combo_box_set_active(self->gtk, nIndex);
	return true;
}

static PyObject *
PxComboBox_refresh(PxComboBoxObject* self)
{
	//g_debug("PxComboBox_refresh");
	if (self->pyDynaset == NULL || !self->bClean)
		Py_RETURN_TRUE;

	if (self->pyDynaset->nRow == -1) {
		if (self->pyData != Py_None) {
			PxAttachObject(&self->pyData, Py_None, true);
			if (!PxComboBox_RenderData(self, true))
				Py_RETURN_FALSE;
		}
		gtk_widget_set_sensitive(self->gtk, false);
	}
	else {
		PyObject* pyData = PxWidget_PullData((PxWidgetObject*)self);
		if (!PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {
			PxAttachObject(&self->pyData, pyData, true);
			if (!PxComboBox_RenderData(self, true))
				Py_RETURN_FALSE;
		}
		gtk_widget_set_sensitive(self->gtk, !(self->bReadOnly || self->pyDynaset->bLocked));
	}
	Py_RETURN_TRUE;
}

static int
PxComboBox_setattro(PxComboBoxObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		/*
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "label") == 0) {
		return PxComboBox_SetLabel((PxComboBoxObject*)self, pyValue) ? 0 : -1;
		}*/
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			if (!PxComboBox_SetData(self, pyValue))
				return -1;
			return 0;
		}
	}
	return PyObject_GenericSetAttr((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxComboBox_getattro(PxComboBoxObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			Py_INCREF(self->pyData);
			return self->pyData;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxComboBox_dealloc(PxComboBoxObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxComboBox_members[] = {
	{ "noneSelectable", T_BOOL, offsetof(PxComboBoxObject, bNoneSelectable), 0, "Is it possible to make no selection?" },
	{ NULL }
};

static PyMethodDef PxComboBox_methods[] = {
	{ "refresh", (PyCFunction)PxComboBox_refresh, METH_NOARGS, "Pull fresh data" },
	{ "append", (PyCFunction)PxComboBox_append, METH_VARARGS | METH_KEYWORDS, "Append selectable item." },
	{ NULL }
};

PyTypeObject PxComboBoxType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.ComboBox",          /* tp_name */
	sizeof(PxComboBoxObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxComboBox_dealloc, /* tp_dealloc */
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
	PxComboBox_getattro,       /* tp_getattro */
	PxComboBox_setattro,       /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"ComboBox objects",        /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxComboBox_methods,        /* tp_methods */
	PxComboBox_members,        /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxComboBox_init, /* tp_init */
	0,                         /* tp_alloc */
	PxComboBox_new,            /* tp_new */
};
