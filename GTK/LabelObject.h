#ifndef Px_LABELOBJECT_H
#define Px_LABELOBJECT_H

typedef struct _PxLabelObject
{
	PxWidgetObject_HEAD
		PxWidgetObject* pyAssociatedWidget;
	PxWidgetObject* pyCaptionClient;
}
PxLabelObject;

extern PyTypeObject PxLabelType;

bool PxLabel_SetCaption(PxLabelObject* self, PyObject* pyText);

#endif