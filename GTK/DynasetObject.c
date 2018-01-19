// DynasetObject.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

// Forward declarations
static bool PxDynaset_UpdateControlWidgets(PxDynasetObject* self);
static bool PxDynaset_RefreshBoundWidgets(PxDynasetObject* self, bool bNonTable, bool bTable, bool bRowPointer);
static bool PxDynaset_CleanUp(PxDynasetObject* self);
static int PxDynaset_Write(PxDynasetObject* self);

static PyStructSequence_Field PxDynasetColumnFields[] = {
	{ "name", "Name of column in query" },
	{ "index", "Position in query" },
	{ "type", "Data type" },
	{ "key", "True if column is part of the primary key, False if non-key database column, None if not part of the database table" },
	{ "default", "Default value" },
	{ "get_default", "Function providing default value" },
	{ "format", "Default display format" },
	{ "parent", "Coresponding column in parent Dynaset" },
	{ NULL }
};

static PyStructSequence_Desc PxDynasetColumnDesc = {
	"DynasetColumn",
	NULL,
	PxDynasetColumnFields,
	8
};

static PyStructSequence_Field PxDynasetRowFields[] = {
	{ "data", "Tuple of data pulled" },
	{ "dataOld", "Tuple of data before modification" },
	{ "new", "True if row is still not in database" },
	{ "delete", "True row to be removed from the database" },
	{ NULL }
};

static PyStructSequence_Desc PxDynasetRowDesc = {
	"DynasetRow",
	NULL,
	PxDynasetRowFields,
	4
};

bool
PxDynasetTypes_Init()
{
	if (PxDynasetColumnType.tp_name == 0)
		PyStructSequence_InitType(&PxDynasetColumnType, &PxDynasetColumnDesc);
	Py_INCREF(&PxDynasetColumnType);
	if (PxDynasetRowType.tp_name == 0)
		PyStructSequence_InitType(&PxDynasetRowType, &PxDynasetRowDesc);
	Py_INCREF(&PxDynasetRowType);
	return true;
}

static PyObject *
PxDynaset_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PxDynasetObject* self = (PxDynasetObject *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->pyParent = NULL;
		self->pyConnection = NULL;
		self->pyTable = NULL;
		self->bHasWhoCols = true;
		self->pyQuery = NULL;
		self->pyCursor = NULL;
		self->sInsertSQL = NULL;
		self->sUpdateSQL = NULL;
		self->sDeleteSQL = NULL;
		self->pyAutoColumn = NULL;
		self->pyParams = NULL;
		self->pyEmptyRowData = NULL;
		self->pyColumns = NULL;
		self->pyRows = NULL;
		self->nRows = 0;
		self->nRow = -1;
		self->nRowEnd = -1;
		self->iLastRowID = -1;
		self->bAutoExecute = true;
		self->bReadOnly = false;
		self->bLocked = true;
		self->bFrozen = false;
		self->bClean = true;   // no pendig changes
		self->pySearchButton = NULL;
		self->pyNewButton = NULL;
		self->pyEditButton = NULL;
		self->pyUndoButton = NULL;
		self->pySaveButton = NULL;
		self->pyDeleteButton = NULL;
		self->pyDialog = NULL;
		self->pyWidgets = NULL;
		self->pyChildren = NULL;
		self->pyOnParentSelectionChangedCB = NULL;
		self->pyBeforeSaveCB = NULL;

		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxDynaset_init(PxDynasetObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "table", "query", "parent", "cnx", NULL };
	PyObject* pyTable = NULL, *pyQuery = NULL, *pyParent = NULL, *pyConnection = NULL, *tmp;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist,
		&pyTable,
		&pyQuery,
		&pyParent,
		&pyConnection))
		return -1;

	if (PyUnicode_Check(pyTable))
		PxAttachObject(&self->pyTable, pyTable, true);
	else {
		PyErr_Format(PyExc_TypeError, "Parameter 1 ('table') must be string, not '%.200s'.", pyTable->ob_type->tp_name);
		return -1;
	}

	if (pyQuery) {
		if (PyUnicode_Check(pyQuery))
			PxAttachObject(&self->pyQuery, pyQuery, true);
		else {
			PyErr_Format(PyExc_TypeError, "Parameter 2 ('query') must be string, not '%.200s'.", pyQuery->ob_type->tp_name);
			return -1;
		}
	}
	else
		self->pyQuery = NULL;

	if (pyParent) {
		if (PyObject_TypeCheck(pyParent, &PxDynasetType)) {
			PxAttachObject(&self->pyParent, pyParent, true);
			if (PyList_Append(self->pyParent->pyChildren, (PyObject*)self) == -1)
				return -1;
		}
		else {
			PyErr_Format(PyExc_TypeError, "Parameter 3 ('parent') must be a Dynaset, not '%.200s'.", pyParent->ob_type->tp_name);
			return -1;
		}
	}

	if (pyConnection) {
		//if (PyObject_TypeCheck(pyConnection, &pysqlite_ConnectionType)) {
		//if pyConnectiono->ob_type == Hinterland...
		if (true)
			PxAttachObject(&self->pyConnection, pyConnection, true);
		else {
			PyErr_Format(PyExc_TypeError, "Parameter 4 ('cnx') must be a SQLite connection, not '%.200s'.", pyConnection->ob_type->tp_name);
			return -1;
		}
	}
	else {
		Py_INCREF(g.pyConnection);
		self->pyConnection = g.pyConnection;
	}

	if ((self->pyRows = PyList_New(0)) == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Can not create list of rows.");
		return -1;
	}
	if ((self->pyColumns = PyDict_New()) == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Can not create dict of columns.");
		return -1;
	}
	if ((self->pyWidgets = PyList_New(0)) == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Can not create list of widgets.");
		return -1;
	}
	if ((self->pyChildren = PyList_New(0)) == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Can not create list of children.");
		return -1;
	}

	return 0;
}

static PyObject* // new ref
PxDynaset_add_column(PxDynasetObject* self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "name", "type", "key", "format", "default", "defaultFunction", "parent", NULL };
	PyObject* pyName = NULL, *pyType = NULL, *pyKey = NULL, *pyFormat = NULL, *pyDefault = NULL, *pyDefaultFunction = NULL, *pyParent = NULL, *pyParentColumn = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOO", kwlist,
		&pyName,
		&pyType,
		&pyKey,
		&pyFormat,
		&pyDefault,
		&pyDefaultFunction,
		&pyParent))
		return NULL;

	if (!PyUnicode_Check(pyName)) {
		PyErr_SetString(PyExc_TypeError, "Parameter 1 ('name') must be a string.");
		return NULL;
	}

	if (pyType) {
		if (!PyType_Check(pyType)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 2 ('type') must be a Type Object.");
			return NULL;
		}
	}
	else {
		pyType = (PyObject*)&PyUnicode_Type;
	}

	if (pyKey) {
		if (pyKey != Py_True && pyKey != Py_False && pyKey != Py_None) {
			PyErr_SetString(PyExc_TypeError, "Parameter 3 ('key') must be a boolean or None.");
			return NULL;
		}
	}
	else {
		pyKey = Py_False;
	}

	if (pyFormat) {
		if (!PyUnicode_Check(pyFormat)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 4 ('format') must be a string.");
			return NULL;
		}
	}
	else {
		pyFormat = Py_None;
	}

	pyDefault = pyDefault ? pyDefault : Py_None;

	if (pyDefaultFunction) {
		if (!PyCallable_Check(pyDefaultFunction)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 5 ('defaultFunction') must be a callable object.");
			return NULL;
		}
	}
	else
		pyDefaultFunction = Py_None;

	if (pyParent) {
		if (PyObject_TypeCheck(pyParent, &PxDynasetType))
			pyParentColumn = pyParent;
		else if (PyUnicode_Check(pyParent)) {
			pyParentColumn = PyDict_GetItem(self->pyParent->pyColumns, pyParent);
			if (!pyParentColumn)
				return PyErr_Format(PyExc_AttributeError, "Parent Dynaset has no column named '%s'.", PyUnicode_AsUTF8(pyParent));
		}
		else {
			PyErr_SetString(PyExc_TypeError, "Parameter 6 ('parent') must be a Dynaset or a str.");
			return NULL;
		}
	}
	else
		pyParentColumn = Py_None;

	Py_INCREF(pyName);
	Py_INCREF(Py_None);
	Py_INCREF(pyType);
	Py_INCREF(pyKey);
	Py_INCREF(pyFormat);
	Py_INCREF(pyDefault);
	Py_INCREF(pyDefaultFunction);
	Py_INCREF(pyParentColumn);

	PyObject* pyColumn = PyStructSequence_New(&PxDynasetColumnType);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_NAME, pyName);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_INDEX, Py_None);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_TYPE, pyType);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_KEY, pyKey);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_FORMAT, pyFormat);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_DEFAULT, pyDefault);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_DEFFUNC, pyDefaultFunction);
	PyStructSequence_SET_ITEM(pyColumn, PXDYNASETCOLUMN_PARENT, pyParentColumn);

	//Py_INCREF(pyName);
	if (PyDict_SetItem(self->pyColumns, pyName, pyColumn) == -1) {
		return NULL;
	}
	return(pyColumn);
}

