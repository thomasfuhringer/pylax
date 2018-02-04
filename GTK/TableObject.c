// TableObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

static void GtkTreeCell_Render(GtkTreeViewColumn* gtkTreeViewColumn, GtkCellRenderer* gtkCellRenderer, GtkTreeModel* gtkTreeModel, GtkTreeIter* gtkTreeIter, gpointer gUserData);
static void GtkTreeCell_RenderRowIndicator(GtkTreeViewColumn* gtkTreeViewColumn, GtkCellRenderer* gtkCellRenderer, GtkTreeModel* gtkTreeModel, GtkTreeIter* gtkTreeIter, gpointer gUserData);
static void GtkTreeSelection_ChangedCB(GtkTreeSelection* gtkTreeSelection, gpointer gUserData);
static void GtkCellRenderer_TextEditedCB(GtkCellRendererText* gtkCellRendererText, gchar* sPath, gchar* sText, gpointer gUserData);
static void GtkCellRenderer_EditingStartedCB(GtkCellRendererText* gtkCellRendererText, GtkCellEditable* gtkCellEditable, const gchar* sPath, gpointer gUserData);
static gboolean GtkTreeView_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData);

static PyObject *
PxTable_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxTableObject* self = (PxTableObject*)PxTableType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->bTable = true;
		self->bPointer = true;
		self->bShowRecordIndicator = false;
		self->nColumns = 0;
		self->iAutoSizeColumn = -1;
		self->pyColumns = PyList_New(0);
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxTable_init(PxTableObject* self, PyObject* args, PyObject* kwds)
{
	if (PxTableType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	self->gtkListStore = gtk_list_store_new(1, G_TYPE_INT);

	self->gtk = gtk_scrolled_window_new(NULL, NULL);
	g_signal_connect(G_OBJECT(self->gtk), "destroy", G_CALLBACK(GtkWidget_DestroyCB), (gpointer)self);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->gtk), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	self->gtkTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(self->gtkListStore));
	gtk_tree_view_set_grid_lines(self->gtkTreeView, GTK_TREE_VIEW_GRID_LINES_BOTH);
	g_signal_connect(G_OBJECT(self->gtkTreeView), "focus-in-event", G_CALLBACK(GtkTreeView_FocusInEventCB), (gpointer)self);
	g_object_unref(self->gtkListStore);   // tree view has acquired reference
	gtk_tree_view_set_fixed_height_mode(self->gtkTreeView, TRUE);
	gtk_container_add(GTK_CONTAINER(self->gtk), self->gtkTreeView);

	gtk_fixed_put(self->pyParent->gtkFixed, self->gtk, 0, 0);
	PxWidget_Reposition(self);
	gtk_widget_show_all(self->gtk);
	gtk_widget_show_all(self->gtkTreeView);
	g_object_set_qdata(self->gtk, g.gQuark, self);

	self->gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->gtkTreeView));
	gtk_tree_selection_set_mode(self->gtkTreeSelection, GTK_SELECTION_SINGLE);
	self->gtkTreeSelectionChangedHandlerID = g_signal_connect(G_OBJECT(self->gtkTreeSelection), "changed", G_CALLBACK(GtkTreeSelection_ChangedCB), self);

	return 0;
}

