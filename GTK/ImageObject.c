// ImageObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

static gboolean GtkButton_ClickedCB(GtkButton* gtkWidget, gpointer pUserData);

static PyObject*
PxImage_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxImageObject* self = (PxEntryObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyImageFormat = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxImage_init(PxImageObject* self, PyObject* args, PyObject* kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;

	self->gtk = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(self->gtk), GTK_RELIEF_NONE);

	self->gtkImage = gtk_image_new_from_pixbuf(g.gdkPixbufPlaceHolder);
	g_object_set(self->gtkImage, "halign", GTK_ALIGN_CENTER, "valign", GTK_ALIGN_CENTER, NULL);
	gtk_button_set_image(GTK_BUTTON(self->gtk), self->gtkImage);
	gtk_button_set_image_position(GTK_BUTTON(self->gtk), GTK_POS_LEFT);
	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	PxWidget_Reposition(self);

	g_signal_connect(G_OBJECT(self->gtk), "clicked", G_CALLBACK(GtkButton_ClickedCB), self);
	gtk_widget_show_all(self->gtk);
	g_object_set_qdata(self->gtk, g.gQuark, self);
	return 0;
}

static gboolean
GtkButton_ClickedCB(GtkButton* gtkWidget, gpointer pUserData)
{
	GtkWidget* gtkFileChooser;
	gint iResponse;
	GdkPixbuf* gdkPixbuf = NULL;
	char* sFileName;
	GError* gError = NULL;
	char* sPictureBuffer = NULL;
	gsize nSize;
	PxImageObject* self;
	PyObject* pyData;

	self = (PxImageObject*)g_object_get_qdata(gtkWidget, g.gQuark);

	gtkFileChooser = gtk_file_chooser_dialog_new("Select Image", g.gtkMainWindow, GTK_FILE_CHOOSER_ACTION_OPEN,
		"_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_CLOSE, NULL);

	GtkFileFilter* gtkFileFilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gtkFileFilter, "*.jpg");
	gtk_file_filter_set_name(gtkFileFilter, "JPG");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkFileChooser), gtkFileFilter);

	iResponse = gtk_dialog_run(GTK_DIALOG(gtkFileChooser));
	if (iResponse == GTK_RESPONSE_CLOSE) {
		sFileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkFileChooser));
		if (sFileName == NULL) {
			gtk_image_set_from_pixbuf(self->gtkImage, g.gdkPixbufPlaceHolder);
			pyData = Py_None;
			Py_INCREF(Py_None);
		}
		else {
			gdkPixbuf = gdk_pixbuf_new_from_file_at_size(sFileName, Px_IMAGE_WIDTH, Px_IMAGE_HEIGHT, &gError);
			//gdkPixbuf = gdk_pixbuf_new_from_file(sFileName, &gError);
			g_assert_no_error(gError);
			if (!gdkPixbuf) {
				ErrorDialog(gError->message);
				g_printerr("%s\n", gError->message);
				g_error_free(gError);
				return false;
			}
			gtk_image_set_from_pixbuf(self->gtkImage, gdkPixbuf);

			if (!gdk_pixbuf_save_to_buffer(gdkPixbuf, &sPictureBuffer, &nSize, "jpeg", &gError, NULL)) {
				g_printerr("%s\n", gError->message);
				g_error_free(gError);
				return false;
			}
			g_assert(sPictureBuffer != NULL);

			pyData = PyBytes_FromStringAndSize(sPictureBuffer, nSize);
			g_free(sPictureBuffer);
			if (!PxWidget_SetData(self, pyData))
				return false;
			Py_XDECREF(pyData);
			g_object_unref(gdkPixbuf);
			g_free(sFileName);
		}
	}

	gtk_widget_destroy(gtkFileChooser);
	g_object_unref(G_OBJECT(gtkFileFilter));
	return true;
}

static bool
PxImage_Load(PxImageObject* self, char* sFileName)
{
	gtk_image_set_from_file(self->gtkImage, sFileName);
	// ...
	return true;
}

