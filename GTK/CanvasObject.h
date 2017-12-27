#ifndef Px_CANVASOBJECT_H
#define Px_CANVASOBJECT_H


typedef struct _PxCanvasObject
{
	PxWidgetObject_HEAD
		guint iWidth;
	guint iHeight;
	cairo_t* caiContext;
	PyObject* pyOnPaintCB;
}
PxCanvasObject;

extern PyTypeObject PxCanvasType;

bool PxCanvasType_Init();

#endif