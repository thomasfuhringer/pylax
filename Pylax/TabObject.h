#ifndef Px_TABOBJECT_H
#define Px_TABOBJECT_H

typedef struct _PxTabPageObject PxTabPageObject;
typedef struct _PxTabObject
{
	PxWidgetObject_HEAD
		PyObject* pyPages; // PyList
	int iPages;
	PxTabPageObject* pyCurrentPage;
}
PxTabObject;

extern PyTypeObject PxTabType;


typedef struct _PxTabPageObject
{
	PxWidgetObject_HEAD
		int iIndex;
}
PxTabPageObject;

extern PyTypeObject PxTabPageType;

#endif /* !Px_TABOBJECT_H */

