#ifndef Px_LABELOBJECT_H
#define Px_LABELOBJECT_H

typedef struct _PxLabelObject
{
	PxWidgetObject_HEAD
		PxWidgetObject* pyAssociatedWidget;
	PxWidgetObject* pyCaptionClient;
    COLORREF textColor;
}
PxLabelObject;

extern PyTypeObject PxLabelType;

#endif /* Px_LABELOBJECT_H */

