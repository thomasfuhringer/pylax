// Utilities.c  | Pylax © 2016 by Thomas Führinger
#include <Pylax.h>
#include "Resource.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <Shlobj.h>



void ShowPythonError()
{
	PyErr_Print();
	MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
}

LPVOID HAlloc(_In_ SIZE_T dwBytes)
{
	return HeapAlloc(g.hHeap, HEAP_GENERATE_EXCEPTIONS, dwBytes);
}

BOOL HFree(_In_ LPVOID pMem)
{
	return HeapFree(g.hHeap, 0, pMem);
}


LPCSTR toU8(const LPWSTR szUTF16)
{
	if (szUTF16 == NULL)
		return NULL;
	if (*szUTF16 == L'\0')
		return '\0';

	int cbUTF8 = WideCharToMultiByte(CP_UTF8, 0, szUTF16, -1, NULL, 0, NULL, NULL);
	if (cbUTF8 == 0) {
		ShowLastError(L"Sting converson to UTF8 failed!");
		return NULL;
	}
	LPCSTR strTextUTF8 = (LPCSTR)PyMem_RawMalloc(cbUTF8);
	int result = WideCharToMultiByte(CP_UTF8, 0, szUTF16, -1, strTextUTF8, cbUTF8, NULL, NULL);
	if (result == 0) {
		ShowLastError(L"Sting converson to UTF8 failed!");
		return NULL;
	}
	return strTextUTF8;
}

LPWSTR toW(const LPCSTR strTextUTF8)
{
	if (strTextUTF8 == NULL)
		return NULL;
	if (*strTextUTF8 == '\0')
		return L'\0';

	int cchUTF16 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, strTextUTF8, -1, NULL, 0); // request buffer size
	if (cchUTF16 == 0) {
		ShowLastError(L"Sting converson to wide character failed!");
		return NULL;
	}
	LPWSTR szUTF16 = (LPWSTR)PyMem_RawMalloc(cchUTF16 * sizeof(WCHAR));
	int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, strTextUTF8, -1, szUTF16, cchUTF16);
	if (result == 0) {
		ShowLastError(L"Sting converson to wide character failed!");
		return NULL;
	}
	return szUTF16;
}

LPTSTR
StringArrayCat(LPCTSTR *pszStrings, int nElements)
{
	int iCharacters = 1;
	for (int i = 0; i < nElements; i++)	{
		iCharacters += lstrlen(pszStrings[i]);
	}

	LPTSTR result = (LPTSTR)PyMem_RawMalloc(sizeof(TCHAR) * iCharacters);

	lstrcpy(result, pszStrings[0]);
	for (int i = 1; i < nElements; i++)	{
		lstrcat(result, pszStrings[i]);
	}
	return result;
}

char*
StringAppend(char* strMain, char* strAppex)
// strMain must be on the heap!
{
	if (strMain == NULL || strAppex == NULL || strAppex[0] == '\0')
		return strMain;

	size_t nLen = lstrlenA(strMain) + lstrlenA(strAppex) + 1;

	strMain = (char*)PyMem_RawRealloc(strMain, nLen);

	if (FAILED(StringCbCatA(strMain, nLen, strAppex))){
		return NULL;
	}

	return strMain;
}


char*
StringAppend2(char* strMain, char* strAppex, char* strAppex2)
// strMain must be on the heap!
{
	if (strMain == NULL || strAppex == NULL || strAppex[0] == '\0')
		return strMain;
	size_t nLen = lstrlenA(strMain) + lstrlenA(strAppex) + lstrlenA(strAppex2) + 1;

	strMain = (char*)PyMem_RawRealloc(strMain, nLen);

	if (FAILED(StringCbCatA(strMain, nLen, strAppex))){
		return NULL;
	}
	if (FAILED(StringCbCatA(strMain, nLen, strAppex2))){
		return NULL;
	}

	return strMain;
}