static PyObject *
PxTable_add_column(PxTableObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "caption", "width", "data", "type", "editable", "format", "widget", "autoSize", NULL };
	int iWidth;
	bool bAutoSize, bEditable = false;
	PyObject* pyCaption = NULL, *pyDataName = NULL, *pyDynasetColumn = NULL, *pyType = NULL, *pyFormat = NULL;//, *pyWidget = NULL;
	PxTableColumnObject* pyColumn = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OOpOOp", kwlist,
		&pyCaption,
		&iWidth,
		&pyDataName,
		&pyType,
		&bEditable,
		&pyFormat,
		//&pyWidget,
		&bAutoSize))
		return NULL;

	if (!PyUnicode_Check(pyCaption)) {
		PyErr_SetString(PyExc_TypeError, "Parameter 1 ('caption') must be a string.");
		return NULL;
	}

	if (pyDataName) {
		if ((pyDynasetColumn = PyDict_GetItem(self->pyDynaset->pyColumns, pyDataName)) == NULL)
			return PyErr_Format(PyExc_ValueError, "DataColumn '%s' does not exist in bound Dynaset'.", PyUnicode_AsUTF8(pyDataName));

		PyObject* pyDataType = PyStructSequence_GetItem(pyDynasetColumn, PXDYNASETCOLUMN_TYPE);
		if (pyType) {
			if (pyType != pyDataType) {
				PyErr_SetString(PyExc_ValueError, "Parameter 4 ('type') is conflicting with data type of bound DataColumn.");
				return NULL;
			}
		}
		else {
			pyType = pyDataType;
			Py_INCREF(pyType);
		}

		if (!pyFormat) {
			pyFormat = PyStructSequence_GetItem(pyDynasetColumn, PXDYNASETCOLUMN_FORMAT);
		}
	}

	if (pyType) {
		if (!PyType_Check(pyType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 4 ('type') must be a Type Object.");
			return NULL;
		}
	}
	else {
		pyType = (PyObject*)&PyUnicode_Type;
	}

	if (pyFormat) {
		if (pyFormat != Py_None && !PyUnicode_Check(pyFormat)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 5 ('format') must be a string.");
			return NULL;
		}
	}
	else {
		pyFormat = Py_None;
	}

	if (!(pyColumn = PyObject_CallObject((PyObject*)&PxTableColumnType, NULL)))
		return NULL;
	Py_INCREF(pyDynasetColumn);
	Py_INCREF(pyType);
	Py_INCREF(pyFormat);
	pyColumn->pyDynasetColumn = pyDynasetColumn;
	pyColumn->pyType = pyType;
	pyColumn->bEditable = bEditable;
	pyColumn->pyFormat = pyFormat;
	pyColumn->iIndex = (int)self->nColumns++;
	pyColumn->pyTable = self;

	/*if (pyWidget) {
		if (!PyObject_TypeCheck(pyWidget, &PxWidgetType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 6 ('widget') must be a Widget.");
			return NULL;
		}
		Py_INCREF(self);
		Py_INCREF(pyWidget);
		pyColumn->pyWidget = (PxWidgetObject*)pyWidget;
		pyColumn->pyWidget->pyParent = (PxWidgetObject*)self;
		pyColumn->pyWidget->pyWindow = self->pyWindow;
		pyColumn->pyWidget->pyDynaset = self->pyDynaset;
		pyColumn->pyWidget->pyDataColumn = self->pyDataColumn;
	}*/

	if (PyList_Append(self->pyColumns, pyColumn) == -1) {
		return NULL;
	}

	pyColumn->gtkCellRenderer = gtk_cell_renderer_text_new();
	if (bEditable) {
		g_object_set(pyColumn->gtkCellRenderer, "editable", TRUE, NULL);
		g_signal_connect(G_OBJECT(pyColumn->gtkCellRenderer), "edited", G_CALLBACK(GtkCellRenderer_TextEditedCB), pyColumn);
		g_signal_connect(G_OBJECT(pyColumn->gtkCellRenderer), "editing-started", G_CALLBACK(GtkCellRenderer_EditingStartedCB), pyColumn);
	}
	gtk_cell_renderer_set_alignment(pyColumn->gtkCellRenderer, pyType == (PyObject*)&PyUnicode_Type ? 0 : 1, 0.5);

	GtkTreeViewColumn* gtkTreeViewColumn = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumn, pyColumn->gtkCellRenderer, TRUE);
	gtk_tree_view_column_set_title(gtkTreeViewColumn, PyUnicode_AsUTF8(pyCaption));
	gtk_tree_view_column_set_fixed_width(gtkTreeViewColumn, iWidth);
	gtk_tree_view_column_set_alignment(gtkTreeViewColumn, pyType == (PyObject*)&PyUnicode_Type ? 0 : 1);
	gtk_tree_view_column_set_cell_data_func(gtkTreeViewColumn, pyColumn->gtkCellRenderer, GtkTreeCell_Render, pyColumn, NULL);
	gtk_tree_view_column_set_resizable(gtkTreeViewColumn, TRUE);
	g_object_set(gtkTreeViewColumn, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
	/*gint iIndex = */gtk_tree_view_append_column(self->gtkTreeView, gtkTreeViewColumn);
	pyColumn->gtkTreeViewColumn = gtkTreeViewColumn;

	if (bAutoSize)
		self->iAutoSizeColumn = pyColumn->iIndex;

	return (PyObject*)pyColumn;
}

