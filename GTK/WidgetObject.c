// WidgetObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

PxWidgetAddArgs gArgs;

static PyObject*
PxWidget_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxWidgetObject *self = (PxWidgetObject*)PxWidgetType.tp_alloc(type, 0);
	if (self != NULL) {
		self->gtk = NULL;
		self->gtkFixed = NULL;
		self->pyParent = NULL;
		self->bReadOnly = false;
		self->bClean = true;
		self->bTable = false;
		self->bPointer = false;
		self->pyWindow = NULL;
		self->pyData = Py_None;
		Py_INCREF(Py_None);
		self->pyDataType = NULL;
		self->pyFormat = NULL;
		self->pyFormatEdit = NULL;
		self->pyDynaset = NULL;
		self->pyDataColumn = NULL;
		self->pyLabel = NULL;
		self->pyAlignHorizontal = NULL;
		self->pyAlignVertical = NULL;
		self->pyVerifyCB = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxWidget_init(PxWidgetObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "parent", "left", "top", "right", "bottom", "caption", "dynaset", "column", "dataType", "format", "label", "visible", NULL };
	PyObject* pyParent = NULL, *pyCaption = NULL, *pyDataColumn = NULL, *pyFormat = NULL, *pyLabel = NULL, *tmp = NULL;
	PyTypeObject* pyDataType = NULL;
	PxDynasetObject* pyDynaset = NULL;
	gArgs.bVisible = PyObject_TypeCheck(self, &PxWindowType) ? false : true;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|llllOOOOOOp", kwlist,
		&pyParent,
		&self->rc.iLeft,
		&self->rc.iTop,
		&self->rc.iWidth,
		&self->rc.iHeight,
		&pyCaption,
		&pyDynaset,
		&pyDataColumn,
		&pyDataType,
		&pyFormat,
		&pyLabel,
		&gArgs.bVisible))
		return -1;

	if (pyParent && pyParent != Py_None) {
		if (!PyObject_TypeCheck(pyParent, &PxWidgetType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 1 ('parent') must be a widget");
			return -1;
		}
		self->pyParent = (PxWidgetObject*)pyParent;
		self->pyWindow = ((PxWidgetObject*)pyParent)->pyWindow;
	}

	if (pyCaption) {
		if (PyUnicode_Check(pyCaption)) {
			gArgs.pyCaption = pyCaption;
		}
		else {
			PyErr_SetString(PyExc_TypeError, "Parameter 2 ('caption') must be a string.");
			return -1;
		}
	}
	else
		gArgs.pyCaption = NULL;

	if (pyDynaset) {
		tmp = self->pyDynaset;
		if (!PyObject_TypeCheck(pyDynaset, &PxDynasetType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 3 ('dynaset') must be a Dynaset.");
			return -1;
		}
		if (pyDataColumn == NULL && !PyObject_TypeCheck(self, &PxTableType) && !PyObject_TypeCheck(self, &PxFormType) && !PyObject_TypeCheck(self, &PxWindowType)) {
			PyErr_SetString(PyExc_RuntimeError, "No data column provided.");
			return -1;
		}
		Py_INCREF(pyDynaset);
		self->pyDynaset = (PxDynasetObject*)pyDynaset;
		Py_XDECREF(tmp);
		if (!PxDynaset_AddWidget(self->pyDynaset, self))
			return -1;
	}

	tmp = self->pyDataColumn;
	if (pyDataColumn) {
		if (!PyUnicode_Check(pyDataColumn)) {
			PyErr_Format(PyExc_TypeError, "Parameter 4 ('column') must be string, not '%.200s'.", pyDataColumn->ob_type->tp_name);
			return -1;
		}
		else {
			PyObject* pyColumn = PyDict_GetItem(self->pyDynaset->pyColumns, pyDataColumn);
			if (pyColumn == NULL) {
				PyErr_SetString(PyExc_TypeError, "Parameter 4 ('column') does not exist.");
				PyErr_Format(PyExc_TypeError, "Column '%.200s' does not exist in Dynaset '%.200s'.", PyUnicode_AsUTF8(pyDataColumn), PyUnicode_AsUTF8(self->pyDynaset));
				return -1;
			}
			self->pyDataColumn = pyColumn;
			self->pyDataType = (PyTypeObject*)PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_TYPE);
			self->pyFormat = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_FORMAT);
			Py_INCREF(self->pyDataColumn);
			Py_INCREF(self->pyDataType);
			Py_INCREF(self->pyFormat);
			Py_XDECREF(tmp);
		}
	}
	if (pyDataType) {
		if (!PyObject_TypeCheck(pyDataType, &PyType_Type)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 5 ('dataType') must be a data type.");
			return -1;
		}
		else {
			if (self->pyDataColumn && self->pyDataType != pyDataType) {
				PyErr_SetString(PyExc_TypeError, "Parameter 5 ('dataType') is different from bound data column.");
				return -1;
			}
		}
	}
	else
		pyDataType = &PyUnicode_Type;
	tmp = self->pyDataType;
	Py_INCREF(pyDataType);
	self->pyDataType = pyDataType;
	Py_XDECREF(tmp);

	Py_INCREF(Py_None);
	self->pyData = Py_None;

	if (pyFormat) {
		if (!PyUnicode_Check(pyFormat)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 6 ('format') must be a string.");
			return -1;
		}
		else {
			tmp = self->pyFormat;
			Py_INCREF(pyFormat);
			self->pyFormat = pyFormat;
			Py_XDECREF(tmp);
		}
	}
	if (!self->pyFormat) {
		Py_INCREF(Py_None);
		self->pyFormat = Py_None;
	}

	if (pyLabel) {
		if (!PyObject_TypeCheck(pyLabel, &PxLabelType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 7 ('label') must be a Label.");
			return -1;
		}
		else {
			tmp = self->pyLabel;
			Py_INCREF(pyLabel);
			self->pyLabel = (PxLabelObject*)pyLabel;
			Py_XDECREF(tmp);
			tmp = self->pyLabel->pyAssociatedWidget;
			Py_INCREF(self);
			self->pyLabel->pyAssociatedWidget = (PxWidgetObject*)self;
			Py_XDECREF(tmp);
		}
	}
	Py_INCREF(self); // reference for parent
	return 0;
}

