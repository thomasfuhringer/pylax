#include "Pylax.h"
#include "Resource.h"

static HBITMAP hPlaceholderBitmap;
static BITMAP bitmap;

static PyObject *
PxImageView_new(PyTypeObject* type, PyObject *args, PyObject *kwds)
{
	PxImageViewObject* self = (PxImageViewObject*)type->tp_base->tp_new(type, args, kwds);
	if (self != NULL) {
		self->bStretch = TRUE;
		self->bFill = FALSE;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxImageView_init(PxImageViewObject* self, PyObject *args, PyObject *kwds)
{
	if (Py_TYPE(self)->tp_base->tp_init((PyObject *)self, args, kwds) < 0)
		return -1;

	RECT rect;
	TransformRectToAbs(&self->rc, gArgs.hwndParent, &rect);

	self->hWin = CreateWindowExW(0, L"PxImageViewClass", L"",
		WS_CHILD | WS_VISIBLE,
		rect.left, rect.top, rect.right, rect.bottom,
		gArgs.hwndParent, (HMENU)IDC_PXIMAGEVIEW, GetModuleHandle(NULL), NULL);
	if (self->hWin == NULL) {
		PyErr_SetFromWindowsErr(0);
		return -1;
	}

	self->pyDataType = &PxImageType;
	Py_INCREF(self->pyDataType);

	SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	return 0;
}

static BOOL
PxImageView_OpenFileDialog(PxImageViewObject* self)
{
	OPENFILENAME ofn;
	wchar_t szFilter[] = L"Bitmap (*.BMP)\0*.bmp\0";
	wchar_t szOpenFileNamePath[MAX_PATH];

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g.hWin;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = szOpenFileNamePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL; //L"Select File"
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; //OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = L"bmp";
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	OutputDebugString(L"PxImageView_OpenFileDialog\n");
	if (GetOpenFileNameW(&ofn)) {
		OutputDebugString(L"iv4!\n");
		LPCSTR sOpenFileNamePath = toU8(szOpenFileNamePath);
		MessageBoxA(NULL, sOpenFileNamePath, "jj", NULL);
		PyObject* pyArgs = Py_BuildValue("s", sOpenFileNamePath);
		PyObject* pyImage;
		PxASSIGN(pyImage = PyObject_CallObject((PyObject*)&PxImageType, pyArgs));
		XX(pyImage);
		Py_DECREF(pyArgs);
		PyMem_RawFree(sOpenFileNamePath);
		PxImageView_SetData(self, pyImage);
		Py_DECREF(pyImage);
	}
	return TRUE;
}

/*
BOOL
PxImageView_Leave(PxImageViewObject* self)
{
self->pyData =

int iSucess = GetDIBits(
_In_     HDC hdc,
_In_     HBITMAP hbmp,
_In_     UINT uStartScan,
_In_     UINT cScanLines,
_Out_    LPVOID lpvBits,
_Inout_  LPBITMAPINFO lpbi,
_In_     UINT uUsage
);

if (iSucess == 0 || iSucess == ERROR_INVALID_PARAMETER)
return FALSE;


return TRUE;
}

BOOL
PxImageView_SetData(PxImageViewObject* self, PyObject* pyData)
{
//http://stackoverflow.com/questions/2886831/win32-c-c-load-image-from-memory-buffer
hBitmap = CreateBitmap
}

static HBITMAP
PyBytes_AsWinBitmap(PyObject* pyByte, int iFormat)
{
if (iFormat = IMAGEFORMAT_BMP) {
char* pBuffer = PyBytes_AsString(pyByte);
BITMAPFILEHEADER bfh = *(BITMAPFILEHEADER*)pBuffer;
BITMAPINFOHEADER bih = *(BITMAPINFOHEADER*)(pBuffer + sizeof(BITMAPFILEHEADER));
RGBQUAD          rgb = *(RGBQUAD*)(pBuffer + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

BITMAPINFO bi;
bi.bmiColors[0] = rgb;
bi.bmiHeader = bih;

char* pPixels = (pBuffer + bfh.bfOffBits);
char* ppvBits;

HBITMAP hBitmap = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void**)&ppvBits, NULL, 0);
SetDIBits(NULL, hBitmap, 0, bih.biHeight, pPixels, &bi, DIB_RGB_COLORS);
return hBitmap;

//GetObject(hBitmap, sizeof(BITMAP), &cBitmap);
}
else {
PyErr_SetString(PyExc_RuntimeError, "Unsupported format");
return 0;
}
}
*/

BOOL
PxImageView_RenderData(PxEntryObject* self, BOOL bFormat)
{
	InvalidateRect(self->hWin, NULL, TRUE);
	return TRUE;
}

BOOL
PxImageView_SetData(PxImageViewObject* self, PyObject* pyData)
{
	if (self->pyData == pyData)
		return TRUE;
	if (!PxWidget_SetData((PxWidgetObject*)self, pyData))
		return FALSE;

	return PxImageView_RenderData(self, TRUE);
}

static int
PxImageView_setattro(PxImageViewObject* self, PyObject* pyAttributeName, PyObject *pyValue)
{
	if (PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			if (!PxImageView_SetData(self, pyValue))
				return -1;
			return 0;
		}
	}
	return Py_TYPE(self)->tp_base->tp_setattro((PyObject*)self, pyAttributeName, pyValue);
}

static PyObject*
PxImageView_getattro(PxImageViewObject* self, PyObject* pyAttributeName)
{
	PyObject* pyResult;
	pyResult = PyObject_GenericGetAttr((PyObject *)self, pyAttributeName);
	if (pyResult == NULL && PyErr_ExceptionMatches(PyExc_AttributeError) && PyUnicode_Check(pyAttributeName)) {
		if (PyUnicode_CompareWithASCIIString(pyAttributeName, "data") == 0) {
			PyErr_Clear();
			return self->pyData;
		}
	}
	return Py_TYPE(self)->tp_base->tp_getattro((PyObject*)self, pyAttributeName);
}

static void
PxImageView_dealloc(PxImageViewObject* self)
{
	DestroyWindow(self->hWin);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject *)self);
}

