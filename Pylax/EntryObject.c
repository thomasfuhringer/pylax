// EntryObject.c  | Pylax © 2015 by Thomas Führinger
#include "Pylax.h"

#define PxENTRY_BUTTON_WIDTH  18

static PyObject*
PxEntry_new(PyTypeObject* type, PyObject *args, PyObject *kwds)
{
	PxEntryObject* self = (PxEntryObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		//self->bGotFocus = FALSE;
		self->pyOnClickButtonCB = NULL;
		self->bButtonDown = FALSE;
		self->bButtonMouseDown = FALSE;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxEntry_init(PxEntryObject *self, PyObject *args, PyObject *kwds)
{
	if (PxEntryType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);

	int iAlignment = (self->pyDataType == &PyUnicode_Type) ? 0 : ES_RIGHT;

	self->hWin = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
		WS_CHILD | WS_TABSTOP | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | iAlignment | (gArgs.bVisible ? WS_VISIBLE : 0), //| ES_MULTILINE
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXENTRY, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	self->fnOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxEntryProc);
	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));
	if (self->pyDataSet)
		//SendMessage(self->hWin, EM_SETREADONLY, TRUE, 0);
		EnableWindow(self->hWin, FALSE);
	if (self->pyParent == NULL)
		ShowWindow(self->hWin, SW_HIDE);

	return 0;
}

BOOL
PxEntry_RenderData(PxEntryObject* self, BOOL bFormat)
{
	if (self->pyData == NULL || self->pyData == Py_None) {
		SendMessage(self->hWin, WM_SETTEXT, 0, (LPARAM)L"");
		return TRUE;
	}

	PyObject* pyText = PxFormatData(self->pyData, bFormat ? self->pyFormat : (self->pyFormatEdit ? self->pyFormatEdit : Py_None));
	if (pyText == NULL) {
		return FALSE;
	}

	LPCSTR strText = PyUnicode_AsUTF8(pyText);
	LPWSTR szText = toW(strText);
	Py_DECREF(pyText);
	if (SendMessage(self->hWin, WM_SETTEXT, 0, (LPARAM)szText)) {
		PyMem_RawFree(szText);
		return TRUE;
	}
	else {
		PyMem_RawFree(szText);
		PyErr_SetFromWindowsErr(0);
		return FALSE;
	}
}

BOOL
PxEntry_SetData(PxEntryObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return TRUE;
	OutputDebugString(L"\n*---- PxEntry_SetData");

	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return FALSE;

	return PxEntry_RenderData(self, TRUE);
}

static PyObject*  // new ref
PxEntry_refresh(PxEntryObject* self)
{
	OutputDebugString(L"\n*---- PxEntry_refresh");
	if (self->pyDataSet == NULL)
		Py_RETURN_TRUE;

	if (self->pyDataSet->nRow == -1) {
		if (!PxEntry_SetData(self, Py_None))
			return NULL;
		//SendMessage(self->hWin, EM_SETREADONLY, TRUE, 0);
		EnableWindow(self->hWin, FALSE);
	}
	else {
		PyObject *pyData = PxWidget_PullData((PxWidgetObject*)self);
		if (pyData && !PxEntry_SetData(self, pyData))
			return NULL;
		//SendMessage(self->hWin, EM_SETREADONLY, self->bReadOnly || self->pyDataSet->bLocked, 0);
		EnableWindow(self->hWin, !(self->bReadOnly || self->pyDataSet->bLocked));
	}
	Py_RETURN_TRUE;
}

BOOL
PxEntry_RepresentCell(PxEntryObject* self)
{
	RECT rcSubItem;
	PxTableObject* pyTable = ((PxTableColumnObject*)self->pyParent)->pyTable;
	ListView_GetSubItemRect(pyTable->hWin, (int)self->pyDataSet->nRow, ((PxTableColumnObject*)self->pyParent)->iIndex + 1, LVIR_BOUNDS, &rcSubItem);
	MoveWindow(self->hWin, rcSubItem.left, rcSubItem.top, rcSubItem.right - rcSubItem.left, rcSubItem.bottom - rcSubItem.top, TRUE);
	PyObject* pyResult = PxEntry_refresh(self);
	if (pyResult == NULL)
		return FALSE;
	else
		Py_DECREF(pyResult);
	ShowWindow(self->hWin, SW_SHOW);
	return TRUE;
	/*
		self->hWin = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
		WS_CHILD | WS_VISIBLE | ES_WANTRETURN,
		rcSubItem.left, rcSubItem.top, rcSubItem.right - rcSubItem.left, rcSubItem.bottom - rcSubItem.top, self->pyParent->pyTable->hWin, 0, GetModuleHandle(NULL), NULL);
		if (self->hWin == NULL) {
		MessageBox(hwnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
		}
		SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));
		SetFocus(self->hWin);
		SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
		self->pOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxEntryProc);
		*/
}