static PyObject* // new ref
PxDynaset_get_column(PxDynasetObject* self, PyObject *args)
{
	PyObject* pyColumnName, *pyColumn;
	if (!PyArg_ParseTuple(args, "O", &pyColumnName)) {
		return NULL;
	}
	pyColumn = PyDict_GetItem(self->pyColumns, pyColumnName);
	Py_INCREF(pyColumn);
	return pyColumn;
}

PyObject* // borrowed ref
PxDynaset_GetData(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn)
{
	if (nRow == -1)
		nRow = self->nRow;
	if (nRow < 0 || nRow > self->nRows) {
		PyErr_SetString(PyExc_IndexError, "Cannot get data from Dynaset. Row number out of range.");
		//g_debug("PxDynaset_GetData nRow %d", nRow);
		return NULL;
	}

	Py_ssize_t nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));
	PyObject* pyRow = PyList_GetItem(self->pyRows, nRow);
	PyObject* pyRowData = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATA);
	PyObject* pyDataItem = PyTuple_GetItem(pyRowData, nColumn);
	return pyDataItem;
}

static PyObject* // new ref
PxDynaset_get_data(PxDynasetObject* self, PyObject *args)
{
	Py_ssize_t nRow = self->nRow;
	PyObject* pyColumn, *pyData;
	if (!PyArg_ParseTuple(args, "O|n", &pyColumn, &nRow)) {
		return NULL;
	}
	if (nRow < -1 || nRow > self->nRows) {
		PyErr_SetString(PyExc_IndexError, "Cannot get data from Dynaset. Row number out of range.");
		//g_debug("PxDynaset_GetData nRow %d", nRow);
		return NULL;
	}

	if (PyUnicode_Check(pyColumn)) {
		pyData = pyColumn;
		pyColumn = PyDict_GetItem(self->pyColumns, pyData);
		if (!pyColumn)
			return PyErr_Format(PyExc_AttributeError, "Dynaset has no column named '%s'.", PyUnicode_AsUTF8(pyData));
	}
	pyData = PxDynaset_GetData(self, nRow, pyColumn);

	if (pyData == NULL)
		return NULL;
	Py_INCREF(pyData);
	return pyData;
}

bool
PxDynaset_SetData(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn, PyObject* pyData)
{
	Py_ssize_t nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));
	PyObject* pyRow = PyList_GetItem(self->pyRows, nRow);
	PyObject* pyRowData = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATA);
	PyObject* pyRowDataOld = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATAOLD);
	PyObject* pyRowNew = PyStructSequence_GetItem(pyRow, PXDYNASETROW_NEW);
	PyObject* pyDataOld = NULL;
	//g_debug("*---- PxDynaset_SetData");

	if (pyRowDataOld == Py_None && pyRowNew == Py_False) { // keep a copy of the original data tuple
		Py_ssize_t nSize = PyTuple_Size(pyRowData);
		pyRowDataOld = PyTuple_Duplicate(pyRowData);
		Py_DECREF(Py_None);
		PyStructSequence_SET_ITEM(pyRow, PXDYNASETROW_DATAOLD, pyRowDataOld);
	}
	pyDataOld = PyTuple_GetItem(pyRowData, nColumn);
	PyTuple_SET_ITEM(pyRowData, nColumn, pyData);
	Py_XDECREF(pyDataOld);
	PxDynaset_Stain(self);

	return PxDynaset_DataChanged(self, nRow, pyColumn);
}

static PyObject* // new ref
PxDynaset_set_data(PxDynasetObject* self, PyObject *args)
{
	Py_ssize_t nRow = self->nRow;
	PyObject* pyColumn, *pyData;
	if (!PyArg_ParseTuple(args, "OO|n", &pyColumn, &pyData, &nRow)) {
		return NULL;
	}
	if (nRow < -1 || nRow > self->nRows) {
		PyErr_SetString(PyExc_IndexError, "Cannot set data in Dynaset. Row number out of range.");
		//g_debug("PxDynaset_set_data nRow %d", nRow);
		return NULL;
	}

	if (PyUnicode_Check(pyColumn)) {
		pyData = pyColumn;
		pyColumn = PyDict_GetItem(self->pyColumns, pyData);
		if (!pyColumn)
			return PyErr_Format(PyExc_AttributeError, "Dynaset has no column named '%s'.", PyUnicode_AsUTF8(pyData));
	}

	if (!PxDynaset_SetData(self, nRow, pyColumn, pyData))
		return NULL;
	Py_RETURN_NONE;
}

static PyObject* // new ref
PxDynaset_get_row(PxDynasetObject* self, PyObject* args)
{
	Py_ssize_t nRow = self->nRow;
	PyObject* pyRow = NULL;

	if (!PyArg_ParseTuple(args, "|n", &nRow)) {
		return NULL;
	}
	if (nRow < 0)
		Py_RETURN_NONE;
	else if (nRow > self->nRows) {
		PyErr_SetString(PyExc_IndexError, "Cannot get row from Dynaset. Row number out of range.");
		//g_debug("PxDynaset_get_row nRow %d", nRow);
		return NULL;
	}
	pyRow = PyList_GetItem(self->pyRows, nRow);
	if (pyRow) {
		Py_INCREF(pyRow);
		return pyRow;
	}
	else
		return NULL;
}

static PyObject* // new ref
PxDynaset_get_row_data(PxDynasetObject* self, PyObject* args)
{
	Py_ssize_t nRow = self->nRow;
	PyObject* pyRow = NULL;

	if (!PyArg_ParseTuple(args, "|n", &nRow)) {
		return NULL;
	}
	if (nRow < 0)
		Py_RETURN_NONE;
	else if (nRow > self->nRows) {
		PyErr_SetString(PyExc_IndexError, "Cannot get row data from Dynaset. Row number out of range.");
		return NULL;
	}
	return PxDynaset_GetRowDataDict(self, nRow, false);
}

bool
PxDynaset_Clear(PxDynasetObject* self)
{
	if (self->nRows == 0)
		return true;

	else if (!PxDynaset_SetRow(self, -1))
		return false;

	Py_DECREF(self->pyRows);
	self->pyRows = PyList_New(0);
	self->nRows = 0;
	self->nRow = -1;
	Py_XDECREF(self->pyEmptyRowData);
	self->pyEmptyRowData = NULL;

	// refresh bound table widgets
	if (!PxDynaset_RefreshBoundWidgets(self, false, true, false))
		return false;
	return true;
}

static PyObject* // new ref
PxDynaset_clear(PxDynasetObject* self, PyObject* args)
{
	if (!PxDynaset_Clear(self))
		return NULL;
	Py_RETURN_NONE;
}

