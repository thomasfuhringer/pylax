#ifndef Px_BUTTONOBJECT_H
#define Px_BUTTONOBJECT_H


typedef struct _PxButtonObject
{
	PxWidgetObject_HEAD
		PyObject* pyOnClickCB;
}
PxButtonObject;

extern PyTypeObject PxButtonType;

BOOL PxButton_Clicked(PxButtonObject* self);

#endif /* !Px_BUTTONOBJECT_H */