/*
LPCSTR StringAppendEx(LPCSTR strResult, ...)
// strPart must be on the heap, arguments terminated by NULL
{
va_list valist;
size_t len = 0;
LPCSTR strPart;

//strResult = strPart;
len = (strResult == NULL ? 0 : strlen(strResult));

va_start(valist, strResult);
while (strPart = va_arg(valist, LPCSTR) != NULL)
{
ShowInts("X", strlen(strPart), len);
MessageBoxA(0, strPart, "strPart", 0);
//len += (strPart == NULL ? 0 : strlen(strPart));
}
va_end(valist);

strResult = (char*)PyMem_RawRealloc(strResult, len);
if (strResult == NULL)
return NULL;

va_start(valist, strPart);
while (strPart = va_arg(valist, LPCSTR) != NULL)
strncat(strResult, strPart, len);
va_end(valist);

return strResult;
}
*/
void ShowLastError(LPCTSTR lpszContext)
{
	TCHAR szBuffer[255];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szBuffer, sizeof(szBuffer) / sizeof(TCHAR), 0);
	if (!MessageBox(0, szBuffer, lpszContext, 0))
		MessageBox(0, L"Can not show", lpszContext, 0);
}

void PyObject_ShowRepr(PyObject* pyObject)
{
	PyObject* objectsRepresentation = PyObject_Repr(pyObject);
	const char* str = PyUnicode_AsUTF8(objectsRepresentation);
	if (!MessageBoxA(0, str, "PyObject_Repr", 0))
		MessageBox(0, L"Can not show", L"PyObject_Repr", 0);
	Py_DECREF(objectsRepresentation);
}

BOOL
PxAttachObject(PyObject** ppyMember, PyObject* pyObject, BOOL bStrong)
{
	PyObject* tmp = *ppyMember;
	if (bStrong)
		Py_INCREF(pyObject);
	*ppyMember = pyObject;
	Py_DECREF(tmp);
	return TRUE;
}

void XX(PyObject* pyObject)
{
	if (pyObject){
		char buffer[30];
		sprintf(buffer, "%d", pyObject->ob_refcnt);
		PyObject* objectsRepresentation = PyObject_Repr(pyObject);
		const char* str = PyUnicode_AsUTF8(objectsRepresentation);
		if (!MessageBoxA(0, str, buffer, 0))
			MessageBox(0, L"Can not show", L"PyObject_Repr", 0);
		Py_DECREF(objectsRepresentation);
	}
	else {
		TCHAR szBuffer[255];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szBuffer, sizeof(szBuffer) / sizeof(TCHAR), 0);
		MessageBoxA(0, "NULL", szBuffer, 0);
	}
}

void
Xi(char* sContext, int a)
{
	TCHAR buffera[30];
	sprintf_s(buffera, 30, "%d", a);
	OutputDebugStringA("\nI--- ");
	OutputDebugStringA(sContext);
	OutputDebugStringA(" - ");
	OutputDebugStringA(buffera);
}

PyObject*
PyObject_Copy(PyObject* pyObject)
// unused!
{
	PyObject* pyArgs = PyTuple_Pack(1, pyObject);
	PyObject* pyDuplicate = PyObject_CallObject(g.pyCopyFunction, pyArgs);
	Py_DECREF(pyArgs);
	return pyDuplicate;
}

void ShowInts(LPCTSTR lpszContext, int a, int b)
{
	TCHAR buffera[30];
	TCHAR bufferb[30];
	wsprintf(buffera, TEXT("%d"), a);
	wsprintf(bufferb, TEXT("%d"), b);
	LPCTSTR arr[3] = { buffera, TEXT(" | "), bufferb };
	LPTSTR out = StringArrayCat(arr, 3);
	MessageBox(g.hWin, out, lpszContext, MB_OK);
	PyMem_RawFree(out);
}

void OutputDebugInts(LPCTSTR lpszContext, int a, int b)
{
	TCHAR buffera[30];
	TCHAR bufferb[30];
	wsprintf(buffera, TEXT("%d"), a);
	wsprintf(bufferb, TEXT("%d"), b);
	LPCTSTR arr[4] = { buffera, TEXT(" | "), bufferb, TEXT("\n") };
	LPTSTR out = StringArrayCat(arr, 3);
	OutputDebugString(lpszContext);
	OutputDebugString(out);
	PyMem_RawFree(out);
}