static PyObject *
PxTable_refresh(PxTableObject* self)
{
	gint iRow;
	GtkTreeIter gtkTreeIter;
	GtkTreePath* gtkTreePath;

	//gtk_tree_view_set_model(GTK_TREE_VIEW(self->gtk), NULL); /* Detach model from view */
	// workaround to bug of gtk_list_store_clear(), which sends all kinds of stupid signals when row# 0 had been selected
	g_signal_handler_block(G_OBJECT(self->gtkTreeSelection), self->gtkTreeSelectionChangedHandlerID);
	gtk_list_store_clear(self->gtkListStore);

	if (self->pyDynaset->nRows > 0) {
		for (iRow = 0; iRow < self->pyDynaset->nRows; iRow++) {
			gtk_list_store_append(self->gtkListStore, &gtkTreeIter);
			gtk_list_store_set(self->gtkListStore, &gtkTreeIter, 0, iRow, -1);
		}

		gtkTreePath = gtk_tree_path_new_from_indices(self->pyDynaset->nRow, -1);
		gtk_tree_selection_select_path(self->gtkTreeSelection, gtkTreePath);
		gtk_tree_path_free(gtkTreePath);
	}
	g_signal_handler_unblock(G_OBJECT(self->gtkTreeSelection), self->gtkTreeSelectionChangedHandlerID);

	Py_RETURN_TRUE;
}

static PyObject *
PxTable_refresh_cell(PxTableObject* self, PyObject* args)
{
	PyObject* pyData = NULL, *pyDynasetColumn = NULL, *pyText = NULL;
	PxTableColumnObject* pyTableColumn = NULL;
	Py_ssize_t nColumn, nRow;
	GtkTreeIter   gtkTreeIter;
	GtkTreePath* gtkTreePath;

	if (!PyArg_ParseTuple(args, "nO", &nRow, &pyDynasetColumn)) {
		return NULL;
	}

	gtkTreePath = gtk_tree_path_new_from_indices(nRow, -1);
	if (!gtk_tree_model_get_iter(self->gtkListStore, &gtkTreeIter, gtkTreePath))
		g_debug("Non-existing path in Table");
	//return; /* path describes a non-existing row - should not happen */
	gtk_list_store_set(self->gtkListStore, &gtkTreeIter, 0, nRow, -1);  // set it to the same value just to trigger an update of the tree view
	//g_object_unref(gtkTreePath);

	Py_RETURN_TRUE;
}

static PyObject*
PxTable_refresh_row_pointer(PxTableObject* self, PyObject* args)
{
	GtkTreeModel* gtkTreeModel;
	GtkTreeIter   gtkTreeIter;
	gint iRow;
	GtkTreePath* gtkTreePath;

	if (gtk_tree_selection_get_selected(self->gtkTreeSelection, &gtkTreeModel, &gtkTreeIter))
	{
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, 0, &iRow, -1);
	}
	else
	{
		//g_debug("PxTable_refresh_row_pointer no row selected DS at %d\n", self->pyDynaset->nRow);
		iRow = -1;
	}

	if (self->pyDynaset->nRow == iRow)
		Py_RETURN_NONE;
	if (self->pyDynaset->nRow == -1) {
		gtk_tree_selection_unselect_all(self->gtkTreeSelection);
	}

	gtkTreePath = gtk_tree_path_new_from_indices(self->pyDynaset->nRow, -1);
	gtk_tree_selection_select_path(self->gtkTreeSelection, gtkTreePath);
	gtk_tree_path_free(gtkTreePath);
	Py_RETURN_NONE;
}


