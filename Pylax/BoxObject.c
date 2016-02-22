#include "Pylax.h"

static LPWSTR szBoxClass = L"PxBoxClass";

static PyObject *
PxBox_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxBoxObject* self = (PxBoxObject*)PxBoxType.tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxBox_init(PxBoxObject *self, PyObject *args, PyObject *kwds)
{
	if (PxBoxType.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);

	self->hWin = CreateWindowExW(0, szBoxClass, L"",
		WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VISIBLE,
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXBOX, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	return 0;
}

static void
PxBox_dealloc(PxBoxObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxBox_members[] = {
	{ NULL }
};

static PyMethodDef PxBox_methods[] = {
	{ NULL }
};

PyTypeObject PxBoxType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Box",               /* tp_name */
	sizeof(PxBoxObject),       /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxBox_dealloc, /* tp_dealloc */
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
	"Box widget objects",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxBox_methods,             /* tp_methods */
	PxBox_members,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxBox_init,      /* tp_init */
	0,                         /* tp_alloc */
	PxBox_new,                 /* tp_new */
};


static LRESULT CALLBACK PxBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL
PxBoxType_Init()
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = PxBoxProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g.hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szBoxClass;
	wc.hIconSm = NULL;

	if (!RegisterClassEx(&wc))
	{
		PyErr_SetFromWindowsErr(0);
		return FALSE;
	}

	return TRUE;
}

static LRESULT CALLBACK
PxBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND: {
		PxBoxObject* self = (PxBoxObject*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		if (self == NULL)
			break;
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect((HDC)wParam, &rc, self->hBkgBrush);
		return 1L;
	}
	}
	return DefParentProc(hwnd, uMsg, wParam, lParam, FALSE);
}