#include "Pylax.h"

/* UNDER CONSTRUCTION -------------*/

static PyObject *
PxTab_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxTabObject* self = (PxTabObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->iPages = 0;
		self->pyCurrentPage = NULL;
		self->pyPages = PyList_New(0);
		return (PyObject*)self;
	}
	else
		return NULL;
}

static LRESULT CALLBACK PxTabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static int
PxTab_init(PxTabObject *self, PyObject *args, PyObject *kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);

	self->hWin = CreateWindowEx(0, WC_TABCONTROL, L"",
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,// | TCS_BOTTOM,
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, g.hMainMenu, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));
	self->fnOldWinProcedure = (WNDPROC)SetWindowLongPtrW(self->hWin, GWLP_WNDPROC, (LONG_PTR)PxTabProc);

	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	/*if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
		return -1;*/

	return 0;
}

static void
PxTab_dealloc(PxTabObject* self)
{
	DestroyWindow(self->hWin);
	Py_DECREF(self->pyPages);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxTab_members[] = {
	{ NULL }
};

static PyMethodDef PxTab_methods[] = {
	{ NULL }
};

PyTypeObject PxTabType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Tab",               /* tp_name */
	sizeof(PxTabObject),       /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxTab_dealloc, /* tp_dealloc */
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
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Tab widget objects",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxTab_methods,             /* tp_methods */
	PxTab_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxTab_init,      /* tp_init */
	0,                         /* tp_alloc */
	PxTab_new,                 /* tp_new */
};

static BOOL PxTabPage_GotSelected(PxTabPageObject* self);

static LRESULT CALLBACK
PxTabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PxTabObject* self = (PxTabObject*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case OCM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGING:
			return FALSE;

		case TCN_SELCHANGE:
		{
			int iIndex = TabCtrl_GetCurSel(self->hWin);
			PxTabPage_GotSelected(PyList_GetItem(self->pyPages, iIndex));
			break;
		}
		}

	}
	}
	return CallWindowProcW(self->fnOldWinProcedure, hwnd, msg, wParam, lParam);
}

/* TabPage ----------------------------------------------------------------------- */

static PyObject *
PxTabPage_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxTabPageObject* self = (PxTabPageObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->iIndex = -1;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxTabPage_init(PxTabPageObject *self, PyObject *args, PyObject *kwds)
{
	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;

	//OxRect rc = { .iLeft = 2, .iTop = 30, .iWidth = -5, .iHeight = -5 };

	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	self->rc.left = 2;
	self->rc.top = 30;
	self->rc.right = -5;
	self->rc.bottom = -5;
	PxWidget_MoveWindow(self);

	if (gArgs.pyCaption != NULL) {
		LPCSTR strText = PyUnicode_AsUTF8(gArgs.pyCaption);
		tie.pszText = toW(strText);
	}
	else
		tie.pszText = L"New";

	PxWidgetObject* pyParent = (PxWidgetObject*)GetWindowLongPtr(gArgs.hwndParent, GWLP_USERDATA);
	if (!PyObject_TypeCheck(pyParent, &PxTabType)) {
		PyErr_SetString(PyExc_TypeError, "'parent' must be a Tab");
		return -1;
	}

	self->iIndex = TabCtrl_InsertItem(pyParent->hWin, ((PxTabObject*)pyParent)->iPages++, &tie);
	if (self->iIndex == -1) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	if (PyList_Append(((PxTabObject*)pyParent)->pyPages, self) == -1)
		return -1;

	if (((PxTabObject*)pyParent)->iPages == 1)
		PxTabPage_GotSelected(self);
        else
		ShowWindow(self->hWin, SW_HIDE);        

	return 0;
}

static BOOL
PxTabPage_GotSelected(PxTabPageObject* self)
{
	PxTabObject* pyTab = (PxTabObject*)self->pyParent;
	if (pyTab->pyCurrentPage)
		ShowWindow(pyTab->pyCurrentPage->hWin, SW_HIDE);
	pyTab->pyCurrentPage = self;
	ShowWindow(self->hWin, SW_SHOW);
	return TRUE;
}

static void
PxTabPage_dealloc(PxTabPageObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PxWidgetObject *)self);
}

static PyMemberDef PxTabPage_members[] = {
	{ NULL }
};

static PyMethodDef PxTabPage_methods[] = {
	{ NULL }
};

PyTypeObject PxTabPageType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.TabPage",           /* tp_name */
	sizeof(PxTabPageObject),   /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxTabPage_dealloc, /* tp_dealloc */
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
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"TabPage widget objects",  /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxTabPage_methods,         /* tp_methods */
	PxTabPage_members,         /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxTabPage_init,  /* tp_init */
	0,                         /* tp_alloc */
	PxTabPage_new,             /* tp_new */
};
