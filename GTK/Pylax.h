// Pylax.h  |  Pylax © 2017 by Thomas Führinger
#ifndef PYLAX_H
#define PYLAX_H

#include <Python.h>
#include "structmember.h"
#include "egg-markdown.h"

// C RunTime Header Files
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

// GTK Header Files
#include <gtk/gtk.h>

#include "Hinterland.h"

// Pylax Classes
#include "Version.h"
#include "DynasetObject.h"
#include "MenuObject.h"
#include "WidgetObject.h"
#include "BoxObject.h"
#include "Utilities.h"
#include "WindowObject.h"
#include "FormObject.h"
#include "EntryObject.h"
#include "MarkDownEntryObject.h"
#include "ComboBoxObject.h"
#include "ImageObject.h"
#include "ButtonObject.h"
#include "LabelObject.h"
#include "CanvasObject.h"
#include "SplitterObject.h"
#include "TabObject.h"
#include "TableObject.h"

#define PARSE_DECLTYPES 1 // from Python-3.4.2\Modules\_sqlite\module.h
#define PARSE_COLNAMES  2
#define TIMEOUT 5

typedef struct _PxGlobals
{
	GtkApplication* gtkApp;
	char sExecutablePath[PATH_MAX];
	char sAppPath[PATH_MAX];
	wchar_t* wcModulePathOld;
	GtkWindow* gtkMainWindow;
	GtkAction* gtkActionFileOpen;
	GtkAction* gtkActionFileClose;
	GtkWidget* gtkAppMenu;
	GtkWidget* gtkAppMenuItem;
	GtkWidget* gtkStatusbar;
	GtkNotebook* gtkNotebook;
	GtkWidget* gtkToolbar;
	GSettings* gSettings;
	GQuark gQuark;
	int iCurrentUser;
	char*  sOpenFileName;
	PyObject* pyPylaxModule;
	PyObject* pySQLiteModule;
	PyTypeObject* pySQLiteConnectionType;
	PyObject* pyUserModule;
	PyObject* pyConnection;
	PyObject* pyPsycopg2Module;
	PyTypeObject* pyPsycopg2ConnectionType;
	PyObject* pyMysqlModule;
	PyTypeObject* pyMysqlConnectionType;
	bool bConnectionHasPxTables;
	PyObject* pyCopyFunction;
	PyObject* pyEnumType;
	PyObject* pyStdDateTimeFormat;
	PyObject* pyAlignEnum;
	PyObject* pyImageFormatEnum;
	//PxImageObject* pyIcon;
	PyObject* pyBeforeCloseCB;;
	GdkPixbuf* gdkPixbufPlaceHolder;
	EggMarkdown* pDownmarker;
}
PxGlobals;

extern PxGlobals g;

#endif