bool
PxWidget_CalculateRect(PxWidgetObject* self, Rect* rc)
{
	GtkFixed* gtkFixedParent = gtk_widget_get_parent(self->gtk);
	if (gtkFixedParent == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Can not get parent window.");
		return false;
	}

	long iWidth = gtk_widget_get_allocated_width(gtkFixedParent);
	long iHeight = gtk_widget_get_allocated_height(gtkFixedParent);

	if (self->rc.iLeft == PxWIDGET_CENTER)
		rc->iLeft = (iWidth - self->rc.iWidth) / 2;
	else
		rc->iLeft = (self->rc.iLeft >= 0 ? self->rc.iLeft : iWidth + self->rc.iLeft);
	rc->iWidth = (self->rc.iWidth > 0 ? self->rc.iWidth : iWidth + self->rc.iWidth - rc->iLeft);

	if (self->rc.iTop == PxWIDGET_CENTER)
		rc->iTop = (iHeight - self->rc.iHeight) / 2;
	else
		rc->iTop = (self->rc.iTop >= 0 ? self->rc.iTop : iHeight + self->rc.iTop);
	rc->iHeight = (self->rc.iHeight > 0 ? self->rc.iHeight : iHeight + self->rc.iHeight - rc->iTop);

	rc->iWidth = rc->iWidth < 0 ? 100 : rc->iWidth;
	rc->iHeight = rc->iHeight < 0 ? 100 : rc->iHeight;

	if (rc->iWidth > iWidth)
		g_debug("rc->iWidth ,iWidth %d %d", rc->iWidth, iWidth);
	if (rc->iHeight > iHeight)
		g_debug("rc->iHeight <= iHeight %d %d", rc->iHeight, iHeight);
	g_assert(rc->iWidth <= iWidth);
	g_assert(rc->iHeight <= iHeight);
	return true;
}

