#ifndef Px_FORMOBJECT_H
#define Px_FORMOBJECT_H

typedef struct _PxFormObject
{
	PxWindowObject_HEAD
		GtkLabel* gtkLabel;
}
PxFormObject;

extern PyTypeObject PxFormType;

bool PxForm_Close(PxFormObject* self);

#endif