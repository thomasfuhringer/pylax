// WindowObject.c  | Pylax © 2016 by Thomas Führinger
#include "Pylax.h"
#include "Resource.h"

static PyObject *
PxWindow_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxWindowObject* self = (PxWindowObject*)PxWindowType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->cxMin = 320;
		self->cyMin = 240;
		self->bTable = TRUE;
		self->pyIcon = NULL;
		self->bNameInCaption = TRUE;
		self->bOkPressed = FALSE;
		self->bModal = FALSE;
		self->pyCancelButton = NULL;
		self->pyOkButton = NULL;
		//self->hLastFocus = 0;
		self->pyFocusWidget = NULL;
		self->pyBeforeCloseCB = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxWindow_init(PxWindowObject *self, PyObject *args, PyObject *kwds)
{
	//OutputDebugString(L"\n-Window init-\n");
	if (PxWindowType.tp_base->tp_init((PyObject*)self, args, kwds) < 0)
		return -1;
	self->pyWindow = self;

	self->pyConnection = g.pyConnection;
	Py_INCREF(self->pyConnection);
	self->pyName = gArgs.pyCaption;
	Py_INCREF(self->pyName);

	if (!PxWindow_RestoreState((PxWindowObject*)self))
		return -1;

	// rest of initialization only if not subclass
	if (!PyObject_TypeCheck((PyObject *)self, &PxWindowType))
		return 0;

	RECT rect;
	TransformRectToAbs(&self->rc, 0, &rect);
	//if (rect.left != CW_USEDEFAULT)
	//	ClientToScreen(gArgs.hwndParent, (POINT*)(&rect.left)); // convert top-left

	self->hWin = CreateWindowExW(WS_EX_CONTROLPARENT, L"PxWindowClass", L"<new>",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_POPUP | (gArgs.bVisible ? WS_VISIBLE : 0),
		rect.left, rect.top, rect.right, rect.bottom, //  , CW_USEDEFAULT, CW_USEDEFAULT
		0, 0, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	//SetWindowLong(self->hWin, GWL_EXSTYLE, GetWindowLong(self->hWin, GWL_EXSTYLE) | WS_EX_LAYERED);

	return 0;
}

BOOL
PxWindow_SetCaption(PxWindowObject* self, PyObject* pyText)
{
	if (self->bNameInCaption) {
		if (pyText == Py_None || PyUnicode_GetLength(pyText) == 0) {
			pyText = self->pyName;
			Py_INCREF(pyText);
		}
		else {
			PyObject* pyNameTuple = Py_BuildValue("OsO", self->pyName, " - ", pyText);
			pyText = PyUnicode_Join(NULL, pyNameTuple);
			Py_DECREF(pyNameTuple);
		}
	}
	else
		Py_INCREF(pyText);

	BOOL bRes = PxWidget_SetCaption((PxWidgetObject*)self, pyText);
	Py_DECREF(pyText);
	return bRes;
}

static PyObject*
PxWindow_run(PxWindowObject* self)
{
	if (PyObject_TypeCheck((PyObject *)self, &PxFormType)) {
		PyErr_SetString(PyExc_TypeError, "A Form can not be run modal.");
		return NULL;
	}

	BOOL r = ShowWindow(self->hWin, SW_SHOW);
	//BringWindowToTop(self->hWin);
	EnableWindow(g.hWin, FALSE);
	/*if (GetLastError() != 0) {
		PyErr_SetFromWindowsErr(0);
		return NULL;
		}*/
	self->bModal = TRUE;
	self->bOkPressed = FALSE;
	MSG msg;
	HACCEL hAccelTable = LoadAccelerators(g.hInstance, MAKEINTRESOURCE(IDS_APP));
	while (GetMessage(&msg, NULL, 0, 0) > 0 && self->bModal)
	{
		if (!IsDialogMessage(self->hWin, &msg) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	EnableWindow(g.hWin, TRUE);
	ShowWindow(self->hWin, SW_HIDE);
	if (self->bOkPressed)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}
/*
PyObject* // new ref
PxWindow_set_focus(PxWindowObject* self, PyObject *args, PyObject *kwds)
{
static char *kwlist[] = { "widget", NULL };
PyObject* pyWidget = NULL;
if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &pyWidget)) {
return NULL;
}
if (!PyObject_TypeCheck(pyWidget, &PxEntryType)) {
PyErr_Format(PyExc_TypeError, "Parameter 1 ('widget') must be an Entry, not '%.200s'.", pyWidget->ob_type->tp_name);
return NULL;
}

SetFocus(((PxWidgetObject*)pyWidget)->hWin);
Py_RETURN_NONE;
}*/

BOOL
PxWindow_SaveState(PxWindowObject* self)
{
	if (g.bConnectionHasPxTables) {
		RECT rcWin;
		PyObject* pyCursor = NULL, *pyResult = NULL, *pyParameters = NULL;
		GetWindowRect(self->hWin, &rcWin);
		if (GetParent(self->hWin) == g.hMDIClientArea)
			MapWindowPoints(HWND_DESKTOP, g.hMDIClientArea, (LPPOINT)&rcWin, 2);

		if ((pyCursor = PyObject_CallMethod(g.pyConnection, "cursor", NULL)) == NULL) {
			return FALSE;
		}
		pyParameters = PyTuple_Pack(1, self->pyName);
		if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "SELECT PxWindowID FROM PxWindow WHERE PxWindowID=?;", pyParameters)) == NULL) {
			return FALSE;
		}
		Py_DECREF(pyParameters);
		if ((pyResult = PyObject_CallMethod(pyCursor, "fetchone", NULL)) == NULL) {
			return FALSE;
		}
		BOOL bEntryExists = pyResult == Py_None ? FALSE : TRUE;
		Py_DECREF(pyResult);

		pyParameters = Py_BuildValue("(iiiiiO)", rcWin.left, rcWin.top, rcWin.right - rcWin.left, rcWin.bottom - rcWin.top, g.iCurrentUser, self->pyName);
		if (bEntryExists) {
			if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "UPDATE PxWindow SET Left=?,Top=?,Width=?,Height=?,ModUser=? WHERE PxWindowID=?;", pyParameters)) == NULL)
				return FALSE;
		}
		else {
			if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "INSERT INTO PxWindow (Left,Top,Width,Height,ModUser,PxWindowID) VALUES (?,?,?,?,?,?);", pyParameters)) == NULL)
				return FALSE;
		}
		Py_DECREF(pyResult);
		if ((pyResult = PyObject_CallMethod(g.pyConnection, "commit", NULL)) == NULL) {
			return FALSE;
		}
		if ((pyCursor && PyObject_CallMethod(pyCursor, "close", NULL)) == NULL) {
			return FALSE;
		}
		Py_DECREF(pyParameters);
		Py_DECREF(pyResult);
		Py_DECREF(pyCursor);
	}
	return TRUE;
}

