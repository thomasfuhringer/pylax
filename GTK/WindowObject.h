#ifndef Px_WINDOWOBJECT_H
#define Px_WINDOWOBJECT_H

#define PxWindowObject_HEAD  \
		PxWidgetObject_HEAD \
		PyObject* pyConnection; \
        int iMinX; \
        int iMinY; \
        PyObject* pyName; \
        bool bNameInCaption; \
        bool bBlocking; \
        GtkWidget gtkLastFocus; \
        PxWidgetObject* pyFocusWidget; \
		PyObject* pyBeforeCloseCB;

typedef struct _PxWindowObject
{
	PxWindowObject_HEAD
}
PxWindowObject;

extern PyTypeObject PxWindowType;

bool PxWindowType_Init(void);
bool PxWindow_SaveState(PxWindowObject* self);
bool PxWindow_RestoreState(PxWindowObject* self);
int PxWindow_MoveFocus(PxWindowObject* self, PxWidgetObject* pyDestinationWidget);
bool PxWindow_SetCaption(PxWindowObject* self, PyObject* pyText);
gboolean GtkWindow_ConfigureEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData);
PyObject* PxWindow_set_icon_from_file(PxWindowObject* self, PyObject* args, PyObject* kwds);

#endif
