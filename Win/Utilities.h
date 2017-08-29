#ifndef PxUTILITIES_H
#define PxUTILITIES_H

void ShowLastError(LPCTSTR lpszContext);
void ShowPythonError();
LPCSTR toU8(const LPWSTR pszTextUTF16);
LPWSTR toW(const LPCSTR strTextUTF8);
LPTSTR StringArrayCat(_In_ LPCTSTR *pszStrings, _In_ int nElements);
char* StringAppend(char* strMain, char* strAppex);
char* StringAppend2(char* strMain, char* strAppex, char* strAppex2);
//LPCSTR StringAppendEx(LPCSTR strPart, ...);
BOOL PxAttachObject(PyObject** ppyMember, PyObject* pyObject, BOOL bStrong);

// Conveniance Wrappers
LPVOID HAlloc(_In_ SIZE_T dwBytes);
BOOL   HFree(_In_ LPVOID pMem);

// For debugging purposes
void ShowInts(LPCTSTR lpszContext, int a, int b);
void ShowFloat(LPCTSTR pszContext, float out);
void Xi(char* sContext, int a);
void OutputDebugInts(LPCTSTR lpszContext, int a, int b);
void XX(PyObject* pyObject);
PyObject* PyObject_Copy(PyObject* pyObject);

#endif 
