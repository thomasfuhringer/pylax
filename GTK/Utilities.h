#ifndef PxUTILITIES_H
#define PxUTILITIES_H

char* StringArrayCat(char** sStrings, int iElements);
char* StringAppend(char* sMain, char* sAppex);
char* StringAppend2(char* sMain, char* sAppex1, char* sAppex2);

void ErrorDialog(char* sMessage);
void PythonErrorDialog();
PyTupleObject* PyTuple_Duplicate(PyTupleObject* pyTuple);
bool PxAttachObject(PyObject** ppyMember, PyObject* pyObject, bool bStrong);
PyObject* PxFormatData(PyObject* pyData, PyObject* pyFormat);
PyObject* PxParseString(char* sText, PyTypeObject* pyDataType, PyObject* pyFormat);

// For debugging purposes
void XX(PyObject* pyObject);
void Xx(char* sText, PyObject* pyObject);
#endif