PyObject* // new ref
PxDynaset_execute(PxDynasetObject* self, PyObject* args, PyObject* kwds)
{
	static char *kwlist[] = { "parameters", "query", NULL };
	PyObject* pyParameters = NULL, *pyQuery = NULL, *pyResult = NULL;
	if (args && !PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &pyParameters, &pyQuery)) {
		return NULL;
	}

	if (pyParameters) {
		if (!PyDict_Check(pyParameters)) {
			PyErr_SetString(PyExc_TypeError, "Parameter 1 ('parameters') must be a dict.");
			return NULL;
		}
	}
	else if (self->pyParent) {
		PyObject* pyColumn, *pyParentDynasetColumn, *pyParentDynasetColumnName, *pyColumnName, *pyData;
		Py_ssize_t nPos = 0;
		pyParameters = PyDict_New();

		while (PyDict_Next(self->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
			if (PyErr_Occurred()) {
				PyErr_Print();
				return NULL;
			}
			pyParentDynasetColumn = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_PARENT);
			if (pyParentDynasetColumn != Py_None) {
				pyData = PxDynaset_GetData(self->pyParent, self->pyParent->nRow, pyParentDynasetColumn);
				pyParentDynasetColumnName = PyStructSequence_GetItem(pyParentDynasetColumn, PXDYNASETCOLUMN_NAME);
				if (PyDict_SetItem(pyParameters, pyParentDynasetColumnName /*pyColumnName*/, pyData) == -1) {
					return NULL;
				}
			}
		}

		if (PyDict_Size(pyParameters) == 0) {
			Py_DECREF(pyParameters);
			pyParameters = NULL;
		}
	}

	if (pyQuery) {
		if (PyUnicode_Check(pyQuery))
			PxAttachObject(&self->pyQuery, pyQuery, true);
		else {
			PyErr_SetString(PyExc_TypeError, "Parameter 2 ('query') must be a string.");
			return NULL;
		}
	}

	if (!PxDynaset_Clear(self))
		return NULL;

	if (self->pyCursor && PyObject_CallMethod(self->pyCursor, "close", NULL) == NULL)
		return NULL;

	if ((self->pyCursor = PyObject_CallMethod(self->pyConnection, "cursor", NULL)) == NULL) {
		return NULL;
	}

	const char* sQuery = PyUnicode_AsUTF8(self->pyQuery);
	if (pyParameters)
		pyResult = PyObject_CallMethod(self->pyCursor, "execute", "(sO)", sQuery, pyParameters);
	else
		pyResult = PyObject_CallMethod(self->pyCursor, "execute", "(s)", sQuery);
	if (pyResult == NULL) {
		return NULL;
	}

	PyObject* pyColumnDescriptions = PyObject_GetAttrString(self->pyCursor, "description");
	PyObject* pyIterator = PyObject_GetIter(pyColumnDescriptions);
	PyObject* pyItem, *pyColumnName, *pyColumn, *pyIndex;
	Py_ssize_t nIndex = 0;
	if (pyIterator == NULL) {
		return NULL;
	}

	// make my columns' index numbers point to correct position in query result tuples
	while (pyItem = PyIter_Next(pyIterator)) {
		pyColumnName = PyTuple_GetItem(pyItem, 0);
		if (pyColumnName == NULL)
			return NULL;

		pyColumn = PyDict_GetItem(self->pyColumns, pyColumnName);
		if (pyColumn != NULL) {
			pyIndex = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX);
			Py_DECREF(pyIndex);
			PyStructSequence_SetItem(pyColumn, PXDYNASETCOLUMN_INDEX, PyLong_FromSsize_t(nIndex));
		}
		else {
			return PyErr_Format(PyExc_AttributeError, "Column '%s' of query not contained in Dynaset's column list.", PyUnicode_AsUTF8(pyColumnName));
		}
		nIndex++;
		Py_DECREF(pyItem);
	}
	Py_DECREF(pyIterator);

	// create Dynaset rows and reference to query result tuples
	PyObject* pyRow = NULL;
	self->nRows = 0;
	while (pyItem = PyIter_Next(pyResult)) {
		//Py_INCREF(pyItem); // ??
		Py_INCREF(Py_None);
		Py_INCREF(Py_False);
		Py_INCREF(Py_False);
		pyRow = PyStructSequence_New(&PxDynasetRowType);
		PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATA, pyItem);
		PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATAOLD, Py_None);
		PyStructSequence_SetItem(pyRow, PXDYNASETROW_NEW, Py_False);
		PyStructSequence_SetItem(pyRow, PXDYNASETROW_DELETE, Py_False);

		if (PyList_Append(self->pyRows, pyRow) == -1) {
			return NULL;
		}
        Py_DECREF(pyRow);
		self->nRows++;
	}
	Py_DECREF(pyResult);
	Py_DECREF(pyColumnDescriptions);
	if (self->pyParent)
		Py_XDECREF(pyParameters);

	if (PyErr_Occurred()) {
		return NULL;
	}

	// notify table widgets
	if (!PxDynaset_RefreshBoundWidgets(self, false, true, false))
		return NULL;

	return PyLong_FromSsize_t(self->nRows);
}

PyObject* // new ref
PxDynaset_GetRowDataDict(PxDynasetObject* self, Py_ssize_t nRow, bool bKeysOnly)
{
	PyObject* pyColumnName, *pyColumn, *pyRow, *pyRowData, *pyData, *pyIsKey, *pyRowDataDict;
	Py_ssize_t nColumn, nPos = 0;
	if (nRow == -1)
		nRow = self->nRow;
	if (nRow == -1)
		Py_RETURN_NONE;
	pyRow = PyList_GetItem(self->pyRows, nRow);
	pyRowData = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATA);
	pyRowDataDict = PyDict_New();

	while (PyDict_Next(self->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
		if (PyErr_Occurred()) {
			PyErr_Print();
			return NULL;
		}
		nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));
		pyIsKey = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_KEY);
		pyData = PyTuple_GetItem(pyRowData, nColumn);
		//XX(pyData);
		if (!bKeysOnly || pyIsKey == Py_True)
			if (PyDict_SetItem(pyRowDataDict, pyColumnName, pyData) == -1) // PyDict_SetItem increfs...
				return NULL;
	}
	return pyRowDataDict;
}

static bool
PxDynaset_UpdateAutoColumnInChildren(PxDynasetObject* self, PyObject* pyParentColumn, PyObject* pyLastRowID)
{
	PyObject* pyColumnName, *pyColumn, *pyRow, *pyRowData;
	PxDynasetObject* pyChild;
	Py_ssize_t n, nLen, nRows, nRow, nPos;

	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyChild = (PxDynasetObject*)PyList_GetItem(self->pyChildren, n);
		while (PyDict_Next(pyChild->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
			if (PyErr_Occurred()) {
				PyErr_Print();
				return false;
			}
			if (PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_PARENT) == pyParentColumn) {
				nRows = PySequence_Size(pyChild->pyRows);
				for (nRow = 0; nRow < nRows; nRow++) {
					pyRow = PyList_GetItem(pyChild->pyRows, nRow);
					pyRowData = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATA);
					PyTuple_SET_ITEM(pyRowData, nPos, pyLastRowID);
				}
				PxDynaset_UpdateAutoColumnInChildren(pyChild, pyColumn, pyLastRowID);
			}
		}
	}
}

bool
PxDynaset_Save(PxDynasetObject* self)
{
	int iRecordsChanged = 0;
	PyObject* pyOk;
	char sMessage[30];

	iRecordsChanged = PxDynaset_Write(self);
	if (iRecordsChanged == -1) {
		pyOk = PyObject_CallMethod(self->pyConnection, "rollback", NULL);
		if (pyOk != NULL)
			Py_DECREF(pyOk);
		return false;
	}
	else {
		pyOk = PyObject_CallMethod(self->pyConnection, "commit", NULL);
		if (pyOk == NULL)
			return false;
		else {
			Py_DECREF(pyOk);
			if (!PxDynaset_CleanUp(self))
				return false;
			if (!PxDynaset_Thaw(self))
				return false;
			sprintf(sMessage, "Records updated: %d", iRecordsChanged);
			gtk_statusbar_push(g.gtkStatusbar, 1, sMessage);
		}
	}
	return true;
}

