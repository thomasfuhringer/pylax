// DynasetObject.h  | Pylax © 2016 by Thomas Führinger
#ifndef Px_DYNASETOBJECT_H
#define Px_DYNASETOBJECT_H

// indices of items in named structure "DynasetRow"
#define PXDYNASETROW_DATA 0
#define PXDYNASETROW_DATAOLD 1
#define PXDYNASETROW_NEW 2
#define PXDYNASETROW_DELETE 3

// indices of items in named structure "DynasetColumn"
#define PXDYNASETCOLUMN_NAME 0
#define PXDYNASETCOLUMN_INDEX 1
#define PXDYNASETCOLUMN_TYPE 2
#define PXDYNASETCOLUMN_KEY 3 // True = column is part of primary key, False = non-key database column, None = column not in database
#define PXDYNASETCOLUMN_DEFAULT 4
#define PXDYNASETCOLUMN_DEFFUNC 5
#define PXDYNASETCOLUMN_FORMAT 6 
#define PXDYNASETCOLUMN_PARENT 7 


typedef struct _PxWidgetObject PxWidgetObject;
typedef struct _PxButtonObject PxButtonObject;
typedef struct _PxDynasetObject PxDynasetObject;
typedef struct _PxDynasetObject
{
	PyObject_HEAD
		PxDynasetObject* pyParent;
	PyObject* pyConnection;
	PyObject* pyTable; // name of table in database
	PyObject* pyQuery;
	PyObject* pyCursor;
	PyObject* pyColumns; // PyDict
	PyObject* pyAutoColumn;
	PyObject* pyRows; // PyList
	PyObject* pyEmptyRowData; // Tuple
	LPCSTR strInsertSQL;
	LPCSTR strUpdateSQL;
	LPCSTR strDeleteSQL;
	PyObject* pyParams;
	Py_ssize_t nRows;
	Py_ssize_t nRow;
	Py_ssize_t nRowEnd; // for later, to select a region
	int iLastRowID;
	PyObject* pyWidgets; // PyList
	PyObject* pyChildren; // PyList
	BOOL bHasWhoCols;
	BOOL bAutoExecute;
	BOOL bFrozen;
	BOOL bLocked;
	BOOL bDirty;
	BOOL bFrozenIfDirty;
	BOOL bBroadcasting;
	PxButtonObject* pySearchButton;
	PxButtonObject* pyNewButton;
	PxButtonObject* pyEditButton;
	PxButtonObject* pyUndoButton;
	PxButtonObject* pySaveButton;
	PxButtonObject* pyDeleteButton;
	PxButtonObject* pyOkButton;
	PyObject* pyOnParentSelectionChangedCB;
	PyObject* pyBeforeSaveCB;
}
PxDynasetObject;

extern PyTypeObject PxDynasetType;
extern PyTypeObject PxDynasetColumnType;
extern PyTypeObject PxDynasetRowType;

BOOL PxDynasetTypes_Init();
PyObject* PxDynaset_GetData(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn);
BOOL PxDynaset_SetData(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn, PyObject* pyData);
PyObject* PxDynaset_execute(PxDynasetObject* self, PyObject *args);
BOOL PxDynaset_Clear(PxDynasetObject* self);
//PyObject* PxDynaset_GetKeyDict(PxDynasetObject* self, Py_ssize_t nRow);
PyObject* PxDynaset_GetRowDataDict(PxDynasetObject* self, Py_ssize_t nRow, BOOL bKeysOnly);
int PxDynaset_Save(PxDynasetObject* self);
BOOL PxDynaset_NewRow(PxDynasetObject* self, Py_ssize_t nRow);
BOOL PxDynaset_Undo(PxDynasetObject* self, Py_ssize_t nRow);
BOOL PxDynaset_DeleteRow(PxDynasetObject* self, Py_ssize_t nRow);
BOOL PxDynaset_AddWidget(PxDynasetObject*, PxWidgetObject*);
BOOL PxDynaset_RemoveWidget(PxDynasetObject* self, PxWidgetObject* widget);
BOOL PxDynaset_Stain(PxDynasetObject* self);
BOOL PxDynaset_Save(PxDynasetObject* self);
BOOL PxDynaset_SetRow(PxDynasetObject* self, Py_ssize_t nRow);
BOOL PxDynaset_DataChanged(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn);
BOOL PxDynaset_Freeze(PxDynasetObject* self);
BOOL PxDynaset_Thaw(PxDynasetObject* self);

#endif