bool
PxWidget_Reposition(PxWidgetObject* self)
{
	Rect rc;
	if (!PxWidget_CalculateRect(self, &rc))
		return false;

	if (self->gtk) {
		GtkFixed* gtkFixedParent = gtk_widget_get_parent(self->gtk);
		gtk_fixed_move(gtkFixedParent, self->gtk, rc.iLeft, rc.iTop);
		gtk_widget_set_size_request(self->gtk, rc.iWidth, rc.iHeight);
	}
	return true;
}

bool
PxWidget_RepositionChildren(PxWidgetObject* self)
{
	PxWidgetObject* pyChild;
	if (PyObject_TypeCheck(self, &PxTabType)) {
		if (!PxTab_RepositionChildren(self))
			return false;
	}
	else
		if (PyObject_TypeCheck(self, &PxSplitterType)) {
			if (!PxSplitter_RepositionChildren(self))
				return false;
		}
		else
			if (self->gtkFixed) {
				GList* pGListChildren = gtk_container_get_children(GTK_CONTAINER(self->gtkFixed));
				while (pGListChildren != NULL) {
					pyChild = (PxWidgetObject*)g_object_get_qdata(pGListChildren->data, g.gQuark);
					if (pyChild) {
						if (!PxWidget_Reposition(pyChild))
							return false;
						if (!PxWidget_RepositionChildren(pyChild))
							return false;
					}
					pGListChildren = pGListChildren->next;
				}
				g_list_free(pGListChildren);
			}
	return true;
}

static bool
PxWidget_ReleaseChildren(PxWidgetObject* self)
{
	if (self->gtkFixed) {
		GList* gListChildren = gtk_container_get_children(GTK_CONTAINER(self->gtkFixed));
		while (gListChildren != NULL) {
			PxWidgetObject* pyChild = g_object_get_qdata(gListChildren->data, g.gQuark);

			if (pyChild)
				Py_XDECREF(pyChild);

			gListChildren = gListChildren->next;
		}
		g_list_free(gListChildren);
	}
	return true;
}

static void
PxWidget_dealloc(PxWidgetObject* self)
{
	if (self->gtk)
		gtk_widget_destroy(self->gtk);

	if (self->gtkFixed)
		gtk_widget_destroy(self->gtkFixed);

	Py_XDECREF(self->pyData);
	Py_XDECREF(self->pyDataType);
	Py_XDECREF(self->pyFormat);
	Py_XDECREF(self->pyFormatEdit);
	Py_XDECREF(self->pyAlignHorizontal);
	Py_XDECREF(self->pyAlignVertical);
	Py_XDECREF(self->pyLabel);
	Py_XDECREF(self->pyDynaset);
	Py_XDECREF(self->pyDataColumn);

	Py_TYPE(self)->tp_free((PyObject*)self);

	if (!PxWidget_ReleaseChildren(self))
		return;
}

static PyObject*
PxWidget_pass(PxWidgetObject* self)
// for empty method
{
	Py_RETURN_TRUE;
}

bool
PxWidget_Move(PxWidgetObject* self)
{
	Rect rc;
	PxWidget_CalculateRect(self, &rc);

	if (self->gtk) {
		GtkFixed* gtkFixed = gtk_widget_get_parent(self->gtk);
		gtk_fixed_move(gtkFixed, self->gtk, rc.iLeft, rc.iTop);
		gtk_widget_set_size_request(self->gtk, rc.iWidth, rc.iHeight);
	}

	if (self->gtkFixed) {
		GList* gListChildren = gtk_container_get_children(GTK_CONTAINER(self->gtkFixed));
		while (gListChildren != NULL) {
			PxWidgetObject* pyChild = g_object_get_qdata(gListChildren->data, g.gQuark);
			gListChildren = gListChildren->next;
		}
		g_list_free(gListChildren);
	}
	return true;
}

