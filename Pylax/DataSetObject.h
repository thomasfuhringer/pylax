// DataSetObject.h  | Pylax © 2016 by Thomas Führinger
#ifndef Px_DATASETOBJECT_H
#define Px_DATASETOBJECT_H

// indices of items in named structure "DataSetRow"
#define PXDATASETROW_DATA 0
#define PXDATASETROW_DATAOLD 1
#define PXDATASETROW_NEW 2
#define PXDATASETROW_DELETE 3

// indices of items in named structure "DataSetColumn"
#define PXDATASETCOLUMN_NAME 0
#define PXDATASETCOLUMN_INDEX 1
#define PXDATASETCOLUMN_TYPE 2
#define PXDATASETCOLUMN_KEY 3 // True = column is part of primary key, False = non-key database column, None = column not in database
#define PXDATASETCOLUMN_DEFAULT 4
#define PXDATASETCOLUMN_DEFFUNC 5
#define PXDATASETCOLUMN_FORMAT 6 
#define PXDATASETCOLUMN_PARENT 7 


typedef struct _PxWidgetObject PxWidgetObject;
typedef struct _PxButtonObject PxButtonObject;
typedef struct _PxDataSetObject PxDataSetObject;
typedef struct _PxDataSetObject
{
	PyObject_HEAD
		PxDataSetObject* pyParent;
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
PxDataSetObject;

extern PyTypeObject PxDataSetType;
extern PyTypeObject PxDataSetColumnType;
extern PyTypeObject PxDataSetRowType;

BOOL PxDataSetTypes_Init();
PyObject* PxDataSet_GetData(PxDataSetObject* self, Py_ssize_t nRow, PyObject* pyColumn);
BOOL PxDataSet_SetData(PxDataSetObject* self, Py_ssize_t nRow, PyObject* pyColumn, PyObject* pyData);
PyObject* PxDataSet_execute(PxDataSetObject* self, PyObject *args);
BOOL PxDataSet_Clear(PxDataSetObject* self);
//PyObject* PxDataSet_GetKeyDict(PxDataSetObject* self, Py_ssize_t nRow);
PyObject* PxDataSet_GetRowDataDict(PxDataSetObject* self, Py_ssize_t nRow, BOOL bKeysOnly);
int PxDataSet_Save(PxDataSetObject* self);
BOOL PxDataSet_NewRow(PxDataSetObject* self, Py_ssize_t nRow);
BOOL PxDataSet_Undo(PxDataSetObject* self, Py_ssize_t nRow);
BOOL PxDataSet_DeleteRow(PxDataSetObject* self, Py_ssize_t nRow);
BOOL PxDataSet_AddWidget(PxDataSetObject*, PxWidgetObject*);
BOOL PxDataSet_RemoveWidget(PxDataSetObject* self, PxWidgetObject* widget);
BOOL PxDataSet_Stain(PxDataSetObject* self);
BOOL PxDataSet_Save(PxDataSetObject* self);
BOOL PxDataSet_SetRow(PxDataSetObject* self, Py_ssize_t nRow);
BOOL PxDataSet_DataChanged(PxDataSetObject* self, Py_ssize_t nRow, PyObject* pyColumn);
BOOL PxDataSet_Freeze(PxDataSetObject* self);
BOOL PxDataSet_Thaw(PxDataSetObject* self);

#endif