#include "Pylax.h"

bool PxLabel_RenderData(PxLabelObject* self, bool bFormat);

static PyObject *
PxLabel_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxLabelObject* self = (PxLabelObject*)PxLabelType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyCaptionClient = NULL;
		self->pyAssociatedWidget = NULL;
		//self->textColor = 0;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxLabel_init(PxLabelObject* self, PyObject* args, PyObject* kwds)
{
	char* sCaption;
	if (PxLabelType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	if (gArgs.pyCaption != NULL)
		sCaption = PyUnicode_AsUTF8(gArgs.pyCaption);
	else
		sCaption = ""; //"<Label>";

	self->gtk = gtk_label_new(sCaption);
	gtk_label_set_xalign(self->gtk, 0);
	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	PxWidget_Reposition(self);
	gtk_widget_show(self->gtk);
	g_object_set_qdata(self->gtk, g.gQuark, self);
	/*
		if (gArgs.pyCaption != NULL)
			if (!PxLabel_SetCaption(self, gArgs.pyCaption))
				return -1;
				*/

	return 0;
}

bool
PxLabel_SetCaption(PxLabelObject* self, PyObject* pyText)
{
	char* sText = PyUnicode_AsUTF8(pyText);
	gtk_label_set_text(self->gtk, sText);
	return true;
}

bool
PxLabel_SetData(PxLabelObject* self, PyObject* pyData)
{
	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return false;

	return PxLabel_RenderData(self, true);
}

bool
PxLabel_RenderData(PxLabelObject* self, bool bFormat)
{
	bool bSuccess;
	PyObject* pyText = PxFormatData(self->pyData, bFormat ? self->pyFormat : (self->pyFormatEdit ? self->pyFormatEdit : Py_None));
	if (pyText == NULL) {
		return false;
	}

	if (self->pyCaptionClient) {
		bSuccess = (PyObject_SetAttrString(self->pyCaptionClient, "caption", pyText) == -1) ? false : true;
		Py_DECREF(pyText);
	}
	else {
		bSuccess = PxLabel_SetCaption((PxLabelObject*)self, pyText);
		Py_DECREF(pyText);
	}
	return bSuccess;
}

static int
PxLabel_setattro(PxLabelObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			return PxLabel_SetCaption((PxWidgetObject*)self, pyValue) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			return PxLabel_SetData(self, pyValue) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "alignHoriz") == 0) {
			switch (PyLong_AsLong(PyObject_GetAttrString(pyValue, "value")))
			{
#if GTK_MINOR_VERSION >= 16
			case ALIGN_LEFT:
				gtk_label_set_xalign(self->gtk, 0);
				break;
			case ALIGN_RIGHT:
				gtk_label_set_xalign(self->gtk, 1);
				break;
			case ALIGN_CENTER:
				gtk_label_set_xalign(self->gtk, 0.5);
				break;
#endif
			}
			self->pyAlignHorizontal = pyValue;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "alignVert") == 0) {
			switch (PyLong_AsLong(PyObject_GetAttrString(pyValue, "value")))
			{
#if GTK_MINOR_VERSION >= 16
			case ALIGN_TOP:
				gtk_label_set_yalign(self->gtk, 0);
				break;
			case ALIGN_BOTTOM:
				gtk_label_set_yalign(self->gtk, 1);
				break;
			case ALIGN_CENTER:
				gtk_label_set_yalign(self->gtk, 0.5);
				break;
#endif
			}
			self->pyAlignVertical = pyValue;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "textColor") == 0) {
			return 0;/*
		if (PyTuple_Check(pyValue)) {
			int iR, iG, iB;
			PyObject* pyColorComponent = PyTuple_GetItem(pyValue, 0);
			if (pyColorComponent == NULL)
				return -1;
			iR = PyLong_AsLong(pyColorComponent);
			pyColorComponent = PyTuple_GetItem(pyValue, 1);
			if (pyColorComponent == NULL)
				return -1;
			iG = PyLong_AsLong(pyColorComponent);
			pyColorComponent = PyTuple_GetItem(pyValue, 2);
			if (pyColorComponent == NULL)
				return -1;
			iB = PyLong_AsLong(pyColorComponent);

			self->textColor = RGB(iR, iG, iB);
			return 0;
		}
		else {
			PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
			return -1;
		}*/
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PxWidgetObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxLabel_getattro(PxLabelObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyObject* pyText = PyUnicode_FromString(gtk_label_get_text(self->gtk));
			PyErr_Clear();
			return pyText;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) {
			PyObject* pyText = PyUnicode_FromString(gtk_label_get_text(self->gtk));
			PyErr_Clear();
			return pyText;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PxWidgetObject*)self, pyAttributeName);
}

static PyObject*
PxLabel_refresh(PxLabelObject* self)
{
	if (self->pyDynaset == NULL || !self->bClean)
		Py_RETURN_TRUE;

	if (self->pyDynaset->nRow == -1) {
		if (self->pyData != Py_None) {
			PxAttachObject(&self->pyData, Py_None, true);
			if (!PxLabel_RenderData(self, true))
				Py_RETURN_FALSE;
		}
	}
	else {
		PyObject* pyData = PxWidget_PullData((PxWidgetObject*)self);
		if (!PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {
			PxAttachObject(&self->pyData, pyData, true);
			if (!PxLabel_RenderData(self, true))
				Py_RETURN_FALSE;
		}
	}
	Py_RETURN_TRUE;
}

static void
PxLabel_dealloc(PxLabelObject* self)
{
	Py_XDECREF(self->pyAssociatedWidget);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxLabel_members[] = {
	{ "captionClient", T_OBJECT, offsetof(PxLabelObject, pyCaptionClient), 0, "Pass any assigment to property 'data' on to property 'caption' of this." },
	{ NULL }
};

static PyMethodDef PxLabel_methods[] = {
	{ "refresh", (PyCFunction)PxLabel_refresh, METH_NOARGS, "Pull fresh data" },
	{ NULL }
};

PyTypeObject PxLabelType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Label",             /* tp_name */
	sizeof(PxLabelObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxLabel_dealloc, /* tp_dealloc */
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
	PxLabel_getattro,          /* tp_getattro */
	PxLabel_setattro,          /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Label objects",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxLabel_methods,           /* tp_methods */
	PxLabel_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxLabel_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxLabel_new,               /* tp_new */
};
