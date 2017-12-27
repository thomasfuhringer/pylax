#ifndef Px_MENUOBJECT_H
#define Px_MENUOBJECT_H

typedef struct _PxMenuObject
{
	PyObject_HEAD
		GtkWidget* gtk;
}
PxMenuObject;

extern PyTypeObject PxMenuType;


typedef struct _PxMenuItemObject
{
	PyObject_HEAD
		GtkWidget* gtk;
	PyObject* pyOnClickCB;
}
PxMenuItemObject;

extern PyTypeObject PxMenuItemType;

void PxMenuItems_DeleteAll();
bool PxMenuItem_Clicked(PxMenuItemObject* self);

#endif