static PyObject*  // new ref
PxEntry_get_input_string(PxEntryObject* self)
{
	TCHAR szText[1024];
	PyObject* pyInput;
	GetWindowText(self->hWin, szText, 1024);
	char* strText = toU8(szText);
	if (lstrlenW(szText) > 0)
		pyInput = PyUnicode_FromString(strText);
	else
		pyInput = PyUnicode_New(0, 0);
	PyMem_RawFree(strText);
	return pyInput;
}

static PyObject*  // new ref
PxEntry_get_input_data(PxEntryObject* self)
{
	TCHAR szText[1024];
	PyObject* pyData;
	GetWindowText(self->hWin, szText, 1024);
	char* strText = toU8(szText);
	if ((pyData = PxParseString(strText, self->pyDataType, NULL)) == NULL)
		return NULL;
	PyMem_RawFree(strText);
	return pyData;
}

BOOL
PxEntry_Entering(PxEntryObject* self)
{
	if (self->pyWindow->pyFocusWidget == (PxWidgetObject*)self)
		return TRUE;

	int iR = PxWindow_MoveFocus(self->pyWindow, (PxWidgetObject*)self);
	if (iR == -1)
		return FALSE;
	else if (iR == FALSE)
		return TRUE;

	return PxEntry_RenderData(self, FALSE);
}
/*
BOOL
PxEntry_Changed(PxEntryObject* self)
{
return TRUE;
//MessageBox(NULL, L"Changed!", L"Error", MB_ICONERROR);
if (!self->bDirty && self->pyDataSet)
PxDataSet_Freeze(self->pyDataSet);
self->bDirty = TRUE;
return TRUE;// MessageBox(NULL, L"Changed!", L"Error", MB_ICONERROR);
}*/

static PyObject*  // new ref
PxEntry_render_focus(PxEntryObject* self)
{
	PyObject* pyData = NULL, *pyResult = NULL;

	if (self->pyVerifyCB) {
		PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
		pyResult = PyObject_CallObject(self->pyVerifyCB, pyArgs);
		Py_DECREF(pyArgs);
		if (pyResult == NULL)
			return NULL;
		else if (pyResult == Py_False)
			SetFocus(self->hWin);
	}

	if (pyResult != Py_False) {
		pyData = PxEntry_get_input_data(self);
		if (pyData == NULL)
			return NULL;
		if (PyObject_RichCompareBool(self->pyData, pyData, Py_EQ)) {
			Py_DECREF(pyData);
			pyData = NULL;
		}
	}

	if (pyData && !PxEntry_SetData(self, pyData))
		return NULL;
	if (self->pyParent && PyObject_TypeCheck(self->pyParent, &PxTableColumnType))
		ShowWindow(self->hWin, SW_HIDE);

	if (pyResult != Py_True && pyResult != Py_False) {
		Py_XDECREF(pyResult);
		pyResult = Py_True;
	}

	OutputDebugString(L"\n*---- PxEntry_render_focus");
	return pyResult;
}

static int
PxEntry_setattro(PxEntryObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			//MessageBox(NULL, L"Got da!", L"Error", MB_ICONERROR);
			OutputDebugString(L"\n-data =\n");
			//ShowInts(L"X", 0, pyValue->ob_refcnt);
			//XX(pyValue);
			if (!PxEntry_SetData((PxEntryObject*)self, pyValue))
				return -1;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "alignHoriz") == 0) {
			LONG_PTR pAlign;
			switch (PyLong_AsLong(PyObject_GetAttrString(pyValue, "value")))
			{
			case ALIGN_LEFT:
				pAlign = ES_LEFT;
				break;
			case ALIGN_RIGHT:
				pAlign = ES_RIGHT;
				break;
			case ALIGN_CENTER:
				pAlign = ES_CENTER;
				break;
			}
			SetWindowLongPtr(self->hWin, GWL_STYLE, GetWindowLongPtr(self->hWin, GWL_STYLE) & ~(ES_CENTER | ES_RIGHT) | pAlign);

			self->pyAlignHorizontal = pyValue;
			return 0;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click_button") == 0) 	{
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyOnClickButtonCB);
				self->pyOnClickButtonCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assign a callable!");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "input") == 0) {
			if (PyUnicode_Check(pyValue)) {
				LPCSTR strText = PyUnicode_AsUTF8(pyValue);
				LPWSTR szText = toW(strText);
				SendMessage(self->hWin, WM_SETTEXT, 0, (LPARAM)szText);
				PyMem_RawFree(szText);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Assign a str!");
				return -1;
			}
		}
	}
	return PxEntryType.tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject* // new ref
