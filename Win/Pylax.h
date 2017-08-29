// Pylax.h  |  Pylax © 2015 by Thomas Führinger
#ifndef PYLAX_H
#define PYLAX_H

// Ignore unreferenced parameters, since they are very common
// when implementing callbacks.
#pragma warning(disable : 4100)

#ifndef WINVER                  // Minimum platform is Windows XP
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT            // Minimum platform is Windows XP
#define _WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINDOWS          // Minimum platform is Windows XP
#define _WIN32_WINDOWS 0x0501
#endif

//#pragma once
#pragma execution_character_set("utf-8")

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define IDC_PXBUTTON	       7101
#define IDC_PXLABEL  	       7102
#define IDC_PXCOMBOBOX 	       7103
#define IDC_PXTABLE  	       7104
#define IDC_PXTABLECOL 	       7105
#define IDC_PXCANVAS  	       7106
#define IDC_PXENTRY  	       7107
#define IDC_PXIMAGEVIEW	       7108
#define IDC_PXBOX   	       7109
#define IDC_PXTOOLBAR  	       7110
#define IDC_PXTAB   	       7111

#define IDM_WINDOWMENUPOS      3
#define FIRST_CUSTOM_MENU_ID   700
#define MAX_CUSTOM_MENU_ID     750

#define WM_APP_TIMER (WM_APP + 1)

#include <Python.h>
#include <datetime.h>

// C RunTime Header Files
#include <stdlib.h>
//#include <malloc.h>
//#include <memory.h>
#include <tchar.h>
#include <Strsafe.h>

// Windows Header Files
#include <windows.h>
#include <Commctrl.h>
#include <OleCtl.h>
#include <Shlwapi.h>
#include <structmember.h>

// Pylax Classes
#include "Version.h"
#include "DynasetObject.h"
#include "ImageObject.h"
#include "MenuObject.h"
#include "WidgetObject.h"
#include "BoxObject.h"
#include "Utilities.h"
#include "WindowObject.h"
#include "FormObject.h"
//#include "SelectorObject.h"
#include "EntryObject.h"
#include "ComboBoxObject.h"
#include "ButtonObject.h"
#include "LabelObject.h"
#include "CanvasObject.h"
#include "ImageViewObject.h"
#include "SplitterObject.h"
#include "TabObject.h"
#include "TableObject.h"

#define PxCALL(fn)  if ((fn)==FALSE) {Xi( __FILE__, __LINE__);ShowPythonError();}
#define PxASSIGN(fn)  if ((fn)==NULL) {Xi( __FILE__, __LINE__);ShowPythonError();}

// If an error occurred, display the HRESULT and exit.
#ifndef EXIT_ON_ERROR
#define EXIT_ON_ERROR(hr) if (FAILED(hr)) { wchar_t error[255]; swprintf_s(error, 254, L"HRESULT = %x", hr); MessageBox(0, error, L"Error, exiting.", 0); exit(1);    }
#endif

#define PARSE_DECLTYPES 1 // from Python-3.4.2\Modules\_sqlite\module.h
#define PARSE_COLNAMES  2
#define TIMEOUT 5

//typedef struct _PxIconObject PxIconObject;
//typedef struct _PxMenuItemObject PxMenuItemObject;
typedef struct _PxGlobals
{
	HINSTANCE hInstance;
	TCHAR* szAppName;
	TCHAR szAppPath[MAX_PATH];
	HWND hWin; // nain window
	HICON hIcon;
	HMENU hMainMenu;
	HMENU hWindowsMenu;
	HMENU hAppMenu;
	HCURSOR hCursorWestEast;
	HCURSOR hCursorNorthSouth;
	PxMenuItemObject* pyMenuItem[MAX_CUSTOM_MENU_ID - FIRST_CUSTOM_MENU_ID + 1]; // map identifiers to pyMenuItems
	HGDIOBJ hfDefaultFont;
	HANDLE hHeap;
	BOOL bShowConsole;
	HWND hToolBar;
	HWND hStatusBar;
	HWND hMDIClientArea;
	int iCurrentUser;
	TCHAR* szWindowClass;
	TCHAR* szChildWindowClass;
	TCHAR szRecentFileNamePath[MAX_PATH];
	TCHAR szOpenFileNamePath[MAX_PATH];
	TCHAR szOpenFilePath[MAX_PATH];
	PTSTR  szOpenFileName;
	PyObject* pyPylaxModule;
	PyObject* pySQLiteModule;
	PyObject* pyUserModule;
	PyObject* pyConnection;
	BOOL bConnectionHasPxTables;
	PyObject* pyCopyFunction;
	PyObject* pyEnumType;
	PyObject* pyStdDateTimeFormat;
	PyObject* pyAlignEnum;
	PyObject* pyImageFormatEnum;
	PxImageObject* pyIcon;
	PyObject* pyBeforeCloseCB;
}
PxGlobals;

extern PxGlobals g;

PyTypeObject* pySQLiteConnectionType;

#endif
