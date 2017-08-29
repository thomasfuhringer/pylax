#ifndef Px_COMBOBOXOBJECT_H
#define Px_COMBOBOXOBJECT_H

typedef struct _PxComboBoxObject
{
	PxWidgetObject_HEAD
		PyObject* pyItems; // PyList
	BOOL bNoneSelectable;
}
PxComboBoxObject;

extern PyTypeObject PxComboBoxType;

BOOL PxComboBox_Selected(PxComboBoxObject* self, int iItem);
BOOL PxComboBox_Entering(PxComboBoxObject* self);
BOOL PxComboBox_Changed(PxComboBoxObject* self);
BOOL PxComboBox_Left(PxComboBoxObject* self);

#endif /* !Px_COMBOBOXOBJECT_H */