BOOL
PxWindow_RestoreState(PxWindowObject* self)
{
	if (g.bConnectionHasPxTables) {
		PyObject* pyCursor = NULL, *pyResult = NULL, *pyParameters = NULL, *pyValue = NULL;

		if ((pyCursor = PyObject_CallMethod(g.pyConnection, "cursor", NULL)) == NULL) {
			return FALSE;
		}
		pyParameters = PyTuple_Pack(1, self->pyName);
		if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(sO)", "SELECT Left,Top,Width,Height FROM PxWindow WHERE PxWindowID=?;", pyParameters)) == NULL) {
			return FALSE;
		}
		Py_DECREF(pyResult);
		if ((pyResult = PyObject_CallMethod(pyCursor, "fetchone", NULL)) == NULL) {
			return FALSE;
		}

		if (pyResult != Py_None) {
			pyValue = PyTuple_GetItem(pyResult, 0);
			self->rc.left = PyLong_AsLong(pyValue);
			pyValue = PyTuple_GetItem(pyResult, 1);
			self->rc.top = PyLong_AsLong(pyValue);
			pyValue = PyTuple_GetItem(pyResult, 2);
			self->rc.right = PyLong_AsLong(pyValue);
			pyValue = PyTuple_GetItem(pyResult, 3);
			self->rc.bottom = PyLong_AsLong(pyValue);
		}
		if ((pyCursor && PyObject_CallMethod(pyCursor, "close", NULL)) == NULL) {
			return FALSE;
		}

		Py_DECREF(pyParameters);
		Py_DECREF(pyResult);
		Py_DECREF(pyCursor);
	}
	return TRUE;
}