static PyObject* // new ref
PxWidget_move(PxWidgetObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "left", "top", "right", "bottom", NULL };
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iiii", kwlist,
		&self->rc.iLeft,
		&self->rc.iTop,
		&self->rc.iWidth,
		&self->rc.iHeight))
		return NULL;

	if (PxWidget_Move(self))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

PyObject* // borrowed ref
PxWidget_PullData(PxWidgetObject* self)
{
	if (self->pyDynaset && self->pyDataColumn) {
		if (self->pyDynaset->nRow == -1)
			return Py_None;//Py_RETURN_NONE;
		else
			return PxDynaset_GetData(self->pyDynaset, self->pyDynaset->nRow, self->pyDataColumn);
	}
	return NULL;
}

bool
PxWidget_SetData(PxWidgetObject* self, PyObject* pyData)
{
	if (PyObject_RichCompareBool(self->pyData, pyData, Py_EQ))
		return true;

	PxAttachObject(&self->pyData, pyData, true);
	if (self->pyDynaset && /*!self->pyDynaset->bBroadcasting && */self->pyDataColumn && self->pyDynaset->nRow != -1) {
		//g_debug("*---- PxWidget_SetData");
		if (!PxDynaset_SetData(self->pyDynaset, self->pyDynaset->nRow, self->pyDataColumn, self->pyData))
			return false;
	}
	return true;
}

static PyObject* // new ref
PxWidget_str(PxWidgetObject* self)
{
	return PyUnicode_FromFormat("%.50s widget at %p, bound to Dynaset '%.50s', column '%.50s', holding data value %.50s.",
		Py_TYPE(self)->tp_name, self,
		self->pyDynaset == NULL ? "NULL" : PyUnicode_AsUTF8(self->pyDynaset->pyTable),
		self->pyDataColumn == NULL ? "NULL" : PyUnicode_AsUTF8(PyStructSequence_GET_ITEM(self->pyDataColumn, PXDYNASETCOLUMN_NAME)),
		self->pyData == NULL ? "NULL" : PyUnicode_AsUTF8(PyObject_Repr(self->pyData)));
}

static int
PxWidget_setattro(PxWidgetObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			PyErr_SetString(PyExc_TypeError, "Cannot set caption for this widget.");
			return -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			gtk_widget_set_visible(self->gtk, PyObject_IsTrue(pyValue));
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "frame") == 0) {
			g_debug("Widget frame not implemented");
			return  0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "verify") == 0) {
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyVerifyCB);
				self->pyVerifyCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assigned object must be callable.");
				return -1;
			}
		}
	}
	if (PyObject_GenericSetAttr((PyObject*)self, pyAttributeName, pyValue) < 0)
		return -1;
	return  0;
}

