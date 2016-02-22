#include "Pylax.h"

static HBITMAP PyBytes_AsBitmap(PyObject* pyBytes);

static PyObject*
PxImage_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PxImageObject* self = (PxImageObject*)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->pyImageFormat = NULL;
		return (PyObject*)self;
	}
	else
		return NULL;
}

static int
PxImage_init(PxImageObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "source", NULL };
	PyObject* pySource = NULL;
	self->hWin = g.hIcon;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist,
		&pySource))
		return -1;

	if (pySource) {
		if (PyUnicode_Check(pySource)) {
			LPWSTR szFileName = toW(PyUnicode_AsUTF8(pySource));
			LPWSTR szExtension = PathFindExtensionW(szFileName);

			if (lstrcmpi(szExtension, L".ico") == 0) {
				self->hWin = LoadImageW(0, szFileName, IMAGE_ICON, 0, 0, LR_LOADFROMFILE); //LR_DEFAULTSIZE | 
				if (self->hWin == 0) {
					PyErr_SetFromWindowsErr(0);
					return -1;
				}
				self->pyImageFormat = PyObject_GetAttrString(g.pyImageFormatEnum, "ico");
				Py_INCREF(self->pyImageFormat);
			}
			else if (lstrcmpi(szExtension, L".bmp") == 0) {
				self->hWin = (HBITMAP)LoadImageW(NULL, szFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
				if (self->hWin == 0) {
					PyErr_SetFromWindowsErr(0);
					return -1;
				}
				self->pyImageFormat = PyObject_GetAttrString(g.pyImageFormatEnum, "bmp");
				Py_INCREF(self->pyImageFormat);
			}
			PyMem_RawFree(szFileName);
		}
		else if (PyObject_TypeCheck(pySource, &PyBytes_Type)) {
			self->hWin = PyBytes_AsBitmap(pySource);
			if (self->hWin == 0) {
				return -1;
			}
			self->pyImageFormat = PyObject_GetAttrString(g.pyImageFormatEnum, "bmp");
			Py_INCREF(self->pyImageFormat);
		}
		else  {
			PyErr_SetString(PyExc_TypeError, "Parameter 1 ('source') can only be a byte object or a string with file name.");
			return -1;
		}
	}

	//SetWindowLongPtr(self->hWin, GWLP_USERDATA, (LONG_PTR)self);
	return 0;
}