int PxWindow_MoveFocus(PxWindowObject* self, PxWidgetObject* pyDestinationWidget)
// return TRUE if possible, FALSE if not, -1 on error
{
	if (self->pyFocusWidget == pyDestinationWidget)
		return TRUE;
	if (self->pyFocusWidget == NULL) {
		self->pyFocusWidget = pyDestinationWidget;
		return TRUE;
	}

	PyObject* pyResult = PyObject_CallMethod((PyObject*)self->pyFocusWidget, "render_focus", NULL);
	if (pyResult == NULL)
		return -1;
	BOOL bR = pyResult == Py_True ? TRUE : FALSE;
	Py_DECREF(pyResult);
	OutputDebugString(bR ? L"\n*---- tr" : L"\n*---- fa");
	if (bR)
		self->pyFocusWidget = pyDestinationWidget;
	return bR;
}

typedef struct _PxWindow_Timer
{
	HWND hWin;
	PyObject* pyCallback;
}
PxWindow_Timer;

static VOID CALLBACK
PxWindow_TimerRoutine(PVOID lParam, BOOLEAN TimerOrWaitFired)
{
	if (lParam == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "Timer fired without callback.");
	}
	else {
		PxWindow_Timer* pTimer = (PxWindow_Timer*)lParam;
		SendMessage(pTimer->hWin, WM_APP_TIMER, (WPARAM)pTimer->pyCallback, (LPARAM)0);
	}
}

PyObject*
PxWindow_create_timer(PxWindowObject *self, PyObject *args, PyObject *kwds)
{
	HANDLE hTimer = NULL;
	int iInterval = 0, iWait = 0;
	PyObject *pyCallback = NULL;
	static char *kwlist[] = { "callback", "interval", "wait", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|ii", kwlist,
		&pyCallback,
		&iInterval,
		&iWait))
		return NULL;

	if (!PyCallable_Check(pyCallback)) {
		PyErr_SetString(PyExc_TypeError, "Parameter 2 ('callback') must be callable.");
		return NULL;
	}

	Py_INCREF(pyCallback);
	PxWindow_Timer* pTimer = (PxWindow_Timer*)PyMem_RawMalloc(sizeof(PxWindow_Timer));
	pTimer->hWin = self->hWin;
	pTimer->pyCallback = pyCallback;
	if (!CreateTimerQueueTimer(&hTimer, NULL, (WAITORTIMERCALLBACK)PxWindow_TimerRoutine, (PVOID)pTimer, iWait, iInterval, WT_EXECUTEDEFAULT)) {
		PyErr_SetFromWindowsErr(0);
		ShowPythonError();
		return NULL;
	}

	return PyLong_FromLong(hTimer);
}

PyObject*
PxWindow_delete_timer(PxWindowObject *self, PyObject *args, PyObject *kwds)
{
	HANDLE hTimer = NULL;
	static char *kwlist[] = { "timer", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist,
		&hTimer))
		return NULL;

	if (DeleteTimerQueueTimer(NULL, hTimer, NULL) == 0) {
		PyErr_SetFromWindowsErr(0);
		ShowPythonError();
		return NULL;
	}

	/*if (!CloseHandle(hTimer)) {
		PyErr_SetFromWindowsErr(0);
		ShowPythonError();
		return NULL;
		}*/

	Py_RETURN_NONE;
	// Memory leak. PxWindow_Timer never gets released
}

