// MarkDownEntryObject.h  | Pylax © 2018 by Thomas Führinger

#ifndef Px_MARKDOWNENTRYOBJECT_H
#define Px_MARKDOWNENTRYOBJECT_H

typedef struct _PxMarkDownEntryObject
{
	PxWidgetObject_HEAD
		GtkTextBuffer*	gtkTextBuffer;
	bool bPlain;
}
PxMarkDownEntryObject;

extern PyTypeObject PxMarkDownEntryType;

#endif
