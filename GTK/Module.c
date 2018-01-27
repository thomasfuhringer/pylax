// Module.c  | Pylax © 2017 by Thomas Führinger
#include "Pylax.h"

// in main.c
PyObject* Pylax_message(PyObject* self, PyObject* args);
PyObject* Pylax_ask(PyObject *self, PyObject *args, PyObject *kwds);
PyObject* Pylax_status_message(PyObject* self, PyObject* args);
PyObject* Pylax_set_icon(PyObject *self, PyObject *args);
PyObject* Pylax_append_menu_item(PyObject *self, PyObject *args);
PyObject* Pylax_set_before_close(PyObject *self, PyObject *args);


static PyMethodDef PylaxMethods[] = {
	{ "message", Pylax_message, METH_VARARGS, "Show message box." },
	{ "status_message", Pylax_status_message, METH_VARARGS, "Show message in the status bar." },
	{ "append_menu_item", Pylax_append_menu_item, METH_VARARGS, "Add an item to menu 'App'." },
	/*{ "ask", Pylax_ask, METH_VARARGS | METH_KEYWORDS, "Show message box." },
	{ "set_before_close", Pylax_set_before_close, METH_VARARGS, "Set before close callback." },*/
	{ NULL, NULL, 0, NULL }
};

static PyModuleDef pylaxmodule = {
	PyModuleDef_HEAD_INIT,
	"pylax",
	"Pylax module",
	-1,
	PylaxMethods
};

PyMODINIT_FUNC
PyInit_pylax(void)
{
	PyObject* pyModule;

	pyModule = PyModule_Create(&pylaxmodule);
	if (pyModule == NULL)
		return NULL;

	if (PyType_Ready(&PxDynasetType) < 0)
		return NULL;

	if (PyType_Ready(&PxMenuType) < 0)
		return NULL;

	if (PyType_Ready(&PxMenuItemType) < 0)
		return NULL;

	if (PyType_Ready(&PxWidgetType) < 0)
		return NULL;

	PxWindowType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxWindowType) < 0)
		return NULL;

	PxFormType.tp_base = &PxWindowType;
	if (PyType_Ready(&PxFormType) < 0)
		return NULL;
        
	PxLabelType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxLabelType) < 0)
		return NULL;

	PxButtonType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxButtonType) < 0)
		return NULL;

	PxEntryType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxEntryType) < 0)
		return NULL;

	PxComboBoxType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxComboBoxType) < 0)
		return NULL;

	PxCanvasType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxCanvasType) < 0)
		return NULL;

	PxImageType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxImageType) < 0)
		return NULL;

	PxBoxType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxBoxType) < 0)
		return NULL;

	PxSplitterType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxSplitterType) < 0)
		return NULL;

	PxTabType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxTabType) < 0)
		return NULL;

	PxTabPageType.tp_base = &PxBoxType;
	if (PyType_Ready(&PxTabPageType) < 0)
		return NULL;

	PxTableType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxTableType) < 0)
		return NULL;

	if (PyType_Ready(&PxTableColumnType) < 0)
		return NULL;

	if (!PxDynasetTypes_Init())
		return NULL;

	if (!PxImageType_Init())
		return NULL;

	Py_INCREF(&PxDynasetType);
	Py_INCREF(&PxImageType);
	Py_INCREF(&PxMenuType);
	Py_INCREF(&PxMenuItemType);
	Py_INCREF(&PxWidgetType);
	Py_INCREF(&PxWindowType);
	Py_INCREF(&PxFormType);
	Py_INCREF(&PxLabelType);
	Py_INCREF(&PxButtonType);
	Py_INCREF(&PxEntryType);
	Py_INCREF(&PxComboBoxType);
	Py_INCREF(&PxCanvasType);
	Py_INCREF(&PxBoxType);
	Py_INCREF(&PxSplitterType);
	Py_INCREF(&PxTabType);
	Py_INCREF(&PxTabPageType);
	Py_INCREF(&PxTableType);
	Py_INCREF(&PxTableColumnType);

	PyModule_AddObject(pyModule, "Dynaset", (PyObject *)&PxDynasetType);
	PyModule_AddObject(pyModule, "Image", (PyObject *)&PxImageType);
	PyModule_AddObject(pyModule, "Menu", (PyObject *)&PxMenuType);
	PyModule_AddObject(pyModule, "MenuItem", (PyObject *)&PxMenuItemType);
	PyModule_AddObject(pyModule, "Widget", (PyObject *)&PxWidgetType);
	PyModule_AddObject(pyModule, "Window", (PyObject *)&PxWindowType);
	PyModule_AddObject(pyModule, "Form", (PyObject *)&PxFormType);
	PyModule_AddObject(pyModule, "Label", (PyObject *)&PxLabelType);
	PyModule_AddObject(pyModule, "Button", (PyObject *)&PxButtonType);
	PyModule_AddObject(pyModule, "Entry", (PyObject *)&PxEntryType);
	PyModule_AddObject(pyModule, "ComboBox", (PyObject *)&PxComboBoxType);
	PyModule_AddObject(pyModule, "Canvas", (PyObject *)&PxCanvasType);
	PyModule_AddObject(pyModule, "Box", (PyObject *)&PxBoxType);
	PyModule_AddObject(pyModule, "Splitter", (PyObject *)&PxSplitterType);
	PyModule_AddObject(pyModule, "Tab", (PyObject *)&PxTabType);
	PyModule_AddObject(pyModule, "TabPage", (PyObject *)&PxTabPageType);
	PyModule_AddObject(pyModule, "Table", (PyObject *)&PxTableType);
	PyModule_AddObject(pyModule, "TableColumn", (PyObject *)&PxTableColumnType);

	if (PyDict_SetItemString(PxWidgetType.tp_dict, "defaultCoordinate", PyLong_FromLong(PxDEFAULT)) == -1)
		return NULL;
	if (PyDict_SetItemString(PxWidgetType.tp_dict, "center", PyLong_FromLong(PxWIDGET_CENTER)) == -1)
		return NULL;

	// named tuples
	PyModule_AddObject(pyModule, "DynasetColumn", (PyObject*)&PxDynasetColumnType);
	PyModule_AddObject(pyModule, "DynasetRow", (PyObject*)&PxDynasetRowType);

	// enumerations
	PyObject* pyArgs = Py_BuildValue("(ss)", "Align", "left right center top bottom block");
	g.pyAlignEnum = PyObject_CallObject(g.pyEnumType, pyArgs);
	PyModule_AddObject(pyModule, "Align", g.pyAlignEnum);
	Py_DECREF(pyArgs);

	pyArgs = Py_BuildValue("(ss)", "ImageFormat", "ico bmp jpg");
	g.pyImageFormatEnum = PyObject_CallObject(g.pyEnumType, pyArgs);
	PyModule_AddObject(pyModule, "ImageFormat", g.pyImageFormatEnum);
	Py_DECREF(pyArgs);

	// other
	PyObject* pyDict = PyModule_GetDict(pyModule);
	char* sCR = COPYRIGHT;
	PyObject* pyValue = PyUnicode_FromString(sCR);
	if (pyValue) {
		PyDict_SetItemString(pyDict, "copyright", pyValue);
		Py_DECREF(pyValue);
	}

	pyValue = Py_BuildValue("(iiis)", VER_MAJOR, VER_MINOR, VER_MICRO, VER_RELEASE_LEVEL);
	if (pyValue) {
		PyDict_SetItemString(pyDict, "version_info", pyValue);
		Py_DECREF(pyValue);
	}

	g.pyPylaxModule = pyModule;
	return pyModule;
}
