// DynasetObject.h  | Pylax © 2017 by Thomas Führinger
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
typedef struct _PxDialogObject PxDialogObject;
typedef struct _PxDynasetObject PxDynasetObject;
typedef struct _PxDynasetObject
{
	PyObject_HEAD
		PxDynasetObject* pyParent;
	PyObject* pyConnection;
	PyObject* pyTable;    // name of table in database
	PyObject* pyQuery;
	PyObject* pyCursor;
	PyObject* pyColumns;  // PyDict
	PyObject* pyAutoColumn;  // column which gets automatically populated by the database by an ID
	PyObject* pyRows;     // PyList
	PyObject* pyEmptyRowData; // Tuple
	char* sInsertSQL;
	char* sUpdateSQL;
	char* sDeleteSQL;
	PyObject* pyParams;
	Py_ssize_t nRows;     // number of rows
	Py_ssize_t nRow;      // pointer to current row, -1 if none
	Py_ssize_t nRowEnd;   // for later, to select a region
	long iLastRowID;
	PyObject* pyWidgets;  // PyList
	PyObject* pyChildren; // PyList
	bool bHasWhoCols;
	bool bAutoExecute;
	bool bReadOnly;
	bool bLocked;         // can not be edited
	bool bFrozen;         // row pointer can not be moved
	bool bClean;          // no record has been edited
	PxButtonObject* pySearchButton;
	PxButtonObject* pyNewButton;
	PxButtonObject* pyEditButton;
	PxButtonObject* pyUndoButton;
	PxButtonObject* pySaveButton;
	PxButtonObject* pyDeleteButton;
	PxDialogObject* pyDialog;
	//PxButtonObject* pyOkButton;
	PyObject* pyOnParentSelectionChangedCB;
	PyObject* pyBeforeSaveCB;
}
PxDynasetObject;

extern PyTypeObject PxDynasetType;
extern PyTypeObject PxDynasetColumnType;
extern PyTypeObject PxDynasetRowType;

bool PxDynasetTypes_Init(void);
PyObject* PxDynaset_GetData(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn);
bool PxDynaset_SetData(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn, PyObject* pyData);
PyObject* PxDynaset_execute(PxDynasetObject* self, PyObject* args, PyObject* kwds);
bool PxDynaset_Clear(PxDynasetObject* self);
PyObject* PxDynaset_GetRowDataDict(PxDynasetObject* self, Py_ssize_t nRow, bool bKeysOnly);
bool PxDynaset_Save(PxDynasetObject* self);
bool PxDynaset_NewRow(PxDynasetObject* self, Py_ssize_t nRow);
bool PxDynaset_Undo(PxDynasetObject* self, Py_ssize_t nRow);
bool PxDynaset_DeleteRow(PxDynasetObject* self, Py_ssize_t nRow);
bool PxDynaset_AddWidget(PxDynasetObject*, PxWidgetObject*);
bool PxDynaset_RemoveWidget(PxDynasetObject* self, PxWidgetObject* widget);
bool PxDynaset_Freeze(PxDynasetObject* self);
bool PxDynaset_Thaw(PxDynasetObject* self);
bool PxDynaset_Stain(PxDynasetObject* self);
bool PxDynaset_UnStain(PxDynasetObject* self);
bool PxDynaset_SetRow(PxDynasetObject* self, Py_ssize_t nRow);
bool PxDynaset_DataChanged(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn);

#endif