PxEntry_getattro(PxEntryObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			Py_INCREF(self->pyData);
			return self->pyData;
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "inputString") == 0) {
			PyErr_Clear();
			return PxEntry_get_input_string(self);
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "inputData") == 0) {
			PyErr_Clear();
			return PxEntry_get_input_data(self);
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxEntry_dealloc(PxEntryObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxEntry_members[] = {
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxEntry_methods[] = {
	{ "refresh", (PyCFunction)PxEntry_refresh, METH_NOARGS, "Pull fresh data" },
	{ "render_focus", (PyCFunction)PxEntry_render_focus, METH_NOARGS, "Return True if ready for focus to move on." },
	{ NULL }  /* Sentinel */
};

PyTypeObject PxEntryType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Entry",            /* tp_name */
	sizeof(PxEntryObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxEntry_dealloc, /* tp_dealloc */
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
	PxEntry_getattro,          /* tp_getattro */
	PxEntry_setattro,          /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Data entry field objects", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxEntry_methods,           /* tp_methods */
	PxEntry_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxEntry_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxEntry_new,               /* tp_new */
};

void PxEntry_DrawInsertedButton(PxEntryObject* self, RECT* pRc)
{
	if (self->pyWindow->pyFocusWidget != (PxWidgetObject*)self)
		return;

	HDC hDC = GetWindowDC(self->hWin);

	if (self->bButtonDown == TRUE)
	{
		DrawEdge(hDC, pRc, EDGE_RAISED, BF_RECT | BF_FLAT | BF_ADJUST);
		FillRect(hDC, pRc, GetSysColorBrush(COLOR_BTNFACE));
		OffsetRect(pRc, 1, 1);
		SetBkMode(hDC, TRANSPARENT);
		DrawTextW(hDC, L"...", 3, pRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		DrawEdge(hDC, pRc, EDGE_RAISED, BF_RECT | BF_ADJUST);
		FillRect(hDC, pRc, GetSysColorBrush(COLOR_BTNFACE));
		SetBkMode(hDC, TRANSPARENT);
		DrawTextW(hDC, L"...", 3, pRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	ReleaseDC(self->hWin, hDC);
}

void PxEntry_GetButtonRect(PxEntryObject* self, RECT* pRc)
{
	pRc->right -= self->cxRightEdge;
	pRc->top += self->cyTopEdge;
	pRc->bottom -= self->cyBottomEdge;
	pRc->left = pRc->right - PxENTRY_BUTTON_WIDTH;

	if (self->cxRightEdge > self->cxLeftEdge)
		OffsetRect(pRc, self->cxRightEdge - self->cxLeftEdge, 0);
}

LRESULT CALLBACK PxEntryProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT    rc, rcOld;
	RECT   *pRc;
	POINT   pt;
	BOOL	bOldstate;
	PxEntryObject* self = (PxEntryObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	//PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);

	switch (msg)
	{
	case WM_NCCALCSIZE:
		if (self->pyOnClickButtonCB == NULL) {
			pRc = (RECT*)lParam;
			rcOld = *pRc;

			CallWindowProc(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
			if (self->pyWindow->pyFocusWidget != (PxWidgetObject*)self)
				return 0;

			self->cxLeftEdge = pRc->left - rcOld.left;
			self->cxRightEdge = rcOld.right - pRc->right;
			self->cyTopEdge = pRc->top - rcOld.top;
			self->cyBottomEdge = rcOld.bottom - pRc->bottom;
			pRc->right -= PxENTRY_BUTTON_WIDTH;
			return 0;
		}
		else
			break;

	case WM_NCPAINT:
		if (self->pyOnClickButtonCB) {
			CallWindowProcW(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
			if (self->pyWindow->pyFocusWidget != (PxWidgetObject*)self)
				return 0;

			GetWindowRect(self->hWin, &rc);
			OffsetRect(&rc, -rc.left, -rc.top);
			PxEntry_GetButtonRect(self, &rc);
			PxEntry_DrawInsertedButton(self, &rc);
			return 0;
		}
		else
			break;

	case WM_NCHITTEST:
		if (self->pyOnClickButtonCB) {
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);

			GetWindowRect(hwnd, &rc);
			PxEntry_GetButtonRect(self, &rc);

			if (PtInRect(&rc, pt)) {
				return HTBORDER;
			}
		}
		break;

	case WM_NCLBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
		if (self->pyOnClickButtonCB == NULL || self->pyWindow->pyFocusWidget != (PxWidgetObject*)self)
			break;
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);

		GetWindowRect(hwnd, &rc);
		pt.x -= rc.left;
		pt.y -= rc.top;
		OffsetRect(&rc, -rc.left, -rc.top);
		PxEntry_GetButtonRect(self, &rc);

		if (PtInRect(&rc, pt)) {
			SetCapture(hwnd);
			self->bButtonDown = TRUE;
			self->bButtonMouseDown = TRUE;
			PxEntry_DrawInsertedButton(self, &rc);
		}
		break;

	case WM_MOUSEMOVE:
		if (self->pyOnClickButtonCB == NULL || self->bButtonMouseDown == FALSE)
			break;

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		ClientToScreen(hwnd, &pt);
		GetWindowRect(hwnd, &rc);

		pt.x -= rc.left;
		pt.y -= rc.top;
		OffsetRect(&rc, -rc.left, -rc.top);

		PxEntry_GetButtonRect(self, &rc);
		bOldstate = self->bButtonDown;

		if (PtInRect(&rc, pt))
			self->bButtonDown = 1;
		else
			self->bButtonDown = 0;

		if (bOldstate != self->bButtonDown)
			PxEntry_DrawInsertedButton(self, &rc);

		break;

	case WM_LBUTTONUP:
		if (self->pyOnClickButtonCB == NULL || self->bButtonMouseDown != TRUE)
			break;

		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		ClientToScreen(hwnd, &pt);

		GetWindowRect(hwnd, &rc);

		pt.x -= rc.left;
		pt.y -= rc.top;
		OffsetRect(&rc, -rc.left, -rc.top);

		PxEntry_GetButtonRect(self, &rc);

		if (PtInRect(&rc, pt) && self->pyWindow->pyFocusWidget != (PxWidgetObject*)self && self->pyOnClickButtonCB) {
			PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
			Py_INCREF(self);
			PyObject* pyResult = PyObject_CallObject(self->pyOnClickButtonCB, pyArgs);
			if (pyResult == NULL)
				ShowPythonError();
			else
				Py_DECREF(pyResult);
		}

		ReleaseCapture();
		self->bButtonDown = FALSE;
		self->bButtonMouseDown = FALSE;

		PxEntry_DrawInsertedButton(self, &rc);

		break;
		/*
			case WM_KILLFOCUS:
			if (self->pyParent && PyObject_TypeCheck(self->pyParent, &PxTableColumnType)) {
			LV_DISPINFO lvDispinfo;
			ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = 0;//iItem;
			lvDispinfo.item.iSubItem = ((PxTableColumnObject*)self->pyParent)->iIndex;
			lvDispinfo.item.pszText = NULL;
			char szEditText[100];
			GetWindowText(self->hWin, szEditText, 100);
			lvDispinfo.item.pszText = szEditText;
			lvDispinfo.item.cchTextMax = lstrlen(szEditText);
			SendMessage(self->pyWindow->hWin, WM_NOTIFY, (WPARAM)self->pyParent->pyParent->hWin, (LPARAM)&lvDispinfo); //the LV ID and the LVs Parent window's HWND
			DestroyWindow(self->hWin);
			break;
			} */

	case OCM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_SETFOCUS:
			OutputDebugString(L"\n*---- Entr focus");
			if (!PxEntry_Entering(self))//(PxEntryObject*)pyWidget)) 
				//XX(self);
				ShowPythonError();
			return 0;

			/*case EN_CHANGE:
				if (!PxEntry_Changed((PxEntryObject*)pyWidget)) {
				PyErr_Print();
				MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
				}
				return 0;

				case EN_KILLFOCUS:
				if (!PxEntry_Left((PxEntryObject*)pyWidget)) {
				PyErr_Print();
				MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
				}
				return 0;*/
		}
		break;

	default:
		break;
	}
	return CallWindowProc(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
}