static int
PxDynaset_Write(PxDynasetObject* self)
{
	PyObject* pyResult, *pyColumnName, *pyColumn, *pyRow, *pyRowData, *pyRowDataOld, *pyRowDelete, *pyRowNew, *pyData, *pyIsKey, *pyParams, *pyCursor, *pyLastRowID, *tmp;
	Py_ssize_t nRow, nColumn, nPos;
	int iRecordsChanged = 0;
	int iChildRecordsChanged = 0;
	char* sSql;
	char* sSql2;
	PxDynasetObject* pyChild;
	Py_ssize_t n, nLen;

	if (self->pyBeforeSaveCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		//Py_INCREF(self);
		PyObject* pyResult = PyObject_CallObject(self->pyBeforeSaveCB, pyArgs);
		Py_DECREF(pyArgs);
		if (pyResult == NULL)
			return -1;
		//else if (pyResult == Py_False) {
		//	Py_DECREF(pyResult);
		//	return 0;
		//}
		else Py_DECREF(pyResult);
	}

	// iterate over own rows
	nLen = PySequence_Size(self->pyRows);
	for (nRow = 0; nRow < nLen; nRow++) {
		pyRow = PyList_GetItem(self->pyRows, nRow);
		pyRowData = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATA);
		pyRowDataOld = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATAOLD);
		pyRowDelete = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DELETE);
		pyRowNew = PyStructSequence_GetItem(pyRow, PXDYNASETROW_NEW);
		pyParams = PyList_New(0);
		nPos = 0;

		// DELETE
		if (pyRowDelete == Py_True) {
			if (pyRowNew == Py_False) {
				char* sArr[3] = { "DELETE FROM ", PyUnicode_AsUTF8(self->pyTable), " WHERE " };
				sSql = StringArrayCat(sArr, 3);

				while (PyDict_Next(self->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
					if (PyErr_Occurred()) {
						PyErr_Print();
						return -1;
					}
					nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));
					pyIsKey = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_KEY);
					pyData = PyTuple_GetItem(pyRowData, nColumn);
					if (pyIsKey == Py_True) {
						sSql = StringAppend2(sSql, PyUnicode_AsUTF8(pyColumnName), "=? AND ");
						if (PyList_Append(pyParams, pyData) == -1) {
							return -1;
						}
					}
				}

				memset(sSql + strlen(sSql) - 5, '\0', 1); // cut off final ' AND '
				sSql = StringAppend2(sSql, ";", NULL);

				if (PySequence_Size(pyParams) > 0) {
					pyCursor = PyObject_CallMethod(self->pyConnection, "execute", "(sO)", sSql, PyList_AsTuple(pyParams));
					if (pyCursor == NULL) {
						return -1;
					}
					iRecordsChanged++;
				}
				else {
					PyErr_SetString(PyExc_RuntimeError, "No key columns. Can not delete.");
					return -1;
				}
				Py_DECREF(pyCursor);

				if (self->sDeleteSQL)
					PyMem_RawFree(self->sDeleteSQL);
				self->sDeleteSQL = sSql;

				Py_XDECREF(self->pyParams);
				self->pyParams = pyParams;

			}
		}
		// INSERT
		else if (pyRowNew == Py_True) {
			//g_debug("INSERT");
			char* sArr[3] = { "INSERT INTO ", PyUnicode_AsUTF8(self->pyTable), " (" };
			sSql = StringArrayCat(sArr, 3);
			sSql2 = StringAppend(NULL, ") VALUES (");

			while (PyDict_Next(self->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
				if (PyErr_Occurred()) {
					PyErr_Print();
					return -1;
				}
				nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));
				pyIsKey = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_KEY);
				pyData = PyTuple_GetItem(pyRowData, nColumn);

				if (pyIsKey != Py_None && self->pyAutoColumn != pyColumn) {
					sSql = StringAppend2(sSql, PyUnicode_AsUTF8(pyColumnName), ",");
					sSql2 = StringAppend(sSql2, "?,");
					if (PyList_Append(pyParams, pyData) == -1) {
						return -1;
					}
				}
				//Py_DECREF(pyData);
			}

			if (PySequence_Size(pyParams) == 0) {
				PyErr_SetString(PyExc_RuntimeError, "No columns.");
				return -1;
			}

			memset(sSql + strlen(sSql) - 1, '\0', 1); // cut off final comma
			memset(sSql2 + strlen(sSql2) - 1, '\0', 1);
			sSql = StringAppend2(sSql, sSql2, ");");

			//XX(pyParams);
			pyCursor = PyObject_CallMethod(self->pyConnection, "execute", "(sO)", sSql, PyList_AsTuple(pyParams));
			if (pyCursor == NULL) {
				PyErr_Print();
				return -1;
			}

			if (self->pyAutoColumn) {
				pyLastRowID = PyObject_GetAttrString(pyCursor, "lastrowid");
				if (pyLastRowID == NULL) {
					PyErr_Print();
					return -1;
				}
				if (pyLastRowID == Py_None) {
					self->iLastRowID = -1;
					Py_XDECREF(pyLastRowID);
				}
				else {
					nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(self->pyAutoColumn, PXDYNASETCOLUMN_INDEX));
					tmp = PyTuple_GetItem(pyRowData, nColumn);
					PyTuple_SET_ITEM(pyRowData, nColumn, pyLastRowID);
					self->iLastRowID = PyLong_AsLong(pyLastRowID);
					Py_XDECREF(tmp);
					PxDynaset_UpdateAutoColumnInChildren(self, self->pyAutoColumn, pyLastRowID);
				}
			}

			iRecordsChanged++;
			Py_DECREF(pyCursor);

			if (self->sInsertSQL)
				PyMem_RawFree(self->sInsertSQL);
			self->sInsertSQL = sSql;
			PyMem_RawFree(sSql2);

			Py_XDECREF(self->pyParams);
			self->pyParams = pyParams;
			Py_DECREF(pyRowNew); // Py_True
			//PyStructSequence_SetItem(pyRow, PXDYNASETROW_NEW, Py_False);
		}
		// UPDATE
		else if (pyRowDataOld != Py_None) {
			g_debug("UPDATE");
			sSql = StringAppend(NULL, "UPDATE ");  // allocate on heap
			sSql = StringAppend2(sSql, PyUnicode_AsUTF8(self->pyTable), " SET ");
			sSql2 = StringAppend(NULL, " WHERE ");
			PyObject* pyParams2 = PyList_New(0);
			PyObject* pyParams1 = pyParams;

			while (PyDict_Next(self->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
				if (PyErr_Occurred()) {
					PyErr_Print();
					return -1;
				}
				nColumn = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));
				pyIsKey = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_KEY);
				pyData = PyTuple_GetItem(pyRowData, nColumn);
				if (pyIsKey == Py_False) {
					sSql = StringAppend2(sSql, PyUnicode_AsUTF8(pyColumnName), "=?,");
					if (PyList_Append(pyParams1, pyData) == -1) {
						return -1;
					}
				}
				else if (pyIsKey == Py_True) {
					sSql2 = StringAppend2(sSql2, PyUnicode_AsUTF8(pyColumnName), "=? AND ");
					if (PyList_Append(pyParams2, pyData) == -1) {
						return -1;
					}
				}
			}

			if ((pyParams = PySequence_Concat(pyParams1, pyParams2)) == NULL)
				return -1;
			//XX(pyParams);
			Py_XDECREF(pyParams1);
			Py_XDECREF(pyParams2);

			if (PySequence_Size(pyParams) == 0) {
				PyErr_SetString(PyExc_RuntimeError, "No key columns given. Can not update.");
				return -1;
			}

			memset(sSql + strlen(sSql) - 1, '\0', 1); // cut off final comma
			memset(sSql2 + strlen(sSql2) - 5, '\0', 1); // cut off final ' AND '
			sSql = StringAppend2(sSql, sSql2, ";");

			//XX(pyParams);

			pyCursor = PyObject_CallMethod(self->pyConnection, "execute", "(sO)", sSql, PyList_AsTuple(pyParams));
			PyErr_Print();
			if (pyCursor == NULL) {
				return -1;
			}
			iRecordsChanged++;

			Py_DECREF(pyCursor);

			if (self->sUpdateSQL)
				PyMem_RawFree(self->sUpdateSQL);
			self->sUpdateSQL = sSql;
			PyMem_RawFree(sSql2);

			Py_XDECREF(self->pyParams);
			self->pyParams = pyParams;
			Py_DECREF(pyRowDataOld);
		}
		Py_XDECREF(pyParams);
	}

	// save all descendants
	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyChild = (PxDynasetObject*)PyList_GetItem(self->pyChildren, n);
		iChildRecordsChanged = PxDynaset_Save(pyChild);
		if (iChildRecordsChanged == -1)
			return -1;
		else
			iRecordsChanged += iChildRecordsChanged;
	}

	//g_debug("Saved Dynaset! %d %s", iRecordsChanged, sSql);
	return iRecordsChanged;
}

