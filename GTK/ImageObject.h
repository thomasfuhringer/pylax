#ifndef Px_IMAGEOBJECT_H
#define Px_IMAGEOBJECT_H

#define Px_IMAGE_WIDTH   320
#define Px_IMAGE_HEIGHT  320

typedef struct _PxImageObject
{
	PxWidgetObject_HEAD
		GtkWidget* gtkImage;
	PyObject* pyImageFormat;
	bool bStretch;
	bool bFill;
}
PxImageObject;

extern PyTypeObject PxImageType;

typedef enum { PxIMAGEFORMAT_ICO, PxIMAGEFORMAT_BMP, PxIMAGEFORMAT_JPG } PxImageFormatEnum;

#endif