static PyObject* // new ref
PxWidget_getattro(PxWidgetObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "parent") == 0) {
			PyErr_Clear();
			if (self->pyParent)
				return self->pyParent;
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			PyErr_Clear();
			if (self->pyLabel) {
				PyObject* pyText = PyObject_GetAttr(self->pyLabel, "caption");
				return pyText;
			}
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "visible") == 0) {
			PyErr_Clear();
			if (gtk_widget_is_visible(self->gtk))
				Py_RETURN_TRUE;
			else
				Py_RETURN_FALSE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "frame") == 0) {
			PyErr_Clear();
			// not yet implemented
			Py_RETURN_FALSE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "position") == 0) {
			PyErr_Clear();
			guint iX, iY;
			gtk_container_child_get(self->pyParent->gtkFixed, self->gtk, "x", &iX, "y", &iY, NULL);
			guint iWidth = gtk_widget_get_allocated_width(self->gtk);
			guint iHeight = gtk_widget_get_allocated_height(self->gtk);
			PyObject* pyRectangle = PyTuple_Pack(4, PyLong_FromLong(iX), PyLong_FromLong(iY), PyLong_FromLong(iWidth), PyLong_FromLong(iHeight));
			return pyRectangle;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "size") == 0) {
			PyErr_Clear();
			guint iWidth = gtk_widget_get_allocated_width(self->gtk);
			guint iHeight = gtk_widget_get_allocated_height(self->gtk);
			PyObject* pyRectangle = PyTuple_Pack(2, PyLong_FromLong(iWidth), PyLong_FromLong(iHeight));
			return pyRectangle;
		}
	}
	//return pyResult;
	return PxWidgetType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static PyMemberDef PxWidget_members[] = {
	{ "nativeWidget", T_INT, offsetof(PxWidgetObject, gtk), READONLY, "Win32 handle of the MS-Windows window" },
	{ "left", T_INT, offsetof(PxWidgetObject, rc.iLeft), READONLY, "Distance from left edge of parent, if negative from right" },
	{ "top", T_INT, offsetof(PxWidgetObject, rc.iTop), READONLY, "Distance from top edge of parent, if negative from bottom" },
	{ "right", T_INT, offsetof(PxWidgetObject, rc.iWidth), READONLY, "Distance from left or, if zero or negative, from right edge of parent of right edge" },
	{ "bottom", T_INT, offsetof(PxWidgetObject, rc.iHeight), READONLY, "Distance from top or, if zero or negative, from bottom edge of parent of bottom edge" },
	{ "dynaset", T_OBJECT, offsetof(PxWidgetObject, pyDynaset), READONLY, "Bound Dynaset" },
	{ "dataColumn", T_OBJECT, offsetof(PxWidgetObject, pyDataColumn), READONLY, "Connected DataColumn" },
	{ "dataType", T_OBJECT, offsetof(PxWidgetObject, pyDataType), READONLY, "Data type of connected DataColumn" },
	{ "editFormat", T_OBJECT, offsetof(PxWidgetObject, pyFormatEdit), 0, "Format when editing" },
	{ "format", T_OBJECT, offsetof(PxWidgetObject, pyFormat), 0, "Format for display" },
	{ "label", T_OBJECT, offsetof(PxWidgetObject, pyLabel), READONLY, "Associated Label widget" },
	{ "parent", T_OBJECT, offsetof(PxWidgetObject, pyParent), READONLY, "Parent widget" },
	{ "window", T_OBJECT, offsetof(PxWidgetObject, pyWindow), READONLY, "Parent window" },
	{ "clean", T_BOOL, offsetof(PxWidgetObject, bClean), READONLY, "False if widget has been edited." },
	{ "readOnly", T_BOOL, offsetof(PxWidgetObject, bReadOnly), 0, "Data can not be edited." },
	{ "alignHoriz", T_OBJECT, offsetof(PxWidgetObject, pyAlignHorizontal), READONLY, "Horizontal alignment" },
	{ "alignVert", T_OBJECT, offsetof(PxWidgetObject, pyAlignVertical), READONLY, "Vertical alignment" },
	{ NULL }
};

static PyMethodDef PxWidget_methods[] = {
	{ "move", (PyCFunction)PxWidget_move, METH_VARARGS | METH_KEYWORDS, "Reposition widget to given coordinates." },
	{ "refresh", (PyCFunction)PxWidget_pass, METH_NOARGS, "Ignore" },
	{ "refresh_cell", (PyCFunction)PxWidget_pass, METH_VARARGS | METH_KEYWORDS, "Ignore" },
	{ "refresh_row_pointer", (PyCFunction)PxWidget_pass, METH_NOARGS, "Ignore" },
	//{ "focus_in", (PyCFunction)PxWidget_focus_in, METH_NOARGS, "Widget receives focus." },
	{ NULL }
};

PyTypeObject PxWidgetType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Widget",            /* tp_name */
	sizeof(PxWidgetObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxWidget_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	PxWidget_str,              /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	PxWidget_str,              /* tp_str */
	PxWidget_getattro,         /* tp_getattro */
	PxWidget_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,       /* tp_flags */
	"The parent of all widgets", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxWidget_methods,          /* tp_methods */
	PxWidget_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxWidget_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxWidget_new,              /* tp_new */
	0,                         /*tp_free*/
	0,                         /*tp_is_gc*/
};
