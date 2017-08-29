#ifndef Px_CANVASOBJECT_H
#define Px_CANVASOBJECT_H


typedef struct _PxCanvasObject
{
	PxWidgetObject_HEAD
		HDC hDC;
	HBRUSH hBrush;
	HPEN hPen;
	PyObject* pyOnPaintCB;
}
PxCanvasObject;

extern PyTypeObject PxCanvasType;

BOOL PxCanvasType_Init();

#endif /* !Px_CANVASOBJECT_H */