static bool
// after sucessful commit remove deleted rows, mark all clean and unfreeze record pointer widgets all through the subtree
PxDynaset_CleanUp(PxDynasetObject* self)
{
	PxDynasetObject* pyChild;
	Py_ssize_t n, nLen;

	// iterate over own rows
	PyObject* pyRow, *pyRowDataOld, *pyRowDelete, *pyRowNew, *tmp;
	Py_ssize_t nRow, nColumn, nPos;

	nLen = PySequence_Size(self->pyRows);
	for (nRow = 0; nRow < nLen; nRow++) {
		pyRow = PyList_GetItem(self->pyRows, nRow);
		pyRowDataOld = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATAOLD);
		pyRowDelete = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DELETE);
		pyRowNew = PyStructSequence_GetItem(pyRow, PXDYNASETROW_NEW);

		// DELETE
		if (pyRowDelete == Py_True) {
			if (PyList_SetSlice(self->pyRows, nRow, nRow + 1, NULL) == -1)
				return -1;

			if (nRow <= self->nRow)
				self->nRow--;
			self->nRows--;
			nLen--;
			nRow--; // the row pointed to has just been deleted, the next one will have to have the same index
		}
		// INSERT
		else if (pyRowNew == Py_True) {
			Py_INCREF(Py_False);
			PyStructSequence_SetItem(pyRow, PXDYNASETROW_NEW, Py_False);
		}
		// UPDATE
		else if (pyRowDataOld != Py_None) {
			Py_DECREF(pyRowDataOld);
			Py_INCREF(Py_None);
			PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATAOLD, Py_None);
		}
	}

	self->bClean = true;
	self->bFrozen = false;
	if (self->pyParent == NULL || self->pyParent == Py_None || self->pyParent->bLocked)
		self->bLocked = true;
	else
		self->bLocked = false;

	// clean up all descendants
	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyChild = (PxDynasetObject*)PyList_GetItem(self->pyChildren, n);
		if (!PxDynaset_CleanUp(pyChild))
			return false;
	}

	PxDynaset_RefreshBoundWidgets(self, true, true, true);
	PxDynaset_UpdateControlWidgets(self);
	return true;
}

bool
PxDynaset_NewRow(PxDynasetObject* self, Py_ssize_t nRow)
{
	PyObject* pyColumnName, *pyColumn, *pyRow, *pyRowData, *pyFreshRowData, *pyData, *pyIndex;
	Py_ssize_t nPos = 0, nCol = 0, nAutoCol = -1;

	if (self->pyEmptyRowData == NULL) {
		self->pyEmptyRowData = PyTuple_New(PyDict_Size(self->pyColumns));//PyList_New(0);

		// determine the index# of the auto column, if any
		if (self->pyAutoColumn) {
			pyColumn = PyStructSequence_GetItem(self->pyAutoColumn, PXDYNASETCOLUMN_INDEX);
			if (pyColumn == Py_None) { // query has not been run yet
				nAutoCol = 0;
				PyStructSequence_SetItem(pyColumn, PXDYNASETCOLUMN_INDEX, PyLong_FromSsize_t(0));
			}
			else
				nAutoCol = PyLong_AsSsize_t(pyColumn);
		}

		// construct a list of row data to prepopulate the new row
		while (PyDict_Next(self->pyColumns, &nPos, &pyColumnName, &pyColumn)) {
			if (PyErr_Occurred()) {
				return false;
			}
			nCol = PyLong_AsSsize_t(PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX));

			// if it is an auto column, prepopulate with -1
			if (nCol == nAutoCol)
				pyData = PyLong_FromLong(-1);
			else {
				// if it got a parent column, prepopulate with data of that
				pyData = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_PARENT);
				if (pyData != Py_None && self->pyParent) {
					pyData = PxDynaset_GetData(self->pyParent, self->pyParent->nRow, pyData);
					if (pyData == NULL) {
						return false;
					}
					//Xx("Parent data", pyData);
				}
				else {
					// if it got a default value, use that
					pyData = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_DEFAULT);
					if (pyData == Py_None) {
						// if it got a default function, call that to get a data value
						pyData = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_DEFFUNC);
						if (pyData != Py_None) {
							if (!(pyData = PyObject_CallObject(pyData, NULL)))
								return false;
						}
					}
					Py_INCREF(pyData);
				}
			}
			PyTuple_SET_ITEM(self->pyEmptyRowData, nCol, pyData);
			//PyList_Append(pyRowData, Py_None);
			//pyIndex = PyStructSequence_GetItem(pyColumn, PXDYNASETCOLUMN_INDEX);
			//if (pyIndex == Py_None)
			//	PyStructSequence_SetItem(pyColumn, PXDYNASETCOLUMN_INDEX, PyLong_FromSsize_t(nCol));
			//nCol++;
		}
	}

	pyFreshRowData = PyTuple_Duplicate(self->pyEmptyRowData);
	pyRow = PyStructSequence_New(&PxDynasetRowType);
	Py_INCREF(Py_None);
	Py_INCREF(Py_True);
	Py_INCREF(Py_False);
	PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATA, pyFreshRowData);
	PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATAOLD, Py_None);
	PyStructSequence_SetItem(pyRow, PXDYNASETROW_NEW, Py_True);
	PyStructSequence_SetItem(pyRow, PXDYNASETROW_DELETE, Py_False);

	int iResult;
	if (nRow == -1)
		iResult = PyList_Append(self->pyRows, pyRow);
	else
		iResult = PyList_Insert(self->pyRows, nRow + 1, pyRow);

	if (iResult == -1) {
		//PyErr_SetString(PyExc_RuntimeError, "Can not add row to Dynaset.");
		Py_DECREF(pyFreshRowData);
		return false;
	}
	//Py_DECREF(pyItem);
	self->nRows++;
	if (nRow <= self->nRow)
		self->nRow++;
	if (!PxDynaset_DataChanged(self, -1, NULL))
		return false;
	return true;
}

static PyObject*
PxDynaset_new_row(PxDynasetObject* self, PyObject *args, PyObject *kwds)
{
	g_debug("PxDynaset_new_row");
	static char *kwlist[] = { "row", NULL };
	Py_ssize_t nRow = self->nRow;
	//if (!PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &nRow)) {
	//	goto ERROR;
	//}
	if (nRow == -1)
		nRow = self->nRows - 1;

	if (!PxDynaset_SetRow(self, -1))
		goto ERROR;
	if (!PxDynaset_NewRow(self, nRow))
		goto ERROR;
	if (!PxDynaset_SetRow(self, nRow + 1))
		goto ERROR;

	Py_RETURN_NONE;

ERROR:
	PythonErrorDialog();
	return NULL;
}

bool
PxDynaset_Undo(PxDynasetObject* self, Py_ssize_t nRow)
{
	PyObject* pyRowDataOld;
	PyObject* pyRow = PyList_GetItem(self->pyRows, nRow);

	if ((pyRowDataOld = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATAOLD)) != Py_None) { // old data
        PyObject* pyRowData = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATA);
        Py_DECREF(pyRowData);
        PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATA, pyRowDataOld);
        PyStructSequence_SetItem(pyRow, PXDYNASETROW_DATAOLD, Py_None);
        if (!PxDynaset_DataChanged(self, nRow, NULL))
            return false;
	}
	if (! PxDynaset_Thaw(self))
        return false;
	return PxDynaset_UnStain(self);
}

