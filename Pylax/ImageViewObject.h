#ifndef Px_IMAGEVIEWOBJECT_H
#define Px_IMAGEVIEWOBJECT_H


typedef struct _PxImageViewObject
{
	PxWidgetObject_HEAD
	BOOL bStretch;
	BOOL bFill;
}
PxImageViewObject;

extern PyTypeObject PxImageViewType;

BOOL PxImageView_SetData(PxImageObject* self, PyObject* pyData);

#endif /* Px_IMAGEVIEWOBJECT_H */

