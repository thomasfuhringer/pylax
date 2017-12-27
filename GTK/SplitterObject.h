#ifndef Px_SPLITTEROBJECT_H
#define Px_SPLITTEROBJECT_H

typedef struct _PxSplitterObject
{
	PxWidgetObject_HEAD
		PxBoxObject* pyBox1;
	PxBoxObject* pyBox2;
	bool bVertical;
	bool bBox1Resizes;
	bool bBox2Resizes;
	int iPosition;
	int iSpacing;
}
PxSplitterObject;

extern PyTypeObject PxSplitterType;

#endif