bool
PxDynaset_DeleteRow(PxDynasetObject* self, Py_ssize_t nRow)
{
	PyObject* pyRow = PyList_GetItem(self->pyRows, nRow);
	PyObject* pyDelete = PyStructSequence_GetItem(pyRow, PXDYNASETROW_DELETE);
	if (pyDelete == Py_True)
		return true;
	Py_DECREF(pyDelete);
	PyStructSequence_SetItem(pyRow, PXDYNASETROW_DELETE, Py_True);
	Py_INCREF(Py_True);
	if (!PxDynaset_DataChanged(self, nRow, NULL))
		return false;
	return PxDynaset_Stain(self);
}

bool
PxDynaset_AddWidget(PxDynasetObject* self, PxWidgetObject *pyWidget)
{
	if (PyList_Append(self->pyWidgets, (PyObject*)pyWidget) == -1) {
		return false;
	}
	//Py_INCREF(pyWidget); not necessary
	return true;
}

bool
PxDynaset_RemoveWidget(PxDynasetObject* self, PxWidgetObject *pyWidget)
{
	if (PySequence_DelItem((PyObject*)self->pyWidgets, PySequence_Index(self->pyWidgets, (PyObject*)pyWidget)) == -1) {
		return false;
	}
	return true;
}


static bool
PxDynaset_Edit(PxDynasetObject* self)
{
	Py_ssize_t n, nLen;
	gboolean bSensitive;
	PxWidgetObject* pyBoundWidget;
	PxDynasetObject* pyChildDynaset;

	self->bLocked = false;

	// iterate over bound widgets and set sensitive
	nLen = PySequence_Size(self->pyWidgets);
	for (n = 0; n < nLen; n++) {
		pyBoundWidget = (PxWidgetObject*)PyList_GetItem(self->pyWidgets, n);
		if (pyBoundWidget->bTable) {
			//
		}
		else if (pyBoundWidget->pyDataColumn) {
			bSensitive = !self->bReadOnly && !pyBoundWidget->bReadOnly /*&& !self->bLocked*/;
			// readonly if widget is bound to auto column unless row is new
			if (self->pyAutoColumn == PyStructSequence_GET_ITEM(pyBoundWidget->pyDataColumn, PXDYNASETCOLUMN_INDEX) &&
				PyStructSequence_GET_ITEM(PyList_GetItem(self->pyRows, self->nRow), PXDYNASETROW_NEW) == Py_False)
				bSensitive = FALSE;
			// readonly if widget is bound to column which has a parent
			if (PyStructSequence_GET_ITEM(pyBoundWidget->pyDataColumn, PXDYNASETCOLUMN_PARENT) != Py_None)
				bSensitive = FALSE;
			gtk_widget_set_sensitive(pyBoundWidget->gtk, bSensitive);
		}
	}

	// unlock children
	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyChildDynaset = (PxDynasetObject*)PyList_GetItem(self->pyChildren, n);
		if (!PxDynaset_Edit(pyChildDynaset))
			return false;
	}
	return PxDynaset_UpdateControlWidgets(self);
}

static PyObject*
PxDynaset_edit(PxDynasetObject* self, PyObject *args)
{
	if (!PxDynaset_Edit(self))
		return NULL;
	Py_RETURN_NONE;
}

static PyObject*
PxDynaset_save(PxDynasetObject* self, PyObject *args)
{
	if (PxDynaset_Save(self) == -1) {
		PythonErrorDialog();
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject*
PxDynaset_undo(PxDynasetObject* self, PyObject *args)
{
	if (self->nRow == -1) {
		PyErr_SetString(PyExc_RuntimeError, "No row selected.");
		return NULL;
	}
	if (PxDynaset_Undo(self, self->nRow))
		Py_RETURN_NONE;
	PythonErrorDialog();
	return NULL;
}

static PyObject*
PxDynaset_delete(PxDynasetObject* self, PyObject *args)
{
	g_debug("PxDynaset_del");
	if (self->nRow == -1) {
		PyErr_SetString(PyExc_RuntimeError, "No row selected.");
		return NULL;
	}
	if (!PxDynaset_DeleteRow(self, self->nRow))
		return NULL;
	Py_RETURN_NONE;
}

bool
PxDynaset_Freeze(PxDynasetObject* self)
{
	Py_ssize_t n, nLen;
	PxWidgetObject* pyWidget;

	self->bFrozen = true;

	// freeze all bound pointer widgets
	nLen = PySequence_Size(self->pyWidgets);
	for (n = 0; n < nLen; n++) {
		pyWidget = (PxWidgetObject*)PyList_GetItem(self->pyWidgets, n);
		if (pyWidget->bPointer)
			gtk_widget_set_sensitive(pyWidget->gtk, false);
	}
	//g_debug("PxDynaset_Froze");

	PxDynaset_UpdateControlWidgets(self);
	if (self->pyParent)
		return PxDynaset_Freeze(self->pyParent);
	else
		return true;
}

bool
PxDynaset_Thaw(PxDynasetObject* self)
{
	Py_ssize_t n, nLen;
	PxDynasetObject* pyChild;
	PxWidgetObject* pyWidget;
	self->bFrozen = false;

	// check if any child is frozen
	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyChild = (PxWidgetObject*)PyList_GetItem(self->pyChildren, n);
		if (pyChild->bFrozen)
			self->bFrozen = true;
	}
	if (self->bFrozen)
		return true;

	// unfreeze all bound pointer widgets
	nLen = PySequence_Size(self->pyWidgets);
	for (n = 0; n < nLen; n++) {
		pyWidget = (PxWidgetObject*)PyList_GetItem(self->pyWidgets, n);
		if (pyWidget->bPointer)
			gtk_widget_set_sensitive(pyWidget->gtk, TRUE);
	}

	PxDynaset_UpdateControlWidgets(self);

	if (self->pyParent)
		return PxDynaset_Thaw(self->pyParent);
	else
		return true;
}

bool
PxDynaset_Stain(PxDynasetObject* self)
{
	self->bClean = false;
	PxDynaset_UpdateControlWidgets(self);
	if (self->pyParent)
		return PxDynaset_Stain(self->pyParent);
	else
		return true;
}

bool
PxDynaset_UnStain(PxDynasetObject* self)
{
	Py_ssize_t n, nLen;
	PxDynasetObject* pyChild;

	self->bClean = true;
	// check if any child is stained
	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyChild = (PxWidgetObject*)PyList_GetItem(self->pyChildren, n);
		if (!pyChild->bClean)
			self->bClean = false;
	}

	PxDynaset_UpdateControlWidgets(self);
	if (self->pyParent)
		return PxDynaset_UnStain(self->pyParent);
	else
		return true;
}

static PyObject*
PxDynaset_ok(PxDynasetObject* self, PyObject *args)
{
	Py_RETURN_NONE;
}

bool
PxDynaset_ParentSelectionChanged(PxDynasetObject* self)
{
	PyObject* pyRes = NULL;
	if (self->pyOnParentSelectionChangedCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		pyRes = PyObject_CallObject(self->pyOnParentSelectionChangedCB, pyArgs);
		Py_XDECREF(pyArgs);
		if (pyRes == NULL)
			return false;
		else {
			Py_XDECREF(pyRes);
		}
	}

	if (self->bAutoExecute) {
		if (self->pyParent->nRow == -1)
			return PxDynaset_Clear(self);
		else
			if ((pyRes = PxDynaset_execute(self, NULL, NULL)) == NULL)
				return false;
			else
				Py_XDECREF(pyRes);
	}
	return true;
}

