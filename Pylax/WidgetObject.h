// WidgetObject.h  | Pylax © 2016 by Thomas Führinger
#ifndef Px_WIDGETOBJECT_H
#define Px_WIDGETOBJECT_H

#define PxWIDGET_CENTER      (CW_USEDEFAULT-1)
#define PxWINDOWBKGCOLOR     COLOR_MENU // COLOR_WINDOW

#define PxWidgetObject_HEAD  \
		PyObject_HEAD \
		PxWidgetObject* pyParent; \
		HWND hWin; \
        WNDPROC fnOldWinProcedure; \
		PxWindowObject* pyWindow; \
		RECT rc; \
        HBRUSH hBkgBrush; \
		BOOL bTable; \
		BOOL bPointer; \
		BOOL bReadOnly; \
		PyObject* pyData; \
		PyTypeObject* pyDataType; \
		PxLabelObject* pyLabel; \
		PyObject* pyFormat; \
		PyObject* pyFormatEdit; \
		PyObject* pyAlignHorizontal; \
		PyObject* pyAlignVertical; \
		PxDynasetObject* pyDynaset; \
		PyObject* pyDataColumn; \
		PyObject* pyVerifyCB;

#define PX_FRAME WS_EX_STATICEDGE

typedef struct _PxWindowObject PxWindowObject;
typedef struct _PxWidgetObject PxWidgetObject;
typedef struct _PxLabelObject PxLabelObject;

typedef struct _PxWidgetObject
{
	PxWidgetObject_HEAD
}
PxWidgetObject;

extern PyTypeObject PxWidgetType;

typedef struct _PxWidgetAddArgs
{
	HWND hwndParent;
	PyObject* pyCaption;
	BOOL bVisible;
}
PxWidgetAddArgs;

extern PxWidgetAddArgs gArgs;

void TransformRectToAbs(_In_ PRECT rcRel, _In_ HWND hwndParentWindow, _Out_ PRECT rcAbs);
BOOL PxWidget_MoveWindow(PxWidgetObject* self);
BOOL PxWidget_SetCaption(PxWidgetObject* self, PyObject* pyText);
BOOL PxWidget_Refresh(PxWidgetObject* self);
PyObject* PxWidget_PullData(PxWidgetObject* self);
BOOL PxWidget_SetData(PxWidgetObject* self, PyObject* pyData);

PyObject* PxFormatData(PyObject* pyData, PyObject* pyFormat);
PyObject* PxParseString(LPCSTR strText, PyTypeObject* pyDataType, PyObject* pyFormat);
BOOL CALLBACK PxWidgetSizeEnumProc(HWND hwndChild, LPARAM lParam);

typedef enum  { ALIGN_NONE, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_TOP, ALIGN_BOTTOM, ALIGN_BLOCK } PxAlignEnum;

#endif