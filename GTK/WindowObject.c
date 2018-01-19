// WindowObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

gboolean GtkWindow_ConfigureEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData);
static gboolean GtkWindow_DeleteEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData);

static PyObject *
PxWindow_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxWindowObject* self = (PxWindowObject*)PxWindowType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->iMinX = 320;
		self->iMinY = 240;
		//self->bTable = false;
		self->bNameInCaption = true;
		self->pyFocusWidget = NULL;
		self->pyBeforeCloseCB = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxWindow_init(PxWindowObject* self, PyObject* args, PyObject* kwds)
{
	if (PxWindowType.tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;
	self->pyWindow = self;

	self->pyConnection = g.pyConnection;
	Py_INCREF(self->pyConnection);
	self->pyName = gArgs.pyCaption;
	Py_INCREF(self->pyName);
	g_debug("main.py running.");

	if (!PxWindow_RestoreState((PxWindowObject*)self))
		return -1;

	// rest of initialization only if not subclass
	if (Py_TYPE(self) != &PxWindowType)
		return 0;

	self->gtk = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_application(GTK_WINDOW(self->gtk), g.gtkApp);
	gtk_window_set_default_size(GTK_WINDOW(self->gtk), self->rc.iWidth, self->rc.iHeight);
	gtk_window_set_modal(GTK_WINDOW(self->gtk),TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(self->gtk),g.gtkMainWindow);
	g_signal_connect(G_OBJECT(self->gtk), "configure-event", G_CALLBACK(GtkWindow_ConfigureEventCB), self);
	g_signal_connect(G_OBJECT(self->gtk), "delete-event", G_CALLBACK(GtkWindow_DeleteEventCB), self);
	gtk_window_move(GTK_WINDOW(self->gtk), self->rc.iLeft, self->rc.iTop);
	self->gtkFixed = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(self->gtk), self->gtkFixed);
	gtk_widget_show_all(self->gtkFixed);

	if (gArgs.pyCaption != NULL)
		if (!PxWindow_SetCaption(self, gArgs.pyCaption))
			return -1;

	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}

gboolean
GtkWindow_ConfigureEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	PxWindowObject* self = (PxWindowObject*)gUserData;
	if (self)
		if (!PxWidget_RepositionChildren(self)) {
			PythonErrorDialog();
			return TRUE;
		}
	return FALSE;
}

bool
PxWindow_SetCaption(PxWindowObject* self, PyObject* pyText)
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

bool
PxWindow_SaveState(PxWindowObject* self)
{
	if (g.bConnectionHasPxTables) {

		gint x, y, width, height;
		PyObject* pyCursor = NULL, *pyResult = NULL, *pyParameters = NULL;

		gtk_window_get_position(self->gtk, &x, &y);
		gtk_window_get_size(self->gtk, &width, &height);

		if ((pyCursor = PyObject_CallMethod(g.pyConnection, "cursor", NULL)) == NULL) {
			return FALSE;
		}
		pyParameters = PyTuple_Pack(1, self->pyName);
		if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "SELECT PxWindowID FROM PxWindow WHERE PxWindowID=?;", pyParameters)) == NULL) {
			return FALSE;
		}
		Py_DECREF(pyParameters);
		if ((pyResult = PyObject_CallMethod(pyCursor, "fetchone", NULL)) == NULL) {
			return FALSE;
		}
		bool bEntryExists = pyResult == Py_None ? FALSE : TRUE;
		Py_DECREF(pyResult);

		pyParameters = Py_BuildValue("(iiiiiO)", x, y, width, height, g.iCurrentUser, self->pyName);
		if (bEntryExists) {
			if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "UPDATE PxWindow SET Left=?,Top=?,Width=?,Height=?,ModUser=? WHERE PxWindowID=?;", pyParameters)) == NULL)
				return FALSE;
		}
		else {
			if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "INSERT INTO PxWindow (Left,Top,Width,Height,ModUser,PxWindowID) VALUES (?,?,?,?,?,?);", pyParameters)) == NULL)
				return FALSE;
		}
		Py_DECREF(pyResult);
		if ((pyResult = PyObject_CallMethod(g.pyConnection, "commit", NULL)) == NULL) {
			return FALSE;
		}
		if ((pyCursor && PyObject_CallMethod(pyCursor, "close", NULL)) == NULL) {
			return FALSE;
		}
		Py_DECREF(pyParameters);
		Py_DECREF(pyResult);
		Py_DECREF(pyCursor);
	}
	return TRUE;
}

