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

//bool PxComboBox_Selected(PxComboBoxObject* self, int iItem);
//bool PxComboBox_Entering(PxComboBoxObject* self);
//bool PxComboBox_Changed(PxComboBoxObject* self);
//bool PxComboBox_Left(PxComboBoxObject* self);

#endif