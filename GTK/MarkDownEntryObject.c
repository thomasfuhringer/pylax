// MarkDownEntryObject.c  | Pylax © 2018 by Thomas Führinger
#include "Pylax.h"

static void GtkTextBuffer_ChangedCB(GtkTextView* gtkTextView, gpointer gUserData);
static gboolean GtkWidget_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData);

static PyObject*
PxMarkDownEntry_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxMarkDownEntryObject* self = (PxMarkDownEntryObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->bPlain = false;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxMarkDownEntry_init(PxMarkDownEntryObject* self, PyObject* args, PyObject* kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;

	self->gtk = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->gtk), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	self->gtkTextBuffer = gtk_text_buffer_new(NULL);
	GtkWidget* gtkTextView = gtk_text_view_new_with_buffer(self->gtkTextBuffer);
	gtk_container_add(GTK_CONTAINER(self->gtk), gtkTextView);

	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);
	g_signal_connect(G_OBJECT(self->gtkTextBuffer), "changed", G_CALLBACK(GtkTextBuffer_ChangedCB), (gpointer)self);
	gtk_widget_set_events(GTK_WIDGET(self->gtk), GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(gtkTextView), "focus-in-event", G_CALLBACK(GtkWidget_FocusInEventCB), (gpointer)self);

	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	PxWidget_Reposition(self);
	g_object_set_qdata(G_OBJECT(self->gtk), g.gQuark, (gpointer)self);


	if (self->pyDynaset){
		gtk_widget_set_sensitive(self->gtk, false);
		self->bSensitive = false;
		}
	if (self->pyParent == NULL)
		gtk_widget_hide(self->gtk);
	else
		gtk_widget_show_all(self->gtk);

	return 0;
}

bool
PxMarkDownEntry_RenderData(PxMarkDownEntryObject* self)
{
	if (self->bSensitive || self->bPlain) {
		if (self->pyData == NULL || !PyUnicode_Check(self->pyData)) {
			gtk_text_buffer_set_text(self->gtkTextBuffer, "", -1);
		}
		else {
			gtk_text_buffer_set_text(self->gtkTextBuffer, PyUnicode_AsUTF8(self->pyData), -1);
		}
	}
	else {
		if (self->pyData == NULL || !PyUnicode_Check(self->pyData)) {
			gtk_text_buffer_set_markup(self->gtkTextBuffer, "");
		}
		else {
			gtk_text_buffer_set_markup(self->gtkTextBuffer, PyUnicode_AsUTF8(self->pyData));
		}
	}
	return true;
}

bool
PxMarkDownEntry_SetData(PxMarkDownEntryObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return true;

	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return false;

	return PxMarkDownEntry_RenderData(self);
}

static PyObject*  // new ref
PxMarkDownEntry_refresh(PxMarkDownEntryObject* self)
{
	if (self->pyDynaset == NULL || !self->bClean)
		Py_RETURN_TRUE;

	if (self->pyDynaset->nRow == -1) {
		gtk_widget_set_sensitive(self->gtk, false);
		self->bSensitive = false;
		if (self->pyData != Py_None) {
			PxAttachObject(&self->pyData, Py_None, true);
			if (!PxMarkDownEntry_RenderData(self))
				Py_RETURN_FALSE;
		}
	}
	else {
		PyObject* pyData = PxWidget_PullData((PxWidgetObject*)self);
		if (pyData == NULL)
			Py_RETURN_FALSE;

		self->bSensitive = !self->pyDynaset->bLocked;
		gtk_widget_set_sensitive(self->gtk, !(self->bReadOnly || self->pyDynaset->bLocked));
		if (!PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {
			PxAttachObject(&self->pyData, pyData, true);
			if (!PxMarkDownEntry_RenderData(self))
				Py_RETURN_FALSE;
		}
	}
	Py_RETURN_TRUE;
}

static PyObject*  // new ref
PxMarkDownEntry_get_input_string(PxMarkDownEntryObject* self)
{
	PyObject* pyInput;
	GtkTextIter gtkTextIterStart, gtkTextIterEnd;

	gtk_text_buffer_get_bounds(self->gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	const gchar* sText = gtk_text_buffer_get_text(self->gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd, TRUE);

	if (strlen(sText) > 0)
		pyInput = PyUnicode_FromString(sText);
	else
		pyInput = PyUnicode_New(0, 0);
	return pyInput;
}

static gboolean
GtkWidget_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	PxMarkDownEntryObject* self = (PxMarkDownEntryObject*)gUserData;

	if (self->pyWindow->pyFocusWidget == (PxWidgetObject*)self)
		return FALSE;

	int iR = PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)self);
	if (iR == -1) {
		PythonErrorDialog();
		return TRUE;
	}
	else if (iR == false)
		return FALSE;

	//if (!PxMarkDownEntry_RenderData(self)) // render unformatted
	//	PythonErrorDialog();
	return FALSE; // to propagate the event further
}

