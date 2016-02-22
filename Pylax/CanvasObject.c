#include "Pylax.h"

static	PAINTSTRUCT ps;

static PyObject *
PxCanvas_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxCanvasObject* self = (PxCanvasObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->pyOnPaintCB = NULL;
		self->hDC = 0;
		self->hPen = 0;
		self->hBrush = 0;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxCanvas_init(PxCanvasObject *self, PyObject *args, PyObject *kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);
	self->hWin = CreateWindowEx(0, L"PxCanvasClass", L"",
		WS_CHILD | WS_VISIBLE,
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXCANVAS, g.hInstance, NULL);

	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	if (gArgs.pyCaption != NULL)
		if (!PxWidget_SetCaption((PxWidgetObject*)self, gArgs.pyCaption))
			return -1;
	SendMessage(self->hWin, WM_SETFONT, (WPARAM)g.hfDefaultFont, MAKELPARAM(FALSE, 0));

	return 0;
}

static PyObject*
PxCanvas_point(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;
	RECT rcClient;

	if (self->hDC == 0) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "ii",
		&iX,
		&iY))
		return NULL;

	GetClientRect(self->hWin, &rcClient);
	if (iX < 0)
		iX += rcClient.right;
	if (iY < 0)
		iY += rcClient.bottom;

	COLORREF c = SetPixel(self->hDC, iX, iY, RGB(1, 1, 1));
	if (c == 1) {
		PyErr_SetFromWindowsErr(0);
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_move_to(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;
	RECT rcClient;

	if (self->hDC == 0) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "ii",
		&iX,
		&iY))
		return NULL;

	GetClientRect(self->hWin, &rcClient);
	if (iX < 0)
		iX += rcClient.right;
	if (iY < 0)
		iY += rcClient.bottom;

	if (MoveToEx(self->hDC, iX, iY, (LPPOINT)NULL) == 0) {
		PyErr_SetFromWindowsErr(0);
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_line_to(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;
	RECT rcClient;

	if (self->hDC == 0) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "ii",
		&iX,
		&iY))
		return NULL;

	GetClientRect(self->hWin, &rcClient);
	if (iX < 0)
		iX += rcClient.right;
	if (iY < 0)
		iY += rcClient.bottom;

	if (LineTo(self->hDC, iX, iY) == 0) {
		PyErr_SetFromWindowsErr(0);
		return NULL;
	}
	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_rectangle(PxCanvasObject* self, PyObject *args)
{
	RECT rcRel, rcAbs;
	int iRadius = -1;
	BOOL bResult;

	if (self->hDC == 0) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "iiii|i",
		&rcRel.left,
		&rcRel.top,
		&rcRel.right,
		&rcRel.bottom,
		&iRadius))
		return NULL;

	TransformRectToAbs(&rcRel, self->hWin, &rcAbs);

	if (iRadius == -1)
		bResult = Rectangle(self->hDC, rcAbs.left, rcAbs.top, rcAbs.right, rcAbs.bottom);
	else
		bResult = RoundRect(self->hDC, rcAbs.left, rcAbs.top, rcAbs.right, rcAbs.bottom, iRadius, iRadius);

	if (bResult)
		Py_RETURN_NONE;
	else {
		PyErr_SetFromWindowsErr(0);
		return NULL;
	}
}

static PyObject*
PxCanvas_text(PxCanvasObject* self, PyObject *args)
{
	int iX, iY;
	const LPCSTR strText;
	LPWSTR szText;
	RECT rcClient;

	if (self->hDC == 0) {
		PyErr_SetString(PyExc_RuntimeError, "This method works only inside an 'on_paint' callback.");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "iis",
		&iX,
		&iY,
		&strText))
		return NULL;

	GetClientRect(self->hWin, &rcClient);
	if (iX < 0)
		iX += rcClient.right;
	if (iY < 0)
		iY += rcClient.bottom;

	szText = toW(strText);
	if (TextOut(self->hDC, iX, iY, szText, _tcslen(szText)) == 0) {
		PyErr_SetFromWindowsErr(0);
		return NULL;
	}
	PyMem_RawFree(szText);
	Py_RETURN_NONE;
}

static PyObject*
PxCanvas_repaint(PxCanvasObject* self)
{
	InvalidateRect(self->hWin, NULL, TRUE);
	Py_RETURN_NONE;
}

