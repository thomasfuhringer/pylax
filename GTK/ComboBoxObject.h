#ifndef Px_COMBOBOXOBJECT_H
#define Px_COMBOBOXOBJECT_H

typedef struct _PxComboBoxObject
{
	PxWidgetObject_HEAD
		PyObject* pyItems; // PyList
	bool bNoneSelectable;
}
PxComboBoxObject;

extern PyTypeObject PxComboBoxType;

#endif