static int
PxWindow_setattro(PxWindowObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "menu") == 0 && PyObject_TypeCheck((PyObject *)self, &PxWindowType)) {
			if (!PyObject_TypeCheck(pyValue, &PxMenuType)) {
				PyErr_SetString(PyExc_TypeError, "Assigned value is not a Menu.");
				return -1;
			}
			if (SetMenu(self->hWin, ((PxMenuObject*)pyValue)->hWin) == 0)
				return -1;
			DrawMenuBar(self->hWin);
			Py_INCREF(pyValue);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "caption") == 0) 	{
			return PxWindow_SetCaption(self, pyValue) ? 0 : -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			// assigning a key dict here should make the form jump to the record if it is bound to a DataSet
			PyErr_SetString(PyExc_NotImplementedError, "Searching still not available.");
			return -1;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "icon") == 0) {
			if (!PyObject_TypeCheck(pyValue, &PxImageType) || ((PxImageObject*)pyValue)->pyImageFormat != PyObject_GetAttrString(g.pyImageFormatEnum, "ico")) {
				PyErr_SetString(PyExc_TypeError, "'icon' must be a Icon.");
				return -1;
			}
			Py_XDECREF(self->pyIcon);
			Py_INCREF(pyValue);
			self->pyIcon = (PxImageObject*)pyValue;
			SendMessageW(self->hWin, WM_SETICON, ICON_SMALL, (LPARAM)self->pyIcon->hWin);
			SendMessageW(self->hWin, WM_SETICON, ICON_BIG, (LPARAM)self->pyIcon->hWin);
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "before_close") == 0) 	{
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyBeforeCloseCB);
				self->pyBeforeCloseCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
				return -1;
			}
		}
	}
	return PxWindowType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxWindow_getattro(PxWindowObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			if (self->pyDataSet != NULL && self->pyDataSet->nRow > -1)
				//return PxDataSet_GetKeyDict(self->pyDataSet, self->pyDataSet->nRow);
				return PxDataSet_GetRowDataDict(self->pyDataSet, self->pyDataSet->nRow, FALSE);
			else
				Py_RETURN_NONE;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "icon") == 0) {
			PyErr_Clear();
			Py_INCREF(self->pyIcon);
			return (PyObject*)self->pyIcon;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "before_close") == 0) 	{
			PyErr_Clear();
			Py_INCREF(self->pyBeforeCloseCB);
			return self->pyBeforeCloseCB;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "focus") == 0) {
			PyErr_Clear();
			/*HWND hWindow = 0;
			if (GetForegroundWindow() == self->hWin)
			hWindow = GetFocus(self->hWin);
			else
			hWindow = self->hLastFocus;
			if (hWindow > 0) {
			PyObject* pyFocusWidget = (PyObject*)GetWindowLongPtr(hWindow, GWLP_USERDATA);
			if (pyFocusWidget)
			return pyFocusWidget;
			}*/
			if (self->pyFocusWidget) {
				Py_INCREF(self->pyFocusWidget);
				return (PyObject*)self->pyFocusWidget;
			}
			Py_RETURN_NONE;
		}
	}
	return PxWindowType.tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxWindow_dealloc(PxWindowObject* self)
{
	OutputDebugString(L"\n- PxWindow_dealloc -\n");
	Py_XDECREF(self->pyConnection);
	Py_XDECREF(self->pyName);
	Py_XDECREF(self->pyIcon);
	Py_XDECREF(self->pyBeforeCloseCB);
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxWindow_members[] = {
	{ "cnx", T_OBJECT, offsetof(PxWindowObject, pyConnection), READONLY, "Database connection" },
	{ "minWidth", T_INT, offsetof(PxWindowObject, cxMin), 0, "Window can not be resized smaller" },
	{ "minHeight", T_INT, offsetof(PxWindowObject, cyMin), 0, "Window can not be resized smaller" },
	{ "nameInCaption", T_BOOL, offsetof(PxWindowObject, bNameInCaption), 0, "Show name of the Window in the caption" },
	{ "buttonCancel", T_OBJECT, offsetof(PxWindowObject, pyCancelButton), 0, "Close the dialog" },
	{ "buttonOK", T_OBJECT, offsetof(PxWindowObject, pyOkButton), 0, "Close the dialog" },
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxWindow_methods[] = {
	{ "run", (PyCFunction)PxWindow_run, METH_NOARGS, "Run as dialog (modal)" },
	{ "create_timer", (PyCFunction)PxWindow_create_timer, METH_VARARGS | METH_KEYWORDS, "Create a new timer." },
	{ "delete_timer", (PyCFunction)PxWindow_delete_timer, METH_VARARGS | METH_KEYWORDS, "Delete a timer." },
	//{ "set_focus", (PyCFunction)PxWindow_set_focus, METH_VARARGS | METH_KEYWORDS, "Set the focus to given widget." },
	{ NULL }  /* Sentinel */
};

PyTypeObject PxWindowType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Window",            /* tp_name */
	sizeof(PxWindowObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxWindow_dealloc, /* tp_dealloc */
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
	PxWindow_getattro,         /* tp_getattro */
	PxWindow_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Simply a window",         /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxWindow_methods,          /* tp_methods */
	PxWindow_members,          /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxWindow_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxWindow_new,              /* tp_new */
};

static LRESULT CALLBACK
PxWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL
WindowClass_Init()
{
	PxFormType.tp_base = &PxWidgetType;
	if (PyType_Ready(&PxFormType) < 0)
		return FALSE;

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = PxWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g.hInstance;
	wc.hIcon = g.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(PxWINDOWBKGCOLOR + 1); //  (HBRUSH)(COLOR_BTNFACE + 1);g.hBkgBrush;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"PxWindowClass";
	wc.hIconSm = g.hIcon;

	if (!RegisterClassEx(&wc)) {
		ShowLastError(L"PxWindowClass Registration Failed!");
		return FALSE;
	}
	return TRUE;
}


LRESULT DefParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bMDI);

static LPMINMAXINFO lpMMI;

static LRESULT CALLBACK
PxWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PxWindowObject* self = NULL;
	PyObject* pyResult = NULL;
	self = (PxWindowObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
	int iR;

	switch (uMsg)
	{
		/*case WM_CREATE:

			//OutputDebugString(L"Window Created!\n");
			break;*/

	case WM_GETMINMAXINFO:
		if (self) {
			lpMMI = (LPMINMAXINFO)lParam;
			lpMMI->ptMinTrackSize.x = self->cxMin;
			lpMMI->ptMinTrackSize.y = self->cyMin;
		}

		/*
	case WM_MDIACTIVATE:
	return 0;

	case WM_COMMAND:
	return ProcessChildMessages(hwnd, uMsg, wParam, lParam);
	*/

		/*
		case WM_CTLCOLORSTATIC:
		if (pyWidget && PyObject_TypeCheck((PyObject*)pyWidget, &PxLabelType)) {
		COLORREF color = ((PxLabelObject*)pyWidget)->textColor;
		HDC hDC = (HDC)wParam;
		SetBkMode(hDC, TRANSPARENT);
		if (color) {
		SetTextColor(hDC, color);
		}
		return g.hBkgBrush;
		//break;
		}
		return 0;
		*/
		/*
		case WM_NOTIFY:
		switch (LOWORD(wParam)) // control
		{
		case IDC_PXTABLE:
		return PxTable_Notify((PxWidgetObject*)GetWindowLongPtr(((LPNMHDR)lParam)->hwndFrom, GWLP_USERDATA), (LPNMHDR)lParam);
		}
		return 0;*/

	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			EnumChildWindows(hwnd, PxWidgetSizeEnumProc, 0);
		}
		break;

		/*case WM_ACTIVATE:
			if (wParam != WA_INACTIVE) {
			self->pyFocusWidget = (PxWidgetObject*)GetWindowLongPtr((HWND)GetFocus(), GWLP_USERDATA);
			}
			break;
			*/

	case WM_SETFOCUS:
		if (self->pyFocusWidget) {
			SetFocus(self->pyFocusWidget->hWin);
			//PostMessage(self->pyFocusWidget->hWin, WM_SETFOCUS, (WPARAM)0, (LPARAM)0);
		}
		break;
		/*
			case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_RETURN:
			if (self->pyOnReturnKeyCB) {
			PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
			Py_INCREF(self);
			PyObject* pyResult = PyObject_CallObject(self->pyOnReturnKeyCB, pyArgs);
			if (pyResult == NULL) {
			PyErr_Print();
			MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
			}
			Py_XDECREF(pyResult);
			return 0;
			}
			}
			break;*/

	case WM_APP_TIMER:
	{
		pyResult = PyObject_CallObject((PyObject*)wParam, NULL);
		if (pyResult == NULL)
			ShowPythonError();
		else
			Py_DECREF(pyResult);
		return 0;
	}

	case WM_QUERYENDSESSION:
	case WM_CLOSE:
		iR = -1;
		if (self->pyBeforeCloseCB) {
			PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
			PyObject* r = PyObject_CallObject(self->pyBeforeCloseCB, pyArgs);
			if (r == NULL){
				ShowPythonError();
				return 0;
			}
			Py_XDECREF(pyArgs);
			int iR = (r == Py_True ? 1 : (r == Py_False ? 0 : -1)); // False: don't close, True: close immediately, other: close cleanly
			Py_XDECREF(r);
		}

		if (iR == 0)
			return 0;
		else if (iR == -1) {
			if (self->pyDataSet && (self->pyDataSet->bDirty || self->pyDataSet->bFrozen)) {
				int iSave = MessageBox(self->hWin, L"Save changes?", L"Pending changes", MB_YESNOCANCEL | MB_ICONQUESTION);
				if (iSave == IDCANCEL)
					return 0;
				else if (iSave == IDYES)
					if (PxDataSet_Save(self->pyDataSet) == -1)
						return 0;
			}
		}

		PxWindow_SaveState((PxWidgetObject*)self);
		if (self && ((PxWindowObject*)self)->bModal) {
			((PxWindowObject*)self)->bModal = FALSE;
			return 0;
		}
		break;

	case WM_DESTROY:
		OutputDebugString(L"Window Destroying!\n");
		//PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (pyWidget != NULL)
			Py_DECREF(pyWidget);
		return 0;
	}

	return DefParentProc(hwnd, uMsg, wParam, lParam, (BOOL)(self && PyObject_TypeCheck((PyObject*)self, &PxFormType)));
	/*
	if (self && PyObject_TypeCheck((PyObject*)self, &PxFormType))
	return DefMDIChildProc(hwnd, msg, wParam, lParam);
	else
	return DefWindowProc(hwnd, msg, wParam, lParam);*/
}


// Reflect messages to child
LRESULT
DefParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bMDI)
{
	switch (uMsg) {
	case WM_NOTIFY:
	{
		NMHDR* nmhdr = (NMHDR*)lParam;
		if (nmhdr->hwndFrom != NULL)
			return SendMessage(nmhdr->hwndFrom, uMsg + OCM__BASE, wParam, lParam);
		break;
	}

	// Control's HWND in LPARAM
	case WM_COMMAND:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
	case WM_VKEYTOITEM:
	case WM_CHARTOITEM:
		if (lParam != 0)
			return SendMessage((HWND)lParam, uMsg + OCM__BASE, wParam, lParam);
		break;

		/*/ Control's HWND in WPARAM:
		case WM_DRAWITEM:
		case WM_MEASUREITEM:
		case WM_DELETEITEM:
		case WM_COMPAREITEM:
		if(wParam != 0) {
		HWND hwndControl = GetDlgItem(hwnd, wParam);
		if(hwndControl)
		return SendMessage(hwndControl, uMsg + OCM__BASE, wParam, lParam);
		}
		break;
		*/
	}

	if (bMDI)
		return DefMDIChildProcW(hwnd, uMsg, wParam, lParam);
	else
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
