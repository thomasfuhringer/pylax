#ifndef Px_IMAGEOBJECT_H
#define Px_IMAGEOBJECT_H

typedef struct _PxImageObject
{
	PyObject_HEAD
		HANDLE hWin;
	PyObject* pyImageFormat;
}
PxImageObject;

extern PyTypeObject PxImageType;

typedef enum  { PxIMAGEFORMAT_ICO, PxIMAGEFORMAT_BMP, PxIMAGEFORMAT_JPG } PxImageFormatEnum;

#endif /* Px_IMAGEOBJECT_H */

