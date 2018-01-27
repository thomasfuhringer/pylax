// WidgetObject.h  | Pylax © 2017 by Thomas Führinger
#ifndef Px_WIDGETOBJECT_H
#define Px_WIDGETOBJECT_H

#define PxDEFAULT            77777
#define PxWIDGET_CENTER      (PxDEFAULT-1)

#define PxWidgetObject_HEAD  \
		PyObject_HEAD \
        GtkWidget* gtk; \
        GtkFixed* gtkFixed; \
		PxWidgetObject* pyParent; \
		PxWindowObject* pyWindow; \
		Rect rc; \
		bool bTable; \
		bool bPointer; \
		bool bReadOnly; \
		bool bClean; \
		PyObject* pyData; \
		PyTypeObject* pyDataType; \
		PxLabelObject* pyLabel; \
		PyObject* pyFormat; \
		PyObject* pyFormatEdit; \
		PyObject* pyAlignHorizontal; \
		PyObject* pyAlignVertical; \
		PyObject* pyDataColumn; \
		PyObject* pyValidateCB; \
		PyObject* pyUserData; \
		PxDynasetObject* pyDynaset;

typedef struct _Rect {
	long  iLeft;
	long  iTop;
	long  iWidth;
	long  iHeight;
} Rect;

typedef struct _PxWindowObject PxWindowObject;
typedef struct _PxWidgetObject PxWidgetObject;
typedef struct _PxLabelObject PxLabelObject;

typedef struct _PxWidgetObject
{
	PxWidgetObject_HEAD
}
PxWidgetObject;

extern PyTypeObject PxWidgetType;

typedef struct _PxWidgetAddArgs
{
	GtkWidget* gtkParent;
	PyObject* pyCaption;
	bool bVisible;
}
PxWidgetAddArgs;

extern PxWidgetAddArgs gArgs;

bool PxWidget_CalculateRect(PxWidgetObject* self, Rect* rc);
bool PxWidget_RepositionChildren(PxWidgetObject* self);
bool PxWidget_Move(PxWidgetObject* self);
bool PxWidget_SetCaption(PxWidgetObject* self, PyObject* pyText);
bool PxWidget_Refresh(PxWidgetObject* self);
PyObject* PxWidget_PullData(PxWidgetObject* self);
bool PxWidget_SetData(PxWidgetObject* self, PyObject* pyData);
void GtkWidget_DestroyCB(GtkWidget* gtkWidget, gpointer gUserData);

typedef enum { ALIGN_NONE, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_TOP, ALIGN_BOTTOM, ALIGN_BLOCK } PxAlignEnum;

#endif