bool
PxWindow_RestoreState(PxWindowObject* self)
{
	if (g.bConnectionHasPxTables) {
		PyObject* pyCursor = NULL, *pyResult = NULL, *pyParameters = NULL, *pyValue = NULL;

		if ((pyCursor = PyObject_CallMethod(g.pyConnection, "cursor", NULL)) == NULL) {
			return FALSE;
		}
		pyParameters = PyTuple_Pack(1, self->pyName);
		if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "SELECT Left,Top,Width,Height FROM PxWindow WHERE PxWindowID=?;", pyParameters)) == NULL) {
			return FALSE;
		}
		Py_DECREF(pyResult);
		if ((pyResult = PyObject_CallMethod(pyCursor, "fetchone", NULL)) == NULL) {
			return FALSE;
		}

		if (pyResult != Py_None) {
			pyValue = PyTuple_GetItem(pyResult, 0);
			self->rc.iLeft = PyLong_AsLong(pyValue);
			pyValue = PyTuple_GetItem(pyResult, 1);
			self->rc.iTop = PyLong_AsLong(pyValue);
			pyValue = PyTuple_GetItem(pyResult, 2);
			self->rc.iWidth = PyLong_AsLong(pyValue);
			pyValue = PyTuple_GetItem(pyResult, 3);
			self->rc.iHeight = PyLong_AsLong(pyValue);
		}
		if ((pyCursor && PyObject_CallMethod(pyCursor, "close", NULL)) == NULL) {
			return FALSE;
		}

		Py_DECREF(pyParameters);
		Py_DECREF(pyResult);
		Py_DECREF(pyCursor);
	}
	return true;
}

int
PxWindow_MoveFocus(PxWindowObject* self, PxWidgetObject* pyDestinationWidget)
// attempt to move the PxWindow's input focus widget to given one
// return true if possible, false if not, -1 on error
{
	if (self->pyFocusWidget == pyDestinationWidget)
		return true;
	if (self->pyFocusWidget == NULL) {
		self->pyFocusWidget = pyDestinationWidget;
		return true;
	}

	PyObject* pyResult = PyObject_CallMethod((PyObject*)self->pyFocusWidget, "render_focus", NULL);
	if (pyResult == NULL)
		return -1;
	int bResult = pyResult == Py_True ? true : false;
	Py_DECREF(pyResult);
	if (bResult)
		self->pyFocusWidget = pyDestinationWidget;
	return bResult;
}

PyObject*
PxWindow_set_icon_from_file(PxWindowObject* self, PyObject* args, PyObject* kwds)
{
	static char* kwlist[] = { "fileName", NULL };
	const char* sFileName;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist,
		&sFileName))
		return NULL;

	gboolean iResult = gtk_window_set_icon_from_file(self->gtk, sFileName, NULL);
	if (!iResult) {
		PyErr_Format(PyExc_RuntimeError, "Setting window icon to '%.200s' failed.", sFileName);
		return NULL;
	}
	Py_RETURN_TRUE;
}

static gboolean
GtkWindow_DeleteEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	PxWindowObject* self = (PxWindowObject*)gUserData;

	if (self->pyBeforeCloseCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		Py_INCREF(self);
		PyObject* pyResult = PyObject_CallObject(self->pyBeforeCloseCB, pyArgs);
		if (pyResult == NULL) {
			PythonErrorDialog();
			return TRUE;
		}
		else {
			Py_DECREF(pyResult);
		}
	}
	return FALSE;
}