static PyObject*  // new ref, True if possible to move focus away
PxTable_render_focus(PxTableObject* self)
{
	//g_debug("*---- PxTable_render_focus");
	Py_RETURN_TRUE;
}

static int
PxTable_setattro(PxTableObject* self, PyObject* pyAttributeName, PyObject* pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "showRowIndicator") == 0) {
			if ((pyValue == Py_True) && !self->bShowRecordIndicator) {
				self->bShowRecordIndicator = true;
				self->gtkCellRendererRecordIndicator = gtk_cell_renderer_text_new();
				g_object_set(self->gtkCellRendererRecordIndicator, "background", "Light Gray", NULL);
				self->gtkTreeViewColumnRecordIndicator = gtk_tree_view_column_new();
				gtk_tree_view_column_pack_start(self->gtkTreeViewColumnRecordIndicator, self->gtkCellRendererRecordIndicator, FALSE);
				//gtk_tree_view_column_set_title(self->gtkTreeViewColumnRecordIndicator, "□");
				gtk_tree_view_column_set_alignment(self->gtkTreeViewColumnRecordIndicator, 0.5);
				gtk_tree_view_column_set_cell_data_func(self->gtkTreeViewColumnRecordIndicator, self->gtkCellRendererRecordIndicator, GtkTreeCell_RenderRowIndicator, self, NULL);
				g_object_set(self->gtkTreeViewColumnRecordIndicator, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);
				gtk_tree_view_column_set_resizable(self->gtkTreeViewColumnRecordIndicator, FALSE);
				gtk_tree_view_column_set_fixed_width(self->gtkTreeViewColumnRecordIndicator, 17);
				gtk_tree_view_insert_column(self->gtkTreeView, self->gtkTreeViewColumnRecordIndicator, 0);
			}/*
			else {
			}*/
			return 0;
		}
	}
	return PxTableType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static void
PxTable_dealloc(PxTableObject* self)
{
	if (self->gtk) {
		g_object_set_qdata(self->gtk, g.gQuark, NULL);
		gtk_widget_destroy(self->gtk);
	}
	Py_XDECREF(self->pyColumns);
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxTable_members[] = {
	{ "columns", T_OBJECT_EX, offsetof(PxTableObject, pyColumns), READONLY, "List of TableColumns" },
	{ NULL }
};

static PyMethodDef PxTable_methods[] = {
	{ "add_column", (PyCFunction)PxTable_add_column, METH_VARARGS | METH_KEYWORDS, "Add a column" },
	{ "refresh", (PyCFunction)PxTable_refresh, METH_NOARGS, "Pull fresh data" },
	{ "refresh_cell", (PyCFunction)PxTable_refresh_cell, METH_VARARGS, "Pull fresh data one cell" },
	{ "refresh_row_pointer", (PyCFunction)PxTable_refresh_row_pointer, METH_NOARGS, "Update highlight of selected row" },
	{ "render_focus", (PyCFunction)PxTable_render_focus, METH_NOARGS, "Return True if ready for focus to move on." },
	{ NULL }
};

PyTypeObject PxTableType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Table",             /* tp_name */
	sizeof(PxTableObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxTable_dealloc, /* tp_dealloc */
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
	PxTable_setattro,          /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Table widget objects ('List View' on Windows)", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxTable_methods,           /* tp_methods */
	PxTable_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxTable_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxTable_new,               /* tp_new */
};

static void
GtkTreeCell_Render(GtkTreeViewColumn* gtkTreeViewColumn, GtkCellRenderer* gtkCellRenderer, GtkTreeModel* gtkTreeModel, GtkTreeIter* gtkTreeIter, gpointer gUserData)
{
	guint iRow;
	PyObject* pyData = NULL, *pyRow, *pyDynasetColumn = NULL, *pyText = NULL;
	PxTableColumnObject* pyTableColumn = NULL;
	char* sText;

	if (gUserData == NULL) { // record indicator
		sText = "*";
	}
	else {
		gtk_tree_model_get(gtkTreeModel, gtkTreeIter, 0, &iRow, -1);
		pyTableColumn = (PxTableColumnObject*)gUserData;
		pyDynasetColumn = pyTableColumn->pyDynasetColumn;
		pyData = PxDynaset_GetData(pyTableColumn->pyTable->pyDynaset, (Py_ssize_t)iRow, pyDynasetColumn);

		if (!(pyText = PxFormatData(pyData, pyTableColumn->pyFormat))) {
			sText = "#Error#";
			PyErr_Print();
		}
		else {
			sText = PyUnicode_AsUTF8(pyText);
			Py_DECREF(pyText);
		}

		//g_object_set(renderer, "foreground-set", FALSE, NULL);
		//g_object_set(renderer, "foreground", "Red", "foreground-set", TRUE, NULL);
	}
	g_object_set(gtkCellRenderer, "text", sText, NULL);
}

static void
GtkTreeCell_RenderRowIndicator(GtkTreeViewColumn* gtkTreeViewColumn, GtkCellRenderer* gtkCellRenderer, GtkTreeModel* gtkTreeModel, GtkTreeIter* gtkTreeIter, gpointer gUserData)
{
	guint iRow;
	PyObject* pyRow;
	PxTableObject* pyTable;
	char* sText;

	gtk_tree_model_get(gtkTreeModel, gtkTreeIter, 0, &iRow, -1);
	pyTable = (PxTableObject*)gUserData;
	pyRow = PyList_GetItem(pyTable->pyDynaset->pyRows, (Py_ssize_t)iRow);

	if (PyStructSequence_GetItem(pyRow, PXDYNASETROW_DELETE) == Py_True)
		sText = "X"; // †×
	else if (PyStructSequence_GetItem(pyRow, PXDYNASETROW_NEW) == Py_True)
		sText = "*"; // ○☼
	else if (PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATAOLD) != Py_None)
		sText = "Δ"; // Ҩ
	else
		sText = " ";

	g_object_set(gtkCellRenderer, "text", sText, NULL);
}