bool
PxDynaset_DataChanged(PxDynasetObject* self, Py_ssize_t nRow, PyObject* pyColumn)
{
	// iterate over bound widgets, refresh if single data and current row changed, pass on if table
	PyObject* pyResult = NULL;
	PxWidgetObject* pyDependent;

	Py_ssize_t n, nLen = PySequence_Size(self->pyWidgets);
	for (n = 0; n < nLen; n++) {
		pyDependent = (PxWidgetObject*)PyList_GetItem(self->pyWidgets, n);
		if (pyDependent->bTable) {
			if (nRow == -1 || pyColumn == NULL)
				pyResult = PyObject_CallMethod((PyObject*)pyDependent, "refresh", NULL);
			else
				pyResult = PyObject_CallMethod((PyObject*)pyDependent, "refresh_cell", "nO", nRow, pyColumn);
		}
		else if ((self->nRow == nRow && (pyDependent->pyDataColumn == pyColumn || pyColumn == NULL)) || (nRow == -1 && self->nRow != -1))
		{
			pyResult = PyObject_CallMethod((PyObject*)pyDependent, "refresh", NULL);
		}
		else
			continue;

		if (pyResult == NULL)
			return false;
		else
			Py_DECREF(pyResult);
	}
	return true;
}

bool
PxDynaset_SetRow(PxDynasetObject* self, Py_ssize_t nRow)
{
	PxDynasetObject* pyDependent;
	PyObject* pyResult;
	Py_ssize_t n, nLen;

	if (self->nRow == nRow)
		return true;

	if (nRow < -1 || nRow > self->nRows) {
		PyErr_Format(PyExc_IndexError, "Cannot set row in Dynaset '%s'. Row number %d out of range (%d).", PyUnicode_AsUTF8(self->pyTable), nRow, self->nRows);
		return false;
	}
	self->nRow = nRow;

	// notify bound row pointing and non-table widgets
	if (!PxDynaset_RefreshBoundWidgets(self, true, false, true))
		return false;

	// notify child Dynasets
	nLen = PySequence_Size(self->pyChildren);
	for (n = 0; n < nLen; n++) {
		pyDependent = (PxDynasetObject*)PyList_GetItem(self->pyChildren, n);
		if (!PxDynaset_ParentSelectionChanged(pyDependent))
			return false;
	}
	return true;
}

static bool
PxDynaset_RefreshBoundWidgets(PxDynasetObject* self, bool bNonTable, bool bTable, bool bRowPointer)
{
	PxWidgetObject* pyDependent;
	PyObject* pyResult = NULL;
	Py_ssize_t n, nLen;

	nLen = PySequence_Size(self->pyWidgets);
		//Xx("self->pyWidgets ",self->pyWidgets);
	//g_debug("Px nLen %i.", nLen);
	for (n = 0; n < nLen; n++) {
		pyDependent = (PxWidgetObject*)PyList_GetItem(self->pyWidgets, n);

		if ((bNonTable && !pyDependent->bTable) || (bTable && pyDependent->bTable))
			if ((pyResult = PyObject_CallMethod((PyObject*)pyDependent, "refresh", NULL)) == NULL)
				return false;
		if (bRowPointer && pyDependent->bPointer)
			if ((pyResult = PyObject_CallMethod((PyObject*)pyDependent, "refresh_row_pointer", NULL)) == NULL)
				return false;

		if (pyResult) {
		    Py_DECREF(pyResult);
            //Py_DECREF(pyDependent); // for some mysterious reason PyObject_CallMethod increases the reference count
		}
	//g_debug("PxDynaset_RefreshBoundWidgets pointer at %i.", n);
	}
		//Xx("PxDynaset_RefreshBoundWidgets ",pyDependent);
	return true;
}

static bool
PxDynaset_UpdateControlWidgets(PxDynasetObject* self)
{
	bool bDelete = false, bClean = true, bEnable = false;
	if (self->nRow != -1) {
		PyObject* pyRow = PyList_GetItem(self->pyRows, self->nRow);

		//bNew = (PyStructSequence_GetItem(pyRow, PXDYNASETROW_NEW) == Py_True);
		bDelete = (PyStructSequence_GetItem(pyRow, PXDYNASETROW_DELETE) == Py_True);
		bClean = (PyStructSequence_GetItem(pyRow, PXDYNASETROW_DATAOLD) == Py_None);

		if (PyErr_Occurred())
			return false;
	}

	if (self->pyEditButton) {
		bEnable = (!self->bReadOnly && self->bLocked);
		gtk_widget_set_sensitive(self->pyEditButton->gtk, bEnable);
	}

	if (self->pyNewButton) {
		bEnable = !self->bFrozen && !self->bLocked;
		gtk_widget_set_sensitive(self->pyNewButton->gtk, bEnable);
	}

	if (self->pyUndoButton) {
		bEnable = !bClean || bDelete;
		gtk_widget_set_sensitive(self->pyUndoButton->gtk, bEnable);
	}

	if (self->pySaveButton) {
		bEnable = !self->bClean;
		gtk_widget_set_sensitive(self->pySaveButton->gtk, bEnable);
	}

	if (self->pyDeleteButton) {
		bEnable = !bDelete && !self->bFrozen && !self->bLocked && self->nRow != -1;
		gtk_widget_set_sensitive(self->pyDeleteButton->gtk, bEnable);
	}

	if (self->pyDialog && gtk_dialog_get_widget_for_response(self->pyDialog->gtk, GTK_RESPONSE_ACCEPT) != NULL) {
		bEnable = self->nRow != -1;
		gtk_dialog_set_response_sensitive(self->pyDialog->gtk, GTK_RESPONSE_ACCEPT, bEnable);
	}

	return true;
}

static PyMethodDef PxDynaset_ControlButtons[] = {
	{ "new", (PyCFunction)PxDynaset_new_row, METH_VARARGS, "New button pressed" },
	{ "edit", (PyCFunction)PxDynaset_edit, METH_VARARGS, "Edit button pressed" },
	{ "undo", (PyCFunction)PxDynaset_undo, METH_VARARGS, "Undo button pressed" },
	{ "save", (PyCFunction)PxDynaset_save, METH_VARARGS, "Save button pressed" },
	{ "delete", (PyCFunction)PxDynaset_delete, METH_VARARGS, "Delete button pressed" },
	{ "ok", (PyCFunction)PxDynaset_ok, METH_VARARGS, "OK button pressed" },
};

