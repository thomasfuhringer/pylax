// EntryObject.h  | Pylax © 2017 by Thomas Führinger

#ifndef Px_ENTRYOBJECT_H
#define Px_ENTRYOBJECT_H

typedef struct _PxEntryObject
{
	PxWidgetObject_HEAD
		PyObject* pyOnClickButtonCB;
}
PxEntryObject;

extern PyTypeObject PxEntryType;

#endif