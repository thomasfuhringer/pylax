#include "Pylax.h"

static gboolean DrawCB(GtkWidget* gtkWidget, cairo_t* caiContext, gpointer pUserData);

static PyObject*
PxCanvas_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxCanvasObject* self = (PxCanvasObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyOnPaintCB = NULL;
		self->caiContext = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxCanvas_init(PxCanvasObject* self, PyObject* args, PyObject* kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;

	self->gtk = gtk_drawing_area_new();
	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);
	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	g_signal_connect(G_OBJECT(self->gtk), "draw", G_CALLBACK(DrawCB), self);
	PxWidget_Reposition(self);
	gtk_widget_show(self->gtk);
	g_object_set_qdata(self->gtk, g.gQuark, self);

	return 0;
}

static gboolean
DrawCB(GtkWidget* gtkWidget, cairo_t* caiContext, gpointer pUserData)
{
	//GdkRGBA color;

	//GtkStyleContext* gdkStyleContext = gtk_widget_get_style_context (gtkWidget);

	PxCanvasObject* self = (PxCanvasObject*)pUserData;
	self->iWidth = gtk_widget_get_allocated_width(gtkWidget);
	self->iHeight = gtk_widget_get_allocated_height(gtkWidget);

	self->caiContext = caiContext;
	cairo_set_line_width(caiContext, 0.2);
	cairo_select_font_face(caiContext, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(caiContext, 1.2);
	//gtk_render_background (gdkStyleContext, caiContext, 0, 0, iWidth, iHeight);

	//gtk_style_context_get_color (gdkStyleContext, gtk_style_context_get_state (gdkStyleContext), &color);
	//gdk_cairo_set_source_rgba (caiContext, &color);

	PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
	Py_INCREF(self);
	PyObject* pyResult = PyObject_CallObject(self->pyOnPaintCB, pyArgs);

	self->caiContext = NULL;

	if (pyResult == NULL) {
		PythonErrorDialog();
	}
	else
		Py_DECREF(pyResult);

	return FALSE;
}

static PyObject*
PxCanvas_point(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;
	if (self->caiContext == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "ii",
		&iX,
		&iY))
		return NULL;

	if (iX < 0)
		iX += self->iWidth;
	if (iY < 0)
		iY += self->iHeight;

	cairo_move_to(self->caiContext, iX, iY);
	cairo_line_to(self->caiContext, iX, iY);
	cairo_stroke(self->caiContext);

	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_move_to(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;

	if (self->caiContext == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "ii",
		&iX,
		&iY))
		return NULL;

	if (iX < 0)
		iX += self->iWidth;
	if (iY < 0)
		iY += self->iHeight;

	cairo_move_to(self->caiContext, iX, iY);

	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_line_to(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;

	if (self->caiContext == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "ii",
		&iX,
		&iY))
		return NULL;

	if (iX < 0)
		iX += self->iWidth;
	if (iY < 0)
		iY += self->iHeight;

	cairo_line_to(self->caiContext, iX, iY);
	cairo_stroke(self->caiContext);
	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_rectangle(PxCanvasObject* self, PyObject *args)
{
	Rect rcRel, rcAbs;
	int iRadius = -1;
	bool bResult;

	if (self->caiContext == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "llll|i",
		&rcRel.iLeft,
		&rcRel.iTop,
		&rcRel.iWidth,
		&rcRel.iHeight,
		&iRadius))
		return NULL;

	PxWidget_CalculateRect(self, &rcAbs);

	if (iRadius == -1)
		cairo_rectangle(self->caiContext, rcAbs.iLeft, rcAbs.iTop, rcAbs.iWidth, rcAbs.iHeight);
	else
		cairo_rectangle(self->caiContext, rcAbs.iLeft, rcAbs.iTop, rcAbs.iWidth, rcAbs.iHeight);
	cairo_fill(self->caiContext);

	if (bResult)
		Py_RETURN_NONE;
	else {
		//PyErr_SetFromWindowsErr(0);
		return NULL;
	}
}

static PyObject*
PxCanvas_text(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;
	const char* sText;

	if (self->caiContext == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "iis",
		&iX,
		&iY,
		&sText))
		return NULL;

	if (iX < 0)
		iX += self->iWidth;
	if (iY < 0)
		iY += self->iHeight;

	cairo_move_to(self->caiContext, iX, iY);
	cairo_show_text(self->caiContext, sText);
	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_repaint(PxCanvasObject* self)
{
	gtk_widget_queue_draw_area(self->gtk, 0, 0, 9999, 9999);
	Py_RETURN_NONE;
}

static int
PxCanvas_setattro(PxCanvasObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_paint") == 0) {
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyOnPaintCB);
				self->pyOnPaintCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "textColor") == 0) {
			if (PyTuple_Check(pyValue)) {
				if (self->caiContext == NULL) {
					PyErr_SetString(PyExc_RuntimeError, "textColor can only be assigned inside an 'on_paint' callback.");
					return -1;
				}
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

				cairo_set_source_rgb(self->caiContext, iR, iG, iB);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "penColor") == 0) {
			if (PyTuple_Check(pyValue)) {
				if (self->caiContext == NULL) {
					PyErr_SetString(PyExc_RuntimeError, "penColor can only be assigned inside an 'on_paint' callback.");
					return -1;
				}
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

				cairo_set_source_rgb(self->caiContext, iR, iG, iB);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "fillColor") == 0) {
			if (PyTuple_Check(pyValue)) {
				if (self->caiContext == NULL) {
					PyErr_SetString(PyExc_RuntimeError, "fillColor can only be assigned inside an 'on_paint' callback.");
					return -1;
				}
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

				cairo_set_source_rgb(self->caiContext, iR, iG, iB);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject*
PxCanvas_getattro(PxCanvasObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_paint") == 0) {
			PyErr_Clear();
			return self->pyOnPaintCB;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxCanvas_dealloc(PxCanvasObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxCanvas_members[] = {
	{ NULL }
};

static PyMethodDef PxCanvas_methods[] = {
	{ "point", (PyCFunction)PxCanvas_point, METH_VARARGS, "Draws a point." },
	{ "move_to", (PyCFunction)PxCanvas_move_to, METH_VARARGS, "Moves to a point." },
	{ "line_to", (PyCFunction)PxCanvas_line_to, METH_VARARGS, "Draws a line." },
	{ "rectangle", (PyCFunction)PxCanvas_rectangle, METH_VARARGS, "Draws a rectangle." },
	{ "text", (PyCFunction)PxCanvas_text, METH_VARARGS, "Draws a rectangle." },
	{ "repaint", (PyCFunction)PxCanvas_repaint, METH_NOARGS, "Trigger the paint process." },
	{ NULL }
};

PyTypeObject PxCanvasType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Canvas",            /* tp_name */
	sizeof(PxCanvasObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxCanvas_dealloc, /* tp_dealloc */
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
	PxCanvas_getattro,         /* tp_getattro */
	PxCanvas_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Canvas, a widget for direct painting", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxCanvas_methods,           /* tp_methods */
	PxCanvas_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxCanvas_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxCanvas_new,              /* tp_new */
};