static int
PxDynaset_setattro(PxDynasetObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "autoColumn") == 0) {

			if (!PyObject_TypeCheck(pyValue, &PxDynasetColumnType)) {
				PyErr_SetString(PyExc_TypeError, "'autoColumn' must be a DataColumn.");
				return -1;
			}

			PyObject* pyAttr = PyStructSequence_GetItem(pyValue, PXDYNASETCOLUMN_TYPE);
			if (pyAttr != &PyLong_Type) {
				PyErr_SetString(PyExc_TypeError, "'autoColumn.type' must be 'int'.");
				return -1;
			}

			pyAttr = PyStructSequence_GetItem(pyValue, PXDYNASETCOLUMN_KEY);
			if (pyAttr != Py_True) {
				Py_DECREF(pyAttr);
				PyStructSequence_SetItem(pyValue, PXDYNASETCOLUMN_KEY, Py_True);
				Py_INCREF(Py_True);
			}

			Py_XDECREF(self->pyAutoColumn);
			self->pyAutoColumn = pyValue;
			Py_INCREF(self->pyAutoColumn);
			return 0;
		}

		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "row") == 0) {
			return PxDynaset_SetRow(self, PyLong_AsSsize_t(pyValue)) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonNew") == 0) {
			PxAttachObject(&self->pyNewButton, pyValue, true);
			gtk_button_set_image(self->pyNewButton->gtk, gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR));
			self->pyNewButton->pyOnClickCB = PyCFunction_NewEx(&PxDynaset_ControlButtons[0], (PyObject *)self, NULL);
			if (self->pyNewButton->pyOnClickCB == NULL) {
				Py_DECREF(pyValue);
				return -1;
			}
			PxDynaset_UpdateControlWidgets(self);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonEdit") == 0) {
			PxAttachObject(&self->pyEditButton, pyValue, true);
			gtk_button_set_image(self->pyEditButton->gtk, gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_SMALL_TOOLBAR));
			self->pyEditButton->pyOnClickCB = PyCFunction_NewEx(&PxDynaset_ControlButtons[1], (PyObject*)self, NULL);
			if (self->pyEditButton->pyOnClickCB == NULL) {
				Py_DECREF(pyValue);
				return -1;
			}
			PxDynaset_UpdateControlWidgets(self);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonUndo") == 0) {
			PxAttachObject(&self->pyUndoButton, pyValue, true);
			gtk_button_set_image(self->pyUndoButton->gtk, gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_SMALL_TOOLBAR));
			self->pyUndoButton->pyOnClickCB = PyCFunction_NewEx(&PxDynaset_ControlButtons[2], (PyObject *)self, NULL);
			if (self->pyUndoButton->pyOnClickCB == NULL) {
				Py_DECREF(pyValue);
				return -1;
			}
			PxDynaset_UpdateControlWidgets(self);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonSave") == 0) {
			PxAttachObject(&self->pySaveButton, pyValue, true);
			gtk_button_set_image(self->pySaveButton->gtk, gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_SMALL_TOOLBAR));
			self->pySaveButton->pyOnClickCB = PyCFunction_NewEx(&PxDynaset_ControlButtons[3], (PyObject *)self, NULL);
			if (self->pySaveButton->pyOnClickCB == NULL) {
				Py_DECREF(pyValue);
				return -1;
			}
			PxDynaset_UpdateControlWidgets(self);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonDelete") == 0) {
			PxAttachObject(&self->pyDeleteButton, pyValue, true);
			gtk_button_set_image(self->pyDeleteButton->gtk, gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_SMALL_TOOLBAR));
			self->pyDeleteButton->pyOnClickCB = PyCFunction_NewEx(&PxDynaset_ControlButtons[4], (PyObject *)self, NULL);
			if (self->pyDeleteButton->pyOnClickCB == NULL) {
				Py_DECREF(pyValue);
				return -1;
			}
			PxDynaset_UpdateControlWidgets(self);
			return 0;
		}/*
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonOK") == 0) {
			PxAttachObject(&self->pyOkButton, pyValue, true);
			gtk_button_set_image(self->pyOkButton->gtk, gtk_image_new_from_stock(GTK_STOCK_OK, GTK_ICON_SIZE_SMALL_TOOLBAR));
			self->pyOkButton->pyOnClickCB = PyCFunction_NewEx(&PxDynaset_ControlButtons[5], (PyObject *)self, NULL);
			if (self->pyDeleteButton->pyOnClickCB == NULL) {
				Py_DECREF(pyValue);
				return -1;
			}
			return 0;
		}*/
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_parent_changed") == 0) {
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyOnParentSelectionChangedCB);
				self->pyOnParentSelectionChangedCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assigned object must be callable.");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "before_save") == 0) {
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyBeforeSaveCB);
				self->pyBeforeSaveCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assigned object must be callable.");
				return -1;
			}
		}
	}
	return PyObject_GenericSetAttr((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxDynaset_getattro(PxDynasetObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "parent") == 0) {
			PyErr_Clear();
			if (self->pyParent) {
				Py_INCREF(self->pyParent);
				return (PyObject*)self->pyParent;
			}
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "lastInsertSQL") == 0) {
			PyErr_Clear();
			if (self->sInsertSQL)
				return PyUnicode_FromString(self->sInsertSQL);
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "lastUpdateSQL") == 0) {
			PyErr_Clear();
			if (self->sUpdateSQL)
				return PyUnicode_FromString(self->sUpdateSQL);
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "lastDeleteSQL") == 0) {
			PyErr_Clear();
			if (self->sDeleteSQL)
				return PyUnicode_FromString(self->sDeleteSQL);
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "lastSQLParameters") == 0) {
			PyErr_Clear();
			if (self->pyParams) {
				Py_INCREF(self->pyParams);
				return self->pyParams;
			}
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "autoColumn") == 0) {
			PyErr_Clear();
			if (self->pyAutoColumn) {
				Py_INCREF(self->pyAutoColumn);
				return self->pyAutoColumn;
			}
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "lastRowID") == 0) {
			PyErr_Clear();
			if (self->iLastRowID > -1) {
				return PyLong_FromLong(self->iLastRowID);
			}
			else
				Py_RETURN_NONE;
		}

		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "row") == 0) {
			PyErr_Clear();
			if (self->nRow == -1)
				Py_RETURN_NONE;
			else
				return PyLong_FromSsize_t(self->nRow);
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "buttonSave") == 0) {
			PyErr_Clear();
			if (self->pySaveButton) {
				Py_INCREF(self->pySaveButton);
				return (PyObject *)self->pySaveButton;
			}
			else
				Py_RETURN_NONE;
		}
	}
	return pyResult;
}

static PyObject* // new ref
PxDynaset_str(PxDynasetObject* self)
{
	return PyUnicode_FromFormat("pylax.Dynaset object at %p on table '%.50s'", self, PyUnicode_AsUTF8(self->pyTable));
}

static void
PxDynaset_dealloc(PxDynasetObject* self)
{
	Py_XDECREF(self->pyParent);
	Py_XDECREF(self->pyConnection);
	Py_XDECREF(self->pyTable);
	Py_XDECREF(self->pyCursor);
	Py_XDECREF(self->pyColumns);
	Py_XDECREF(self->pyAutoColumn);
	Py_XDECREF(self->pyRows);
	Py_XDECREF(self->pyChildren);
	Py_XDECREF(self->pyEmptyRowData);
	Py_XDECREF(self->pyQuery);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMemberDef PxDynaset_members[] = {
	{ "rows", T_PYSSIZET, offsetof(PxDynasetObject, nRows), READONLY, "Row count. -1 if still not executed." },
	{ "query", T_OBJECT, offsetof(PxDynasetObject, pyQuery), 0, "Query string" },
	{ "autoExecute", T_BOOL, offsetof(PxDynasetObject, bAutoExecute), 0, "Execute query if parent row has changed." },
	{ "readOnly", T_BOOL, offsetof(PxDynasetObject, bReadOnly), 0, "Data can not be edited." },
	//{ "buttonOK", T_OBJECT, offsetof(PxDynasetObject, pyOkButton), 0, "Close the dialog." },
	{ "buttonSearch", T_OBJECT, offsetof(PxDynasetObject, pySearchButton), 0, "Execute seach." },
	{ NULL }
};

static PyMethodDef PxDynaset_methods[] = {
	{ "add_column", (PyCFunction)PxDynaset_add_column, METH_VARARGS | METH_KEYWORDS, "Add a data column" },
	{ "get_column", (PyCFunction)PxDynaset_get_column, METH_VARARGS, "Returns a data column as named tuple." },
	{ "execute", (PyCFunction)PxDynaset_execute, METH_VARARGS | METH_KEYWORDS, "Run the query" },
	{ "get_row", (PyCFunction)PxDynaset_get_row, METH_VARARGS, "Returns a data row as named tuple." },
	{ "get_data", (PyCFunction)PxDynaset_get_data, METH_VARARGS, "Returns the data for a row/column combination" },
	{ "set_data", (PyCFunction)PxDynaset_set_data, METH_VARARGS, "Sets the data for a row/column combination" },
	{ "get_row_data", (PyCFunction)PxDynaset_get_row_data, METH_VARARGS, "Returns a data row as named tuple." },
	{ "clear", (PyCFunction)PxDynaset_clear, METH_NOARGS, "Empties the data." },
	{ "save", (PyCFunction)PxDynaset_save, METH_NOARGS, "Save the data." },
	{ NULL }
};

PyTypeObject PxDynasetType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Dynaset",           /* tp_name */
	sizeof(PxDynasetObject),   /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxDynaset_dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	PxDynaset_str,             /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	PxDynaset_str,             /* tp_str */
	PxDynaset_getattro,        /* tp_getattro */
	PxDynaset_setattro,        /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"The result of a database query that can be manipulated and updates back", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxDynaset_methods,         /* tp_methods */
	PxDynaset_members,         /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxDynaset_init,  /* tp_init */
	0,                         /* tp_alloc */
	PxDynaset_new,             /* tp_new */
};

PyTypeObject PxDynasetColumnType = { 0, 0, 0, 0, 0, 0 };
PyTypeObject PxDynasetRowType = { 0, 0, 0, 0, 0, 0 };
