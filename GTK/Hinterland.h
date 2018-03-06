#ifndef Px_HINTERLAND_H
#define Px_HINTERLAND_H

typedef struct _PxHinterlandGlobals
{
	PyObject* pyModule;
	PyObject* pyClientType;
	PyObject* pyMsg;
	PyObject* pyResultMsg;
	PyObject* MsgClass;
	PyObject* Msg_Type;
	PyObject* Msg_Get;
	PyObject* Msg_Set;
	PyObject* Msg_Delete;
	char* sStatusMessage;
}
PxHinterlandGlobals;

extern PxHinterlandGlobals hl;

bool Hinterland_Init();
bool Hinterland_Exchange();

#endif