static int
PxWindow_setattro(PxWindowObject* self, PyObject* pyAttributeName, PyObject* pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		/*
			if (PyUnicode_CompareWithASCIIString(pyAttributeName, "menu") == 0 && PyObject_TypeCheck((PyObject *)self, &PxWindowType)) {
				if (!PyObject_TypeCheck(pyValue, &PxMenuType)) {
					PyErr_SetString(PyExc_TypeError, "Assigned value is not a Menu.");
					return -1;
				}
				if (SetMenu(self->hWin, ((PxMenuObject*)pyValue)->hWin) == 0)
					return -1;
				DrawMenuBar(self->hWin);
				Py_INCREF(pyValue);
				return 0;
			}
			*/
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			return PxWindow_SetCaption(self, pyValue) ? 0 : -1;
		}

		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			// assigning a key dict here should make the form jump to the record if it is bound to a Dynaset
			PyErr_SetString(PyExc_NotImplementedError, "Searching by assigning to 'data' property still not available.");
			return -1;
		}

		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "before_close") == 0) {
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyBeforeCloseCB);
				self->pyBeforeCloseCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			if (PyBool_Check(pyValue)){
			    if (pyValue == Py_True)
		            gtk_widget_show(self->gtk);
	            else
		            gtk_widget_hide(self->gtk);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be bool");
				return -1;
			}
		}
	}
	return PxWindowType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxWindow_getattro(PxWindowObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			if (self->pyDynaset != NULL && self->pyDynaset->nRow > -1)
				return PxDynaset_GetRowDataDict(self->pyDynaset, self->pyDynaset->nRow, false);
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "before_close") == 0) {
			PyErr_Clear();
			Py_INCREF(self->pyBeforeCloseCB);
			return self->pyBeforeCloseCB;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "focus") == 0) {
			PyErr_Clear();
			if (self->pyFocusWidget) {
				Py_INCREF(self->pyFocusWidget);
				return (PyObject*)self->pyFocusWidget;
			}
			Py_RETURN_NONE;
		}
	}
	return PxWindowType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxWindow_dealloc(PxWindowObject* self)
{
	Py_XDECREF(self->pyConnection);
	Py_XDECREF(self->pyName);
	Py_XDECREF(self->pyBeforeCloseCB);
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxWindow_members[] = {
	{ "cnx", T_OBJECT, offsetof(PxWindowObject, pyConnection), READONLY, "Database connection" },
	{ "minWidth", T_INT, offsetof(PxWindowObject, iMinX), 0, "Window can not be resized smaller" },
	{ "minHeight", T_INT, offsetof(PxWindowObject, iMinY), 0, "Window can not be resized smaller" },
	{ "nameInCaption", T_BOOL, offsetof(PxWindowObject, bNameInCaption), 0, "Show name of the Window in the caption" },
	//{ "buttonCancel", T_OBJECT, offsetof(PxWindowObject, pyCancelButton), 0, "Close the dialog" },
	//{ "buttonOK", T_OBJECT, offsetof(PxWindowObject, pyOkButton), 0, "Close the dialog" },
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxWindow_methods[] = {
	{ "set_icon_from_file", (PyCFunction)PxWindow_set_icon_from_file, METH_VARARGS | METH_KEYWORDS, "Set the window icon from filename given." },
	//{ "set_focus", (PyCFunction)PxWindow_set_focus, METH_VARARGS | METH_KEYWORDS, "Set the focus to given widget." },
	{ NULL }
};

PyTypeObject PxWindowType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Window",            /* tp_name */
	sizeof(PxWindowObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxWindow_dealloc, /* tp_dealloc */
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
	PxWindow_getattro,         /* tp_getattro */
	PxWindow_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Simply a window",         /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxWindow_methods,          /* tp_methods */
	PxWindow_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxWindow_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxWindow_new,              /* tp_new */
};
