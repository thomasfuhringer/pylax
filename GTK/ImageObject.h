#ifndef Px_IMAGEOBJECT_H
#define Px_IMAGEOBJECT_H

#define Px_IMAGE_WIDTH   320
#define Px_IMAGE_HEIGHT  320

typedef struct _PxImageObject
{
	PxWidgetObject_HEAD
		GtkWidget* gtkImage;
	GtkWidget* gtkButton;
	PyObject* pyImageFormat;
	bool bScale;
	long iMaxSize;
	long iPrevWidth;
	long iPrevHeight;
}
PxImageObject;

extern PyTypeObject PxImageType;

typedef enum { PxIMAGEFORMAT_ICO, PxIMAGEFORMAT_BMP, PxIMAGEFORMAT_JPG } PxImageFormatEnum;

#endif