static PyObject*
PxImage_load(PxImageObject* self, PyObject* args)
{
	const char* sFileName;

	if (!PyArg_ParseTuple(args, "s",
		&sFileName))
		return NULL;

	if (!PxImage_Load(self, sFileName))
		return NULL;

	Py_RETURN_NONE;
}

bool
PxImage_SetData(PxImageObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return true;
	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return false;

	return true;
}

bool
PxImage_RenderData(PxImageObject* self, bool bFormat)
{
	GdkPixbufLoader* gdkPixbufLoader;
	GdkPixbuf* gdkPixbuf;
	Py_ssize_t nSize;
	char* sBuffer;

	if (self->pyData == NULL || self->pyData == Py_None) {
	gtk_image_set_from_pixbuf(self->gtkImage, g.gdkPixbufPlaceHolder);
		return true;
	}

	nSize = PyBytes_Size(self->pyData);
	sBuffer = PyBytes_AsString(self->pyData);
	gdkPixbufLoader = gdk_pixbuf_loader_new();
	if (!gdk_pixbuf_loader_write(gdkPixbufLoader, sBuffer, nSize, NULL)) {
		//g_debug("* error loading image buffer");
		return false;
	}
	gdkPixbuf = gdk_pixbuf_loader_get_pixbuf(gdkPixbufLoader);
	gtk_image_set_from_pixbuf(self->gtkImage, gdkPixbuf);
	gdk_pixbuf_loader_close(gdkPixbufLoader, NULL);
	g_object_unref(G_OBJECT(gdkPixbufLoader));
	return true;
}

static PyObject*  // new ref
PxImage_refresh(PxImageObject* self)
{
	if (self->pyDynaset == NULL || !self->bClean)
		Py_RETURN_TRUE;

	if (self->pyDynaset->nRow == -1) {
		if (self->pyData != Py_None) {
			PxAttachObject(&self->pyData, Py_None, true);
			if (!PxImage_RenderData(self, true))
				Py_RETURN_FALSE;
		}
		gtk_widget_set_sensitive(self->gtk, false);
	}
	else {
		PyObject* pyData = PxWidget_PullData((PxWidgetObject*)self);
		if (!PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {
			PxAttachObject(&self->pyData, pyData, true);
		}
			if (!PxImage_RenderData(self, true))
				Py_RETURN_FALSE;
		gtk_widget_set_sensitive(self->gtk, !(self->bReadOnly || self->pyDynaset->bLocked));
	}
	Py_RETURN_TRUE;
}

static int
PxImage_setattro(PxImageObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			if (!PxImage_SetData(self, pyValue))
				return -1;
			return 0;
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject*
PxImage_getattro(PxImageObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			return self->pyData;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxImage_dealloc(PxImageObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject*)self);
}

static PyMemberDef PxImage_members[] = {
	{ "format", T_OBJECT, offsetof(PxImageObject, pyImageFormat), 0, "Format" },
	{ "stretch", T_BOOL, offsetof(PxImageObject, bStretch), 0, "Stretch to fit into widget, keeping aspect ratio." },
	{ "fill", T_BOOL, offsetof(PxImageObject, bFill), 0, "Stretch to fit into widget, filling all available space (distorting)." },
	{ NULL }
};

static PyMethodDef PxImage_methods[] = {
	{ "refresh", (PyCFunction)PxImage_refresh, METH_NOARGS, "Pull fresh data" },
	//{ "load", (PyCFunction)PxImage_load, METH_VARARGS, "Load from file" },
	//{ "bytes", (PyCFunction)PxImage_as_bytes, METH_NOARGS, "Convert to bytes object." },
	{ NULL }
};

PyTypeObject PxImageType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Image",             /* tp_name */
	sizeof(PxImageObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxImage_dealloc, /* tp_dealloc */
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
	PxImage_getattro,          /* tp_getattro */
	PxImage_setattro,          /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Image object",            /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxImage_methods,           /* tp_methods */
	PxImage_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxImage_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxImage_new,               /* tp_new */
};

bool
PxImageType_Init()
{
	g.gdkPixbufPlaceHolder = gdk_pixbuf_new_from_file_at_size("Placeholder.bmp", Px_IMAGE_WIDTH, Px_IMAGE_HEIGHT, NULL);
	return true;
}
