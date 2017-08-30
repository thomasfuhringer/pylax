// Wanderlust.h  |  Wanderlust © 2017 by Thomas Führinger
#ifndef WANDERLUST_H
#define WANDERLUST_H

#include <Python.h>
#include "structmember.h"
#include "egg-markdown.h"

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

// GTK Header Files
#include <gtk/gtk.h>

// Wanderlust modules
#include "Version.h"

#define PARSE_DECLTYPES 1 // from Python-3.4.2\Modules\_sqlite\module.h
#define PARSE_COLNAMES  2
#define TIMEOUT 5

#define PAGE_PICTURE_WIDTH   320
#define PAGE_PICTURE_HEIGHT  320

#define DEFAULT_SITE    "45.76.133.182"
//#define DEFAULT_SITE    "localhost"

typedef struct _ClientSession
{
	GtkNotebook* gtkNotebook;
	GtkWidget* gtkEntrySite;
	GtkWidget* gtkLabelSite;
	GtkEntry* gtkEntryPageAddress;
	GtkLabel* gtkLabelPageHeading;
	GtkTextBuffer* gtkTextBufferPageText;
	GtkImage* gtkImagePageImage;
	GtkTreeView* gtkTreeKeyWord;
	GtkTreeStore* gtkTreeStoreOrg;
	GtkLabel* gtkLabelPagePerson;
	GtkTreeStore* gtkTreeStoreKeyWord;
	GtkTreeView* gtkTreeViewMessages;
	GtkListStore* gtkListStoreMessages;
	GtkLabel* gtkLabelMessageHeading;
	GtkTextBuffer* gtkTextBufferMessageText;
	gint iIndex;
	gint iPageOrg;
	gint iPagePerson;
	gint iPage;
	gint iPageParent;
	gchar* sKeyWord;
	gint iLanguage;
	PyObject* pyUserID;
	gchar* sCurrentUserHandle;
	PyObject* pySession;
	PyObject* pyResultMsg;
	char* sStatusMessage;
}
ClientSession;

typedef struct _Globals
{
	GtkApplication* gtkApp;
	GtkWindow* gtkMainWindow;
	GtkWidget* gtkStatusbar;
	GtkNotebook* gtkNotebook;
	GtkWidget* gtkToolbar;
	GtkWidget* gtkImageClose;
	GtkWidget* gtkImageRefresh;
	GtkWidget* gtkImageFind;
	GtkWidget* gtkImagePerson;
	GdkPixbuf* gdkPixbufKey;
	GdkPixbuf* gdkPixbufPlaceHolder;
	GtkEntry* gtkEntrySite;
	GtkToolItem* gtkToolItemEncrypted;
	GtkWidget* gtkSiteMenuItem;
	GtkWidget* gtkSignOnMenuItem;
	GtkWidget* gtkLabelUserName;
	GtkAction* gtkActionLogIn;
	GtkAction* gtkActionRefreshMessages;
	GSettings* gSettings;
	GQuark gQuark;
	EggMarkdown* pDownmarker;
	//GdkPixbufLoader* gdkPixbufLoader;
	ClientSession* pCurrentSession;

	PyObject* pyNaclModule;
	PyObject* pyHinterlandModule;
	PyObject* pyHinterlandClientType;
	PyObject* pyStdDateTimeFormat;
	PyObject* pyMsg;
}
Globals;
extern Globals g;

typedef struct _HinterlandMsg
{
	PyObject* Type;
	PyObject* Disconnect;
	PyObject* SetPage;
	PyObject* GetPage;
	PyObject* GetPageChildren;
	PyObject* GetOrgChildren;
	PyObject* DeletePage;
	PyObject* SearchPerson;
	PyObject* GetMessageList;
	PyObject* NotFound;
}
HinterlandMsg;
extern HinterlandMsg hlMsg;

void ErrorDialog(char* sMessage);
void MessageDialog(char* sMessage);
void PythonErrorDialog(char* sMessage);
void gtk_text_buffer_set_markup(GtkTextBuffer* gtkTextBuffer, char* sMarkup);
PyObject* PyDict_GetItemStringX(PyObject* pyDict, char* sKey);
char* PyUnicode_AsString(PyObject* pyString);
PyObject* Session_Send(ClientSession* pSession);
gboolean Session_Exchange(ClientSession* pSession);
gboolean Session_PullPage(ClientSession* pSession, int iPage);
gboolean Session_PullHomePage(ClientSession* pSession);
PyObject* Session_GetMessageAttribute(ClientSession* pSession, char* sKey);
gboolean SelectPerson(ClientSession* pSession, int* piPerson, char** psFirstName, char** psLastName);
void ActionMessagesRefreshCB(GtkAction* gtkAction, gpointer pUserData);
void GtkWidget_SetBorder(GtkWidget* gtkWidget, int iBorder);
void y(char* sText, PyObject* pyObject);   // for debugging purposes

#endif