static void
GtkTextBuffer_ChangedCB(GtkTextView* gtkTextView, gpointer gUserData)
{
	PxMarkDownEntryObject* self = (PxMarkDownEntryObject*)gUserData;
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

static PyObject*  // new ref, True if possible to move focus away
PxMarkDownEntry_render_focus(PxMarkDownEntryObject* self)
{
	PyObject* pyData = NULL, *pyResult = NULL;

	if (self->bClean) {
		Py_RETURN_TRUE;
	}

	pyData = PxMarkDownEntry_get_input_string(self);
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
			pyResult = PxMarkDownEntry_refresh(self);
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

	if (!PxMarkDownEntry_SetData(self, pyData))
		return NULL;

	self->bClean = true;
	Py_RETURN_TRUE;
}

static int
PxMarkDownEntry_setattro(PxMarkDownEntryObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			if (!PxMarkDownEntry_SetData((PxMarkDownEntryObject*)self, pyValue))
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
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "sensitive") == 0) {
			if (PyObject_IsTrue(pyValue)) {
			    self->bSensitive = true;
			    gtk_widget_set_sensitive(self->gtk, !self->bReadOnly);
			}
			else {
			    self->bSensitive = false;
			    gtk_widget_set_sensitive(self->gtk, FALSE);
			    g_debug("insens");
			}
		    if (!PxMarkDownEntry_RenderData(self))
			    return -1;
			return 0;
		}
	}
	return PxMarkDownEntryType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxMarkDownEntry_getattro(PxMarkDownEntryObject* self, PyObject* pyAttributeName)
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
			return PxMarkDownEntry_get_input_string(self);
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "inputData") == 0) {
			PyErr_Clear();
			return PxMarkDownEntry_get_input_string(self);
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "sensitive") == 0) {
			if (self->bSensitive)
			    Py_RETURN_TRUE;
			else
			    Py_RETURN_FALSE;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxMarkDownEntry_dealloc(PxMarkDownEntryObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	g_object_unref(self->gtkTextBuffer);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxMarkDownEntry_members[] = {
	{ "plain", T_BOOL, offsetof(PxMarkDownEntryObject, bPlain), 0, "If True text will be rendered as plain text (without markdown)." },
	{ NULL }
};

static PyMethodDef PxMarkDownEntry_methods[] = {
	{ "refresh", (PyCFunction)PxMarkDownEntry_refresh, METH_NOARGS, "Pull fresh data" },
	{ "render_focus", (PyCFunction)PxMarkDownEntry_render_focus, METH_NOARGS, "Return True if ready for focus to move on." },
	{ NULL }
};

PyTypeObject PxMarkDownEntryType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.MarkDownEntry",     /* tp_name */
	sizeof(PxMarkDownEntryObject), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxMarkDownEntry_dealloc, /* tp_dealloc */
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
	PxMarkDownEntry_getattro,  /* tp_getattro */
	PxMarkDownEntry_setattro,  /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Entry field for MarkDown text", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxMarkDownEntry_methods,   /* tp_methods */
	PxMarkDownEntry_members,   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxMarkDownEntry_init, /* tp_init */
	0,                         /* tp_alloc */
	PxMarkDownEntry_new,       /* tp_new */
};


void
gtk_text_buffer_set_markup(GtkTextBuffer* gtkTextBuffer, char* sMarkup)
{
	egg_markdown_clear(g.pDownmarker);
	egg_markdown_set_extensions(g.pDownmarker, EGG_MARKDOWN_EXTENSION_GITHUB);
	char* sPango = egg_markdown_parse(g.pDownmarker, sMarkup);
	GtkTextIter gtkTextIterStart, gtkTextIterEnd;
	//gtk_text_buffer_set_text(gtkTextBuffer,NULL,0);
	gtk_text_buffer_get_bounds(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	gtk_text_buffer_delete(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	gtk_text_buffer_get_bounds(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	gtk_text_buffer_insert_markup(gtkTextBuffer, &gtkTextIterStart, sPango, -1);
	g_free(sPango);
}
