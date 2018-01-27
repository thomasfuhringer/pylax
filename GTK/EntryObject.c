// EntryObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

static void GtkEntry_ChangedCB(GtkEditable* gtkEditable, gpointer gUserData);
static void GtkEntry_IconPressCB(GtkEntry* gtkEntry, GtkEntryIconPosition gtkEntryIconPosition, GdkEvent* gdkEvent, gpointer gUserData);
static gboolean GtkEntry_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData);
//static void GtkEntry_DestroyCB(GtkWidget* gtkWidget, gpointer gUserData);

static PyObject*
PxEntry_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxEntryObject* self = (PxEntryObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyOnClickButtonCB = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxEntry_init(PxEntryObject* self, PyObject* args, PyObject* kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;

	self->gtk = gtk_entry_new();
	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);
	g_signal_connect(G_OBJECT(self->gtk), "changed", G_CALLBACK(GtkEntry_ChangedCB), (gpointer)self);
	gtk_widget_set_events(GTK_WIDGET(self->gtk), GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(self->gtk), "focus-in-event", G_CALLBACK(GtkEntry_FocusInEventCB), (gpointer)self);

	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	PxWidget_Reposition(self);
	g_object_set_qdata(G_OBJECT(self->gtk), g.gQuark, (gpointer)self);

	gtk_entry_set_alignment(self->gtk, (self->pyDataType == &PyUnicode_Type) ? 0 : 1);

	if (gArgs.pyCaption != NULL)
		gtk_entry_set_placeholder_text(self->gtk, PyUnicode_AsUTF8(gArgs.pyCaption));
	if (self->pyDynaset)
		gtk_widget_set_sensitive(self->gtk, false);
	if (self->pyParent == NULL)
		gtk_widget_hide(self->gtk);
	else
		gtk_widget_show(self->gtk);

	return 0;
}

bool
PxEntry_RenderData(PxEntryObject* self, bool bFormat)
{
	if (self->pyData == NULL || self->pyData == Py_None) {
		gtk_entry_set_text(self->gtk, "");
		return true;
	}

	PyObject* pyText = PxFormatData(self->pyData, bFormat ? self->pyFormat : (self->pyFormatEdit ? self->pyFormatEdit : Py_None));
	if (pyText == NULL) {
		return false;
	}

	gtk_entry_set_text(self->gtk, PyUnicode_AsUTF8(pyText));
	Py_DECREF(pyText);
	return true;
}

bool
PxEntry_SetData(PxEntryObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return true;

	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return false;

	return PxEntry_RenderData(self, true);
}

static PyObject*  // new ref
PxEntry_refresh(PxEntryObject* self)
{
	if (self->pyDynaset == NULL || !self->bClean)
		Py_RETURN_TRUE;

	if (self->pyDynaset->nRow == -1) {
		if (self->pyData != Py_None) {
			PxAttachObject(&self->pyData, Py_None, true);
			if (!PxEntry_RenderData(self, true))
				Py_RETURN_FALSE;
		}
		gtk_widget_set_sensitive(self->gtk, false);
	}
	else {
		PyObject* pyData = PxWidget_PullData((PxWidgetObject*)self);
		if (pyData == NULL)
			Py_RETURN_FALSE;
		if (!PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {
			PxAttachObject(&self->pyData, pyData, true);
			if (!PxEntry_RenderData(self, true))
				Py_RETURN_FALSE;
		}
		gtk_widget_set_sensitive(self->gtk, !(self->bReadOnly || self->pyDynaset->bLocked));
	}
	Py_RETURN_TRUE;
}

static PyObject*  // new ref
PxEntry_get_input_string(PxEntryObject* self)
{
	PyObject* pyInput;
	const gchar* sText = gtk_entry_get_text(self->gtk);
	if (strlen(sText) > 0)
		pyInput = PyUnicode_FromString(sText);
	else
		pyInput = PyUnicode_New(0, 0);
	return pyInput;
}

static PyObject*  // new ref
PxEntry_get_input_data(PxEntryObject* self)
{
	PyObject* pyData;
	const gchar* sText = gtk_entry_get_text(self->gtk);
	if ((pyData = PxParseString(sText, self->pyDataType, NULL)) == NULL) {
		PythonErrorDialog();
		Py_RETURN_NONE;
	}
	return pyData;
}

static gboolean
GtkEntry_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData)
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

	if (!PxEntry_RenderData(self, false)) // render unformatted
		PythonErrorDialog();
	return FALSE; // to propagate the event further
}

static void
GtkEntry_ChangedCB(GtkEditable* gtkEditable, gpointer gUserData)
{
	PxEntryObject* self = (PxEntryObject*)gUserData;
	if (self->pyWindow->pyFocusWidget != self)
		return;

	if (self->bClean) {
		self->bClean = false;
		if (self->pyDynaset) {
			PxDynaset_Freeze(self->pyDynaset);
			if (self->pyDynaset->pySaveButton) {
				gtk_widget_set_sensitive(self->pyDynaset->pySaveButton->gtk, TRUE);
			}
		}
	}
}

