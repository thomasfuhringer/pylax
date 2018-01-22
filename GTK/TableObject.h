// TableObject.h  | Pylax © 2017 by Thomas Führinger
#ifndef Px_TABLEOBJECT_H
#define Px_TABLEOBJECT_H

typedef struct _PxTableObject
{
	PxWidgetObject_HEAD
		PyObject* pyColumns; // PyList
	Py_ssize_t nColumns;
	int iAutoSizeColumn;
	bool bShowRecordIndicator;
	GtkTreeView* gtkTreeView;
	GtkListStore* gtkListStore;
	GtkTreeSelection* gtkTreeSelection;
	GtkTreeViewColumn* gtkTreeViewColumnRecordIndicator;
	GtkCellRenderer* gtkCellRendererRecordIndicator;
	gulong gtkTreeSelectionChangedHandlerID;
	//int iFocusRow;
	//int iFocusColumn;
}
PxTableObject;

typedef struct _PxTableColumnObject
{
	PyObject_HEAD
		PxTableObject* pyTable;
	PyObject* pyDynasetColumn;
	int iIndex;
	PyObject* pyType;
	PyObject* pyFormat;
	PyObject* pyFormatEdit;
	bool bEditable;
	//PxWidgetObject* pyWidget;
	GtkTreeViewColumn* gtkTreeViewColumn;
	GtkCellRenderer* gtkCellRenderer;
}
PxTableColumnObject;

extern PyTypeObject PxTableType;
extern PyTypeObject PxTableColumnType;

bool PxTable_SelectionChanged(PxTableObject* self, int iRow);

#endif
