// TableObject.h  | Pylax © 2015 by Thomas Führinger
#ifndef Px_TABLEOBJECT_H
#define Px_TABLEOBJECT_H

typedef struct _PxTableObject
{
	PxWidgetObject_HEAD
		PyObject* pyColumns; // PyList
	Py_ssize_t nColumns;
	int iAutoSizeColumn;
	BOOL bShowRecordIndicator;
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
	PxWidgetObject* pyWidget;
}
PxTableColumnObject;

extern PyTypeObject PxTableType;
extern PyTypeObject PxTableColumnType;

BOOL PxTable_SelectionChanged(PxTableObject* self, int iRow);
//int PxTable_Notify(PxTableObject* self, LPNMHDR nmhdr);

#endif /* !Px_TABLEOBJECT_H */

