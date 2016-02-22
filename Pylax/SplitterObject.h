#ifndef Px_SPLITTEROBJECT_H
#define Px_SPLITTEROBJECT_H

typedef struct _PxSplitterObject
{
	PxWidgetObject_HEAD
		PxBoxObject* pyBox1;
	PxBoxObject* pyBox2;
	BOOL bVertical;
	int iPosition;
	int iSpacing;
}
PxSplitterObject;

extern PyTypeObject PxSplitterType;

BOOL PxSplitterType_Init();

#endif /* Px_SPLITTEROBJECT_H */