static BOOL
PxImage_Load(PxImageObject* self, LPCSTR strFileName)
{
	LPWSTR szFileName;
	szFileName = toW(strFileName);
	self->hWin = (HBITMAP)LoadImage(NULL, szFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	PyMem_RawFree(szFileName);
	return TRUE;
}

static PyObject*
PxImage_load(PxImageObject* self, PyObject* args)
{
	const char* strFileName;

	if (!PyArg_ParseTuple(args, "s",
		&strFileName))
		return NULL;

	if (!PxImage_Load(self, strFileName))
		return NULL;

	Py_RETURN_NONE;
}

static PyObject*
PxImage_AsBytes(PxImageObject* self)
// from https://phvu.wordpress.com/2009/07/09/serialize-and-deserialize-bitmap-object-in-mfcwin32/
{
	if (!PyObject_TypeCheck(self, &PxImageType)) {
		PyErr_SetString(PyExc_TypeError, "Argument must be an Image object.");
		return NULL;
	}
	//HBITMAP hBitmap;
	int len;

	BITMAP bmpObj;
	HDC hDCScreen;
	int iRet;
	DWORD dwBmpSize;

	BITMAPFILEHEADER    bmfHeader;
	LPBITMAPINFO        lpbi;
	const DWORD dwSizeOfBmfHeader = sizeof(BITMAPFILEHEADER);
	DWORD dwSizeOfBmInfo = sizeof(BITMAPINFO);

	hDCScreen = GetDC(NULL);
	GetObject(self->hWin, sizeof(BITMAP), &bmpObj);

	lpbi = HAlloc(dwSizeOfBmInfo + 8);
	lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	// Gets the "bits" from the bitmap and copies them into a buffer
	// which is pointed to by lpbi
	iRet = GetDIBits(hDCScreen, self->hWin, 0, (UINT)bmpObj.bmHeight, NULL, lpbi, DIB_RGB_COLORS);
	assert(iRet > 0);

	// only 16 and 32 bit images are supported.
	assert(lpbi->bmiHeader.biBitCount == 16 || lpbi->bmiHeader.biBitCount == 32);
	if (lpbi->bmiHeader.biCompression == BI_BITFIELDS)
		dwSizeOfBmInfo += 8;

	dwBmpSize = lpbi->bmiHeader.biSizeImage;
	char* lpbitmap = (char*)HAlloc(dwBmpSize);

	iRet = GetDIBits(hDCScreen, self->hWin, 0, (UINT)bmpObj.bmHeight, lpbitmap, lpbi, DIB_RGB_COLORS);
	assert(iRet > 0);

	DWORD dwSizeofDIB = dwBmpSize + dwSizeOfBmfHeader + dwSizeOfBmInfo;
	bmfHeader.bfOffBits = (DWORD)dwSizeOfBmfHeader + (DWORD)dwSizeOfBmInfo;
	bmfHeader.bfSize = dwSizeofDIB;
	bmfHeader.bfType = 0x4D42; //BM
	bmfHeader.bfReserved1 = bmfHeader.bfReserved2 = 0;

	char* arrData = (char*)HAlloc(dwSizeofDIB);
	memcpy(arrData, &bmfHeader, dwSizeOfBmfHeader);
	memcpy(arrData + dwSizeOfBmfHeader, lpbi, dwSizeOfBmInfo);
	memcpy(arrData + dwSizeOfBmfHeader + dwSizeOfBmInfo, lpbitmap, dwBmpSize);


	HFree(lpbi);
	HFree(lpbitmap);
	ReleaseDC(NULL, hDCScreen);

	len = dwSizeofDIB;
	PyObject* pyBytes = PyBytes_FromStringAndSize(arrData, len);
	HFree(arrData);
	return  pyBytes;
}

static PyObject*
PxImage_as_bytes(PxImageObject* self)
{
	return PxImage_AsBytes(self);
}

static HBITMAP
PyBytes_AsBitmap(PyObject* pyBytes)
{
	PBITMAPFILEHEADER    bmfHeader;
	PBITMAPINFO    pbi;
	HDC            hDC;
	HBITMAP        hBmpRet;
	int            iRet;
	//char        *lpbitmap;
	int            iSizeOfBmInfo;
	const int    iSizeOfBmfHeader = sizeof(BITMAPFILEHEADER);

	if (!PyObject_TypeCheck(pyBytes, &PyBytes_Type)) {
		PyErr_SetString(PyExc_TypeError, "Argument must be a bytes object.");
		return NULL;
	}

	char* arrData = PyBytes_AsString(pyBytes);
	int iLen = PyBytes_Size(pyBytes);


	// get the BITMAPFILEHEADER
	bmfHeader = (PBITMAPFILEHEADER)arrData;
	arrData += iSizeOfBmfHeader;

	// get the BITMAPINFO
	iSizeOfBmInfo = bmfHeader->bfOffBits - iSizeOfBmfHeader;
	pbi = (PBITMAPINFO)arrData;
	arrData += iSizeOfBmInfo;

	assert(pbi->bmiHeader.biSizeImage == iLen - iSizeOfBmfHeader - iSizeOfBmInfo);

	// create the output DDB bitmap
	hDC = GetDC(NULL);
	hBmpRet = CreateCompatibleBitmap(hDC,
		pbi->bmiHeader.biWidth, pbi->bmiHeader.biHeight);
	assert(hBmpRet);

	// set the image...
	iRet = SetDIBits(hDC, hBmpRet, 0,
		pbi->bmiHeader.biHeight,
		arrData,
		pbi, DIB_RGB_COLORS);
	assert(iRet > 0);

	ReleaseDC(NULL, hDC);
	return hBmpRet;
}

static PyObject*
PxImage_FromBytes(PyObject* pyBytes)
{
	PxImageObject* self = (PxImageObject*)PxImage_new(&PxImageType, NULL, NULL);
	self->hWin = PyBytes_AsBitmap(pyBytes);
	return (PyObject*)self;
}


static void
PxImage_dealloc(PxImageObject* self)
{
	if (self->pyImageFormat = PyObject_GetAttrString(g.pyImageFormatEnum, "bmp"))
		DeleteObject(self->hWin);
	else if (self->pyImageFormat == PyObject_GetAttrString(g.pyImageFormatEnum, "ico"))
		DestroyIcon(self->hWin);
	Py_TYPE(self)->tp_base->tp_dealloc((PyObject*)self);
}

static PyMemberDef PxImage_members[] = {
	{ "format", T_OBJECT, offsetof(PxImageObject, pyImageFormat), 0, "Format" },
	{ NULL }
};

static PyMethodDef PxImage_methods[] = {
	{ "load", (PyCFunction)PxImage_load, METH_VARARGS, "Load from file" },
	{ "bytes", (PyCFunction)PxImage_as_bytes, METH_NOARGS, "Convert to bytes object." },
	{ NULL }
};

PyTypeObject PxImageType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"pylax.Image",             /* tp_name */
	sizeof(PxImageObject),     /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)PxImage_dealloc, /* tp_dealloc */
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
	"Image object",            /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	PxImage_methods,           /* tp_methods */
	PxImage_members,           /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)PxImage_init,    /* tp_init */
	0,                         /* tp_alloc */
	PxImage_new,               /* tp_new */
};