static PyMemberDef PxImageView_members[] = {
	{ "stretch", T_BOOL, offsetof(PxImageViewObject, bStretch), 0, "Stretch to fit into widget, keeping aspect ratio." },
	{ "fill", T_BOOL, offsetof(PxImageViewObject, bFill), 0, "Stretch to fit into widget, filling all available space (distorting)." },
	{ NULL }
};

static PyMethodDef PxImageView_methods[] = {
	{ NULL }
};

PyTypeObject PxImageViewType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.ImageView",         /* tp_name */
	sizeof(PxImageViewObject), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxImageView_dealloc, /* tp_dealloc */
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
	PxImageView_getattro,      /* tp_getattro */
	PxImageView_setattro,      /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Canvas, a widget for direct painting", /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxImageView_methods,       /* tp_methods */
	PxImageView_members,       /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxImageView_init, /* tp_init */
	0,                         /* tp_alloc */
	PxImageView_new,           /* tp_new */
};


LRESULT CALLBACK PxImageViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL PxImageViewType_Init()
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = PxImageViewWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g.hInstance;
	wc.hIcon = g.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"PxImageViewClass";
	wc.hIconSm = NULL;

	if (!RegisterClassEx(&wc)) {
		PyErr_SetFromWindowsErr(0);
		return FALSE;
	}

	hPlaceholderBitmap = (HBITMAP)LoadImage(g.hInstance, MAKEINTRESOURCE(IDB_PLACEHOLDER), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	if (hPlaceholderBitmap == NULL) {
		PyErr_SetFromWindowsErr(0);
		return FALSE;
	}
	return TRUE;
}

static
LRESULT CALLBACK PxImageViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int cxClient, cyClient;
	PAINTSTRUCT paintStruct;
	HDC hDC;
	PxImageViewObject* self = (PxImageViewObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_PAINT:
		if (self->pyData && self->pyData != Py_None) {
			PxImageObject* pyImage = (PxImageObject*)self->pyData;
			hDC = BeginPaint(hwnd, &paintStruct);
			if (pyImage->pyImageFormat == PyObject_GetAttrString(g.pyImageFormatEnum, "bmp")) {
				HDC hdcMem = CreateCompatibleDC(hDC);
				HBITMAP hbmOld = SelectObject(hdcMem, pyImage->hWin);
				GetObject(pyImage->hWin, sizeof(BITMAP), &bitmap);
				if (self->bFill) {
					StretchBlt(hDC, 0, 0, cxClient, cyClient, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
				}
				else if (self->bStretch) {

					double dAspectRatio = (double)cxClient / cyClient;
					double dPictureAspectRatio = (double)bitmap.bmWidth / bitmap.bmHeight;

					if (dPictureAspectRatio > dAspectRatio)
					{
						int nNewHeight = (int)(cxClient / dPictureAspectRatio);
						int nCenteringFactor = (cyClient - nNewHeight) / 2;
						StretchBlt(hDC, 0, nCenteringFactor, cxClient, nNewHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
					}
					else if (dPictureAspectRatio < dAspectRatio)
					{
						int nNewWidth = (int)(cyClient * dPictureAspectRatio);
						int nCenteringFactor = (cxClient - nNewWidth) / 2;
						StretchBlt(hDC, nCenteringFactor, 0, nNewWidth, cyClient, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
					}
					else
						StretchBlt(hDC, 0, 0, cxClient, cyClient, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
				}
				else
					BitBlt(hDC, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, hbmOld);
			}
			else if (pyImage->pyImageFormat == PyObject_GetAttrString(g.pyImageFormatEnum, "ico")) {
				DrawIcon(hDC, 0, 0, pyImage->hWin);
			}
			EndPaint(hwnd, &paintStruct);
		}
		break;

	case WM_RBUTTONDOWN:
		PxImageView_OpenFileDialog(self);
		break;

	case WM_SIZE:
		cxClient = LOWORD(lParam);
		cyClient = HIWORD(lParam);
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
		OutputDebugString(L"Image Destroyed!\n");
		if (self != NULL)
			Py_DECREF(self);
		return 0;
	}
	return DefFrameProc(hwnd, g.hMDIClientArea, msg, wParam, lParam);
}