static int
PxCanvas_setattro(PxCanvasObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_paint") == 0) 	{
			if (PyCallable_Check(pyValue)) {
				Py_XINCREF(pyValue);
				Py_XDECREF(self->pyOnPaintCB);
				self->pyOnPaintCB = pyValue;
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "textColor") == 0) 	{
			if (PyTuple_Check(pyValue)) {
				if (self->hDC == 0) {
					PyErr_SetString(PyExc_RuntimeError, "textColor can only be assigned inside an 'on_paint' callback.");
					return -1;
				}
				int iR, iG, iB;
				PyObject* pyColorComponent = PyTuple_GetItem(pyValue, 0);
				if (pyColorComponent == NULL)
					return -1;
				iR = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 1);
				if (pyColorComponent == NULL)
					return -1;
				iG = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 2);
				if (pyColorComponent == NULL)
					return -1;
				iB = PyLong_AsLong(pyColorComponent);

				SetTextColor(self->hDC, RGB(iR, iG, iB));
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "penColor") == 0) 	{
			if (PyTuple_Check(pyValue)) {
				if (self->hDC == 0) {
					PyErr_SetString(PyExc_RuntimeError, "penColor can only be assigned inside an 'on_paint' callback.");
					return -1;
				}
				int iR, iG, iB;
				PyObject* pyColorComponent = PyTuple_GetItem(pyValue, 0);
				if (pyColorComponent == NULL)
					return -1;
				iR = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 1);
				if (pyColorComponent == NULL)
					return -1;
				iG = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 2);
				if (pyColorComponent == NULL)
					return -1;
				iB = PyLong_AsLong(pyColorComponent);

				if (self->hPen)
					DeleteObject(self->hPen);
				self->hPen = CreatePen(PS_SOLID, 1, RGB(iR, iG, iB));
				SelectObject(self->hDC, self->hPen);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "fillColor") == 0) 	{
			if (PyTuple_Check(pyValue)) {
				if (self->hDC == 0) {
					PyErr_SetString(PyExc_RuntimeError, "fillColor can only be assigned inside an 'on_paint' callback.");
					return -1;
				}
				int iR, iG, iB;
				PyObject* pyColorComponent = PyTuple_GetItem(pyValue, 0);
				if (pyColorComponent == NULL)
					return -1;
				iR = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 1);
				if (pyColorComponent == NULL)
					return -1;
				iG = PyLong_AsLong(pyColorComponent);
				pyColorComponent = PyTuple_GetItem(pyValue, 2);
				if (pyColorComponent == NULL)
					return -1;
				iB = PyLong_AsLong(pyColorComponent);

				if (self->hBrush)
					DeleteObject(self->hBrush);
				self->hBrush = CreateSolidBrush(RGB(iR, iG, iB));
				SelectObject(self->hDC, self->hBrush);
				return 0;
			}
			else {
				PyErr_SetString(PyExc_TypeError, "Parameter must be tuple of RGB values");
				return -1;
			}
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject *
PxCanvas_getattro(PxCanvasObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject*)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "on_click") == 0) 	{
			PyErr_Clear();
			return self->pyOnPaintCB;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxCanvas_dealloc(PxCanvasObject* self)
{
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxCanvas_members[] = {
	//{ "title", T_OBJECT_EX, offsetof(PxWindowObject, title), 0, "window title" },
	{ NULL }  /* Sentinel */
};

static PyMethodDef PxCanvas_methods[] = {
	{ "point", (PyCFunction)PxCanvas_point, METH_VARARGS, "Draws a point." },
	{ "move_to", (PyCFunction)PxCanvas_move_to, METH_VARARGS, "Moves to a point." },
	{ "line_to", (PyCFunction)PxCanvas_line_to, METH_VARARGS, "Draws a line." },
	{ "rectangle", (PyCFunction)PxCanvas_rectangle, METH_VARARGS, "Draws a rectangle." },
	{ "text", (PyCFunction)PxCanvas_text, METH_VARARGS, "Draws a rectangle." },
	{ "repaint", (PyCFunction)PxCanvas_repaint, METH_NOARGS, "Trigger the paint process." },
	{ NULL }
};

PyTypeObject PxCanvasType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Canvas",            /* tp_name */
	sizeof(PxCanvasObject),    /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxCanvas_dealloc, /* tp_dealloc */
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
	PxCanvas_getattro,         /* tp_getattro */
	PxCanvas_setattro,         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Canvas, a widget for direct painting", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxCanvas_methods,           /* tp_methods */
	PxCanvas_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxCanvas_init,   /* tp_init */
	0,                         /* tp_alloc */
	PxCanvas_new,              /* tp_new */
};


LRESULT CALLBACK PxCanvasWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL PxCanvasType_Init()
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = PxCanvasWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g.hInstance;
	wc.hIcon = g.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"PxCanvasClass";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		PyErr_SetFromWindowsErr(0);
		return FALSE;
	}
	return TRUE;
}

static
LRESULT CALLBACK PxCanvasWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{

	case WM_PAINT:
	{
		PxCanvasObject* self = (PxCanvasObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (self->pyOnPaintCB) {
			self->hDC = BeginPaint(self->hWin, &ps);
			SelectObject(self->hDC, g.hfDefaultFont);
			SetBkMode(self->hDC, TRANSPARENT);
			PyObject* pyArgs = PyTuple_Pack(1, (PyObject*)self);
			Py_INCREF(self);
			PyObject* pyResult = PyObject_CallObject(self->pyOnPaintCB, pyArgs);
			//ReleaseDC (self->hWin, self->hDC); 
			EndPaint(self->hWin, &ps);
			self->hDC = 0;
			if (pyResult == NULL) {
				PyErr_Print();
				MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
			}
			else {
				Py_DECREF(pyResult);
			}
		}
		return 0;
	}

	case WM_SIZE:
		return 0;

	case WM_QUERYENDSESSION:
	case WM_CLOSE:
		OutputDebugString(L"Window Closing!\n");
		if (TRUE)
			//DestroyWindow(hwnd);
			break;
		else
			return 0;

	case WM_DESTROY:
		OutputDebugString(L"Window Destroyed!\n");
		PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (pyWidget != NULL)
			Py_DECREF(pyWidget);
		return 0;
	}
	return DefFrameProc(hwnd, g.hMDIClientArea, msg, wParam, lParam);
}