static void
GtkEntry_IconPressCB(GtkEntry* gtkEntry, GtkEntryIconPosition gtkEntryIconPosition, GdkEvent* gdkEvent, gpointer gUserData)
{
	PxEntryObject* self = (PxEntryObject*)gUserData;

	int iR = PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)NULL);
	if (iR == -1)
		goto ERROR;
	else if (iR == false)
		return;

	if (self->pyOnClickButtonCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		Py_INCREF(self);
		PyObject* pyResult = PyObject_CallObject(self->pyOnClickButtonCB, pyArgs);
		if (pyResult == NULL)
			goto ERROR;
		Py_DECREF(pyResult);
	}
	return;

ERROR:
	PythonErrorDialog();
}

static PyObject*  // new ref, True if possible to move focus away
PxEntry_render_focus(PxEntryObject* self)
{
	PyObject* pyData = NULL, *pyResult = NULL;

	if (self->bClean) {
		Py_RETURN_TRUE;
	}

	pyData = PxEntry_get_input_data(self);
	if (pyData == NULL)
		return NULL;
	if (PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {  // no true changes were made
		Py_DECREF(pyData);
		PxDynaset_Thaw(self->pyDynaset);
		self->bClean = true;
		Py_RETURN_TRUE;
	}

	if (self->pyValidateCB) {
		PyObject* pyArgs = PyTuple_Pack(2, (PyObject*)self, pyData);
		pyResult = PyObject_CallObject(self->pyValidateCB, pyArgs);
		Py_DECREF(pyArgs);
		if (pyResult == NULL)
			return NULL;
		else if (pyResult == Py_False) { // "validate" function of Widget is supposed to return False if data declined.
			gtk_widget_grab_focus(self->gtk);
			return pyResult;
		}
		else if (pyResult == PyExc_Warning) { // callback has taken care of populating widget
	        self->bClean = true;
		Py_XDECREF(pyResult);
	        pyResult = PxEntry_refresh(self);
		if (pyResult == NULL)
			return NULL;
		Py_XDECREF(pyResult);
	        Py_RETURN_TRUE;
		}
		else if (pyResult != Py_True) {
			PyErr_SetString(PyExc_RuntimeError, "'verify' function of Widget has to return a boolean.");
			return NULL;
		}
		Py_XDECREF(pyResult);
	}

	if (!PxEntry_SetData(self, pyData))
		return NULL;

	self->bClean = true;
	Py_RETURN_TRUE;
}

static int
PxEntry_setattro(PxEntryObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			if (!PxEntry_SetData((PxEntryObject*)self, pyValue))
				return -1;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			gtk_entry_set_placeholder_text(self->gtk, PyUnicode_AsUTF8(pyValue));
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "alignHoriz") == 0) {
			gfloat fXalign;
			switch (PyLong_AsLong(PyObject_GetAttrString(pyValue, "value")))
			{
			case ALIGN_LEFT:
				fXalign = 0;
				break;
			case ALIGN_RIGHT:
				fXalign = 1;
				break;
			case ALIGN_CENTER:
				fXalign = 0.5;
				break;
			}
			gtk_entry_set_alignment(self->gtk, fXalign);
			self->pyAlignHorizontal = pyValue;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click_button") == 0) {
			if (PyCallable_Check(pyValue)) {
				if (self->pyOnClickButtonCB == NULL) {
					gtk_entry_set_icon_from_stock(GTK_ENTRY(self->gtk), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_FIND);
					g_signal_connect(G_OBJECT(self->gtk), "icon-press", G_CALLBACK(GtkEntry_IconPressCB), self);
				}
				else if (pyValue == Py_None) {
					gtk_entry_set_icon_from_stock(GTK_ENTRY(self->gtk), GTK_ENTRY_ICON_SECONDARY, NULL);
				}
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyOnClickButtonCB);
				self->pyOnClickButtonCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assign a callable!");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "input") == 0) {
			if (PyUnicode_Check(pyValue)) {
				gtk_entry_set_text(self->gtk, PyUnicode_AsUTF8(pyValue));
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assign a str!");
				return -1;
			}
		}
	}
	return PxEntryType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxEntry_getattro(PxEntryObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			Py_INCREF(self->pyData);
			return self->pyData;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			PyErr_Clear();
			PyObject* pyText = PyUnicode_FromString(gtk_entry_get_placeholder_text(self->gtk));
			return pyText;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "inputString") == 0) {
			PyErr_Clear();
			return PxEntry_get_input_string(self);
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "inputData") == 0) {
			PyErr_Clear();
			return PxEntry_get_input_data(self);
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxEntry_dealloc(PxEntryObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	Py_XDECREF(self->pyOnClickButtonCB);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxEntry_members[] = {
	{ NULL }
};

static PyMethodDef PxEntry_methods[] = {
	{ "refresh", (PyCFunction)PxEntry_refresh, METH_NOARGS, "Pull fresh data" },
	{ "render_focus", (PyCFunction)PxEntry_render_focus, METH_NOARGS, "Return True if ready for focus to move on." },
	{ NULL }
};

PyTypeObject PxEntryType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Entry",             /* tp_name */
	sizeof(PxEntryObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxEntry_dealloc, /* tp_dealloc */
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
	PxEntry_getattro,          /* tp_getattro */
	PxEntry_setattro,          /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Data entry field",        /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxEntry_methods,           /* tp_methods */
	PxEntry_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxEntry_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxEntry_new,               /* tp_new */
};