static void
GtkTreeSelection_ChangedCB(GtkTreeSelection* gtkTreeSelection, gpointer gUserData)
{
	GtkTreeIter gtkTreeIter;
	GtkTreeModel* gtkTreeModel;
	gint iRow = -1;

	if (gtk_tree_selection_get_selected(gtkTreeSelection, &gtkTreeModel, &gtkTreeIter))
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, 0, &iRow, -1);
	if (!PxDynaset_SetRow(((PxTableObject*)gUserData)->pyDynaset, (Py_ssize_t)iRow))
		PythonErrorDialog();
}

static void
GtkCellRenderer_TextEditedCB(GtkCellRendererText* gtkCellRendererText, gchar* sPath, gchar* sText, gpointer gUserData)
{
	PyObject* pyCurrentData = NULL, *pyNewData = NULL;
	PxTableColumnObject* self = (PxTableColumnObject*)gUserData;
	gint* iRow = atoi(sPath);

	int iR = PxWindow_MoveFocus(self->pyTable->pyWindow, (PxWidgetObject*)self);
	if (iR == -1)
		goto ERROR;
	else if (iR == false)
		return;

	if ((pyNewData = PxParseString(sText, self->pyType, NULL)) == NULL)
		goto ERROR;

	pyCurrentData = PxDynaset_GetData(self->pyTable->pyDynaset, (Py_ssize_t)iRow, self->pyDynasetColumn);
	if (PyObject_RichCompareBool(pyCurrentData, pyNewData, Py_EQ)) {
		Py_DECREF(pyNewData);
		pyNewData = NULL;
		return;
	}

	if (PxDynaset_SetData(self->pyTable->pyDynaset, iRow, self->pyDynasetColumn, pyNewData))
		return;

ERROR:
	PythonErrorDialog();
}

