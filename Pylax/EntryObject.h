// EntryObject.h  | Pylax © 2015 by Thomas Führinger
#ifndef Px_ENTRYOBJECT_H
#define Px_ENTRYOBJECT_H

typedef struct _PxEntryObject
{
	PxWidgetObject_HEAD
		PyObject* pyOnClickButtonCB;
	//BOOL bGotFocus;

	// for inserted button
	BOOL bButtonDown;
	BOOL bButtonMouseDown;
	int cxLeftEdge;
	int cxRightEdge;
	int cyTopEdge;
	int cyBottomEdge;
}
PxEntryObject;

extern PyTypeObject PxEntryType;

BOOL PxEntry_Entering(PxEntryObject* self);
BOOL PxEntry_Changed(PxEntryObject* self);
BOOL PxEntry_Left(PxEntryObject* self);
BOOL PxEntry_RepresentCell(PxEntryObject* self);

LRESULT CALLBACK PxEntryProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif

