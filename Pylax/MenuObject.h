#ifndef Px_MENUOBJECT_H
#define Px_MENUOBJECT_H

typedef struct _PxMenuObject
{
	PyObject_HEAD
		HMENU hWin;
	LPWSTR szCaption;
}
PxMenuObject;

extern PyTypeObject PxMenuType;


typedef struct _PxMenuItemObject
{
	PyObject_HEAD
		UINT_PTR iIdentifier;
	LPWSTR szCaption;
	PyObject* pyOnClickCB;
}
PxMenuItemObject;

extern PyTypeObject PxMenuItemType;

void PxMenuItems_DeleteAll();

#endif