static void
GtkCellRenderer_EditingStartedCB(GtkCellRendererText* gtkCellRendererText, GtkCellEditable* gtkCellEditable, const gchar* sPath, gpointer gUserData)
{
	PyObject* pyCurrentData = NULL;
	if (GTK_IS_ENTRY(gtkCellEditable)) {
		GtkEntry* gtkEntry = GTK_ENTRY(gtkCellEditable);
		GtkTreePath* gtkTreePath = gtk_tree_path_new_from_string(sPath);
		gint* gIndices = gtk_tree_path_get_indices(gtkTreePath);
		gint iRow = gIndices[0];
		gtk_tree_path_free(gtkTreePath);

		PxTableColumnObject* self = (PxTableColumnObject*)gUserData;
		pyCurrentData = PxDynaset_GetData(self->pyTable->pyDynaset, (Py_ssize_t)iRow, self->pyDynasetColumn);

		if (pyCurrentData == NULL || pyCurrentData == Py_None) {
			gtk_entry_set_text(gtkEntry, "");
			return;
		}

		PyObject* pyText = PxFormatData(pyCurrentData, self->pyFormatEdit ? self->pyFormatEdit : Py_None);
		if (pyText == NULL) {
			return;
		}

		gtk_entry_set_text(gtkEntry, PyUnicode_AsUTF8(pyText));
		Py_DECREF(pyText);
	}
}

static gboolean
GtkTreeView_FocusInEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	PxTableObject* self = (PxTableObject*)gUserData;

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

/* TableColumn -----------------------------------------------------------------------*/

static PyObject *
PxTableColumn_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxTableColumnObject* self = (PxTableColumnObject *)PxTableColumnType.tp_alloc(type, 0);

	if (self != NULL) {
		self->pyTable = NULL;
		self->pyDynasetColumn = NULL;
		self->pyType = NULL;
		self->pyFormat = NULL;
		self->bEditable = false;
		//self->pyWidget = NULL;
		self->iIndex = -1;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static PyObject*  // new ref
PxTableColumn_get_input_data(PxEntryObject* self)
{
	PyObject* pyData;
	const gchar* sText = gtk_entry_get_text(self->gtk);
	if ((pyData = PxParseString(sText, self->pyDataType, NULL)) == NULL) {
		PythonErrorDialog();
		Py_RETURN_NONE;
	}
	return pyData;
}

static PyObject*  // new ref, True if possible to move focus away
PxTableColumn_render_focus(PxEntryObject* self)
{
	PyObject* pyData = NULL, *pyResult = NULL;

	if (self->bClean) {
		Py_RETURN_TRUE;
	}
	/*
		pyData = PxTableColumn_get_input_data(self);
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
	*/
	self->bClean = true;
	Py_RETURN_TRUE;
}

static int
PxTableColumn_setattro(PxWidgetObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {

		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "validate") == 0) {
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyValidateCB);
				self->pyValidateCB = pyValue;
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

static void
PxTableColumn_dealloc(PxTableColumnObject* self)
{
	Py_XDECREF(self->pyTable);
	Py_XDECREF(self->pyDynasetColumn);
	Py_XDECREF(self->pyType);
	Py_XDECREF(self->pyFormat);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMemberDef PxTableColumn_members[] = {
	{ "data", T_OBJECT_EX, offsetof(PxTableColumnObject, pyDynasetColumn), READONLY, "Bound DynasetColumn" },
	{ "type", T_OBJECT_EX, offsetof(PxTableColumnObject, pyType), READONLY, "Data type" },
	{ "format", T_OBJECT_EX, offsetof(PxTableColumnObject, pyFormat), READONLY, "Display format" },
	//{ "widget", T_OBJECT_EX, offsetof(PxTableColumnObject, pyWidget), READONLY, "Edit widget" },
	{ NULL }
};

static PyMethodDef PxTableColumn_methods[] = {
	{ "render_focus", (PyCFunction)PxTableColumn_render_focus, METH_NOARGS, "Return True if ready for focus to move on." },
	{ NULL }
};

PyTypeObject PxTableColumnType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.TableColumn",       /* tp_name */
	sizeof(PxTableColumnObject), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxTableColumn_dealloc, /* tp_dealloc */
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
	PxTableColumn_setattro,    /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Colunm for a Table widget", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxTableColumn_methods,     /* tp_methods */
	PxTableColumn_members,     /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	0,                         /* tp_init */
	0,                         /* tp_alloc */
	PxTableColumn_new,         /* tp_new */
};
