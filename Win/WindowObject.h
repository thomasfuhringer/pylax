#ifndef Px_WINDOWOBJECT_H
#define Px_WINDOWOBJECT_H

#define PxWindowObject_HEAD  \
		PxWidgetObject_HEAD \
		PyObject* pyConnection; \
        int cxMin; \
        int cyMin; \
        PyObject* pyName; \
		BOOL bModal; \
        BOOL bNameInCaption; \
        BOOL bOkPressed; \
        HWND hLastFocus; \
        PxWidgetObject* pyFocusWidget; \
        PxImageObject* pyIcon; \
		PxButtonObject* pyOkButton; \
        PxButtonObject* pyCancelButton; \
		PyObject* pyBeforeCloseCB;

typedef struct _PxWindowObject
{
	PxWindowObject_HEAD
}
PxWindowObject;

extern PyTypeObject PxWindowType;

BOOL PxWindowType_Init();
BOOL PxWindow_SaveState(PxWindowObject* self);
BOOL PxWindow_RestoreState(PxWindowObject* self);
int PxWindow_MoveFocus(PxWindowObject* self, PxWidgetObject* pyDestinationWidget);
PyObject* PxWindow_create_timer(PxWindowObject *self, PyObject *args, PyObject *kwds);
PyObject* PxWindow_delete_timer(PxWindowObject *self, PyObject *args, PyObject *kwds);
LRESULT DefParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bMDI);

#endif

