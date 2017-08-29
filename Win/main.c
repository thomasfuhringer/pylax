// main.c  |  Pylax

#include <Pylax.h>
#include <Resource.h>
#include <fcntl.h>     /* for _O_TEXT and _O_BINARY */

#define IDM_FIRSTCHILD    50000 // used in structure when creating mdi client area for the main frame
#define MAX_CONSOLE_LINES 500   // maximum mumber of lines the output console should have

PxGlobals g;

PyMODINIT_FUNC PyInit_pylax(void);

// Forward declarations
static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK CloseEnumProc(HWND hwnd, LPARAM lParam);
static BOOL SaveState();
static BOOL RestoreState();
static BOOL CreateToolBar();
static BOOL CreateStatusBar();
static void RedirectIOToConsole();
static INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK Preferences(HWND, UINT, WPARAM, LPARAM);
static BOOL FileOpen();
static BOOL FileClose();
static BOOL OpenAppW(TCHAR* szFileNamePath);

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	g.hInstance = hInstance;
	g.szAppName = L"Pylax";
	g.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
	g.hfDefaultFont = GetStockObject(DEFAULT_GUI_FONT);
	g.hMainMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_BAR));
	g.hWindowsMenu = GetSubMenu(g.hMainMenu, IDM_WINDOWMENUPOS);
	g.hAppMenu = CreateMenu();
	g.hHeap = GetProcessHeap();
	g.szWindowClass = L"MainWindowClass";
	g.szChildWindowClass = L"ChildWindowClass";
	g.szOpenFileNamePath[0] = '\0';
	g.bShowConsole = FALSE;

	GetModuleFileNameW(NULL, g.szAppPath, MAX_PATH);
	PathRemoveFileSpecW(g.szAppPath);

	WNDCLASSEX wc;
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);


	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g.hInstance;
	wc.hIcon = g.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g.szWindowClass;
	wc.hIconSm = g.hIcon;

	if (!RegisterClassExW(&wc)) {
		ShowLastError(L"Main Window Class Registration Failed!");
		return FALSE;
	}

	InsertMenuW(g.hMainMenu, IDM_WINDOWMENUPOS + 1, MF_BYPOSITION | MF_POPUP | MF_GRAYED, (UINT_PTR)g.hAppMenu, L"App");

	g.hWin = CreateWindowExW(WS_EX_CONTROLPARENT, L"MainWindowClass", g.szAppName,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		NULL, g.hMainMenu, g.hInstance, NULL);

	if (g.hWin == NULL) {
		ShowLastError(L"Window Creation Failed!");
		return FALSE;
	}

	CreateToolBar();
	CreateStatusBar();

	if (!RestoreState())
		ShowWindow(g.hWin, SW_SHOW);
	UpdateWindow(g.hWin);

	g.hCursorWestEast = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
	g.hCursorNorthSouth = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZENS));

	//ShowInts("Xa", g.fShowConsole, 0);
	//if (!g.bShowConsole)

	RedirectIOToConsole();
	if (!g.bShowConsole)
		ShowWindow(GetConsoleWindow(), SW_HIDE);

	//SendMessage(g.hToolBar, TB_ENABLEBUTTON, (WPARAM)IDM_UNDO, FALSE);
	//_wputenv(L"PYTHONIOENCODING=cp1252:backslashreplace");
	Py_NoSiteFlag = 1;
	//Py_SetProgramName(L"Pylax");
	Py_SetProgramName(__wargv[0]);
	PyImport_AppendInittab("pylax", PyInit_pylax);
	Py_SetPythonHome(g.szAppPath); // Standard Libraries (.\Lib, .\\DLLs)
	Py_InitializeEx(0);
	OutputDebugStringW(L"\nPython bb initialized\n");

	/*
	__version__ = '0.1.0'

	if (FALSE){//g.bShowConsole) {
		if (AllocConsole())
			SetConsoleTitle(L"Pylax");

		PyObject* sys = PyImport_ImportModule("sys");
		//PyObject* pyStdout = PyFile_FromString("CONOUT$", "wt");
		FILE* fd;
		if (_wfopen_s(&fd, L"CONOUT$", L"wt"))
			ShowLastError(L"Can not open output file.");

		PyObject* pyStdout = PyFile_FromFd(fd, NULL, "wt", -1, NULL, NULL, NULL, 1);
		if (pyStdout == NULL)
			ShowLastError(L"Can not create output file object.");

		if (-1 == PyObject_SetAttrString(sys, "stdout", pyStdout)) {
			ShowLastError(L"Can not output to console!");
		}
		Py_DECREF(sys);
		Py_DECREF(pyStdout);
		/*
		PyRun_SimpleString("import sys\n");
		PyRun_SimpleString("sys.stdout = sys.stderr = open(\"CONOUT$\", \"w\")\n");
		* /
	}
	else {
		PyRun_SimpleString("import sys\n");
		PyRun_SimpleString("sys.stdout = sys.stderr = open(\"Pylax.log\", \"w\")\n");
	}
	*/

	g.pyStdDateTimeFormat = PyUnicode_FromString("{:%Y-%m-%d}");
	g.pyBeforeCloseCB = NULL;

	g.pySQLiteModule = PyImport_ImportModule("copy");
	if (g.pySQLiteModule == NULL) {
		ShowLastError(L"Can not import Python module 'copy'");
		return FALSE;
	}
	g.pyCopyFunction = PyObject_GetAttrString(g.pySQLiteModule, "copy");
	if (!g.pyCopyFunction) {
		ShowLastError(L"Can not load Python function 'copy()'");
		return FALSE;
	}

	g.pySQLiteModule = PyImport_ImportModule("enum");
	if (g.pySQLiteModule == NULL) {
		ShowLastError(L"Can not import Python module 'enum'");
		return FALSE;
	}
	g.pyEnumType = PyObject_GetAttrString(g.pySQLiteModule, "Enum");
	if (!g.pyEnumType) {
		ShowLastError(L"Can not load Python type 'Enum'");
		return FALSE;
	}

	//PyObject * pMainModule = PyImport_AddModule("__main__");
	g.pySQLiteModule = PyImport_ImportModule("sqlite3");
	if (g.pySQLiteModule == NULL) {
		ShowLastError(L"Can not import Python module sqlite3");
		return FALSE;
	}
	//PyModule_AddObject(pMainModule, "sqlite3", g.pySQLiteModule);

	pySQLiteConnectionType = (PyTypeObject*)PyObject_GetAttrString(g.pySQLiteModule, "Connection");
	if (pySQLiteConnectionType == NULL) {
		MessageBox(NULL, L"Cannot get SQLite Connection type", L"Error", MB_ICONERROR);
		return FALSE;
	}

	//pyCursorType = (PyTypeObject*)PyObject_GetAttrString(g.pySQLiteModule, "Cursor");
	//pyConnection_GetCursor = PyObject_GetAttrString(pyConnectionType, "cursor");
	//pyCursor_Execute = PyObject_GetAttrString(pyCursorType, "execute");
	//pyCursor_FetchOne = PyObject_GetAttrString(pyCursorType, "fetchone");
	//OpenAppW(L"E:\\C\\Pylax\\Data\\Example.pxa\\Example.px");


	/*
	if (PyRun_SimpleString("import sqlite3") == -1){
	ShowLastError(L"Can not import sqlite3.");
	return FALSE;
	}
	if (PyRun_SimpleString("sqlite3.register_adapter(pylax.Image, lambda x:x.bytes())") == -1){
	ShowLastError(L"Can not register adapter for Image.");
	return FALSE;
	}
	if (PyRun_SimpleString("sqlite3.register_converter('Image', pylax.Image)") == -1){
	ShowLastError(L"Can not register converter for Image.");
	return FALSE;
	}
	*/

	OutputDebugString(L"Python initialized");

	if (__argc == 2)
		OpenAppW(__wargv[1]);

	PxWidgetObject* pyWidget;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDS_APP));
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		pyWidget = (PxWidgetObject*)GetWindowLongPtrW(msg.hwnd, GWLP_USERDATA);
		if (pyWidget != NULL && IsDialogMessageW(pyWidget->pyWindow->hWin, &msg)) {
		}
		else if (!TranslateMDISysAccel(g.hMDIClientArea, &msg) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

static LRESULT CALLBACK
MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CLIENTCREATESTRUCT  MDIClientCreateStruct;
	HWND                hwndChild;
	int					iClientAreaTop;
	int					iClientAreaHeight;
	RECT				rcClient;
	WORD                wCommandIdentifier;

	switch (msg)
	{
	case WM_CREATE:
		MDIClientCreateStruct.hWindowMenu = g.hMainMenu; // NULL;
		MDIClientCreateStruct.idFirstChild = IDM_FIRSTCHILD;

		g.hMDIClientArea = CreateWindowExW(WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT, L"MDICLIENT", L"",
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,// | MDIS_ALLCHILDSTYLES,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hwnd, g.hMainMenu, g.hInstance, (void*)&MDIClientCreateStruct);

		if (g.hMDIClientArea == NULL) {
			ShowLastError(L"MDI Area Creation Failed!");
			return FALSE;
		}

		// set the Windows submenu
		SendMessage(g.hMDIClientArea, WM_MDISETMENU, (WPARAM)g.hMainMenu, (LPARAM)g.hWindowsMenu);
		DrawMenuBar(g.hWin);

		//HWND hParent = GetParent(g.hMDIClientArea);
		//ShowInts(L"a", g.hMDIClientArea, hParent);
		//PxWidgetObject* pyWidget = (PxWidgetObject*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		//SetWindowLongPtr(g.hMDIClientArea, GWLP_USERDATA, (LONG_PTR)pyWidget);
		return 0;

	case WM_COMMAND:
		//wmEvent = HIWORD(wParam);
		wCommandIdentifier = LOWORD(wParam);

		if (wCommandIdentifier >= FIRST_CUSTOM_MENU_ID && wCommandIdentifier <= MAX_CUSTOM_MENU_ID) {
			//ShowInts(L"X", wCommandIdentifier, 0);
			PxMenuItemObject* pyMenuItem = g.pyMenuItem[wCommandIdentifier - FIRST_CUSTOM_MENU_ID];
			if (pyMenuItem->pyOnClickCB) {
				PyObject* pyResult = PyObject_CallObject(pyMenuItem->pyOnClickCB, NULL);
				if (pyResult == NULL) {
					PyErr_Print();
					MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
				}
				Py_DECREF(pyResult);
			}
		}
		else
			switch (wCommandIdentifier)
			{
			case IDM_FILE_OPEN:
				FileOpen();
				return 0;

			case IDM_FILE_CLOSE:
				FileClose();
				return 0;

			case IDM_FILE_EXIT:
				SendMessage(hwnd, WM_CLOSE, 0, 0);
				return 0;

			case IDM_EDIT_PREFERENCES:
				DialogBox(g.hInstance, MAKEINTRESOURCE(IDD_PREFERENCES), hwnd, Preferences);
				break;

			case IDM_WINDOW_TILE:
				SendMessage(g.hMDIClientArea, WM_MDITILE, 0, 0);
				break;

			case IDM_WINDOW_ARRANGE:
				SendMessage(g.hMDIClientArea, WM_MDIICONARRANGE, 0, 0);
				break;

			case IDM_WINDOW_CASCADE:
				SendMessage(g.hMDIClientArea, WM_MDICASCADE, 0, 0);
				break;

			case IDM_WINDOW_CLOSEALL:     // Attempt to close all children
				EnumChildWindows(g.hMDIClientArea, CloseEnumProc, 0);
				return 0;

			case IDM_HELP_ABOUT:
				DialogBox(g.hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
				break;

			case IDM_FILE_NEW:
			case ID_EDIT_CUT:
			case ID_EDIT_COPY:
			case ID_EDIT_PASTE:
			case ID_FORM_PROPERTIES:
				MessageBox(NULL, L"Not yet implemented", L"Menu Command", MB_ICONINFORMATION);
				return 0;

			default:             // Pass to active child...
				hwndChild = (HWND)SendMessage(g.hMDIClientArea, WM_MDIGETACTIVE, 0, 0);

				if (IsWindow(hwndChild))
					SendMessage(hwndChild, WM_COMMAND, wParam, lParam);
				break;        // ...and then to DefFrameProc
			}
		break;

	case WM_MENUSELECT:
		switch (LOWORD(wParam))
		{
		case IDM_FILE_EXIT:
			SendMessage(g.hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Exit application");
			break;
		default:
			SendMessage(g.hStatusBar, SB_SETTEXT, 0, 0);
		}
		break;

	case WM_SIZE:
		GetClientRect(g.hWin, &rcClient);

		int iPart[3] = { rcClient.right - 200, rcClient.right - 100, -1 };
		SendMessage(g.hStatusBar, SB_SETPARTS, (WPARAM)3, (LPARAM)iPart);
		SendMessage(g.hStatusBar, WM_SIZE, 0, 0);
		SendMessage(g.hToolBar, TB_AUTOSIZE, 0, 0);

		iClientAreaHeight = rcClient.bottom;

		RECT rect;
		GetWindowRect(g.hToolBar, &rect);
		iClientAreaTop = rect.bottom - rect.top;
		iClientAreaHeight -= iClientAreaTop;
		GetClientRect(g.hStatusBar, &rect);
		iClientAreaHeight -= rect.bottom;

		MoveWindow(g.hMDIClientArea, 0,
			iClientAreaTop,
			rcClient.right, //width
			iClientAreaHeight, //height
			TRUE);
		return 0;

	case WM_QUERYENDSESSION:
	case WM_CLOSE:
		if (g.szOpenFileName == NULL || FileClose()) {
			Py_Finalize();
			SaveState();
			//DestroyWindow(hwnd);
			break;
		}
		else
			return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefFrameProcW(hwnd, g.hMDIClientArea, msg, wParam, lParam);
}

static BOOL CALLBACK
CloseEnumProc(HWND hwnd, LPARAM lParam)
{
	if (GetWindow(hwnd, GW_OWNER))         // Check for icon title
		return TRUE;

	SendMessage(GetParent(hwnd), WM_MDIRESTORE, (WPARAM)hwnd, 0);

	if (!SendMessage(hwnd, WM_QUERYENDSESSION, 0, 0))
		return TRUE;

	SendMessage(GetParent(hwnd), WM_MDIDESTROY, (WPARAM)hwnd, 0);
	return TRUE;
}

static BOOL
SaveState()
{
	HKEY hKey;
	WINDOWPLACEMENT wp;
	TCHAR szSubkey[256];
	lstrcpy(szSubkey, L"Software\\");
	lstrcat(szSubkey, g.szAppName);
	if (RegCreateKeyEx(HKEY_CURRENT_USER, szSubkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
		ShowLastError(L"Creating Registry Key");
		return FALSE;
	}

	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(g.hWin, &wp);

	if ((RegSetValueEx(hKey, L"flags", 0, REG_BINARY, (PBYTE)&wp.flags, sizeof(wp.flags)) != ERROR_SUCCESS) ||
		(RegSetValueEx(hKey, L"showCmd", 0, REG_BINARY, (PBYTE)&wp.showCmd, sizeof(wp.showCmd)) != ERROR_SUCCESS) ||
		(RegSetValueEx(hKey, L"rcNormalPosition", 0, REG_BINARY, (PBYTE)&wp.rcNormalPosition, sizeof(wp.rcNormalPosition)) != ERROR_SUCCESS) ||
		(RegSetValueEx(hKey, L"bShowConsole", 0, REG_BINARY, (PBYTE)&g.bShowConsole, sizeof(g.bShowConsole)) != ERROR_SUCCESS))
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	RegCloseKey(hKey);
	return TRUE;
}

static BOOL
RestoreState()
{
	HKEY hKey;
	DWORD dwSizeFlags, dwSizeShowCmd, dwSizeRcNormal, dwBShowConsole;
	WINDOWPLACEMENT wp;
	TCHAR szSubkey[256];

	lstrcpy(szSubkey, L"Software\\");
	lstrcat(szSubkey, g.szAppName);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, szSubkey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		wp.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(g.hWin, &wp);
		dwSizeFlags = sizeof(wp.flags);
		dwSizeShowCmd = sizeof(wp.showCmd);
		dwSizeRcNormal = sizeof(wp.rcNormalPosition);
		dwBShowConsole = sizeof(g.bShowConsole);
		if ((RegQueryValueEx(hKey, L"flags", NULL, NULL,
			(PBYTE)&wp.flags, &dwSizeFlags) != ERROR_SUCCESS) ||
			(RegQueryValueEx(hKey, L"showCmd", NULL, NULL,
				(PBYTE)&wp.showCmd, &dwSizeShowCmd) != ERROR_SUCCESS) ||
			(RegQueryValueEx(hKey, L"rcNormalPosition", NULL, NULL,
				(PBYTE)&wp.rcNormalPosition, &dwSizeRcNormal) != ERROR_SUCCESS) ||
			(RegQueryValueEx(hKey, L"bShowConsole", NULL, NULL,
				(PBYTE)&g.bShowConsole, &dwBShowConsole) != ERROR_SUCCESS))
		{
			RegCloseKey(hKey);
			return FALSE;
		}
		RegCloseKey(hKey);
		if ((wp.rcNormalPosition.left <=
			(GetSystemMetrics(SM_CXSCREEN) - GetSystemMetrics(SM_CXICON))) &&
			(wp.rcNormalPosition.top <= (GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYICON))))
		{
			SetWindowPlacement(g.hWin, &wp);
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL
CreateToolBar()
{
	const int iImageListID = 0;
	const int iButtons = 1;// 6;
	const int iBitmapSize = 16;

	const DWORD buttonStyles = BTNS_AUTOSIZE;
	HIMAGELIST hImageList = NULL;

	HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_LIST | TBSTYLE_FLAT, 0, 0, 0, 0,
		g.hWin, NULL, g.hInstance, NULL);

	if (hWndToolbar == NULL) {
		ShowLastError(L"Toolbar Creation Failed!");
		return FALSE;
	}
	hImageList = ImageList_Create(iBitmapSize, iBitmapSize, ILC_COLOR16 | ILC_MASK, iButtons, 0);

	SendMessage(hWndToolbar, TB_SETIMAGELIST, (WPARAM)iImageListID, (LPARAM)hImageList);
	SendMessage(hWndToolbar, TB_LOADIMAGES, (WPARAM)IDB_STD_SMALL_COLOR, (LPARAM)HINST_COMMCTRL);
	SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	TBBUTTON tbButtons[2] =
	{
		{ MAKELONG(STD_FILEOPEN, iImageListID), IDM_FILE_OPEN, TBSTATE_ENABLED, buttonStyles, { 0 }, 0, (INT_PTR)TEXT("Open") },
		{ 0, 0, 0, TBSTYLE_SEP, { 0 }, 0, 0 }/*,
		{ MAKELONG(STD_FILENEW, iImageListID), IDM_NEW, TBSTATE_ENABLED, buttonStyles, { 0 }, 0, (INT_PTR)L"New" },
		{ MAKELONG(STD_DELETE, iImageListID), IDM_DELETE, TBSTATE_ENABLED, buttonStyles, { 0 }, 0, (INT_PTR)TEXT("Delete") },
		{ MAKELONG(STD_UNDO, iImageListID), IDM_UNDO, TBSTATE_ENABLED, buttonStyles, { 0 }, 0, (INT_PTR)TEXT("Undo") },
		{ MAKELONG(STD_FILESAVE, iImageListID), IDM_SAVE, TBSTATE_ENABLED, buttonStyles, { 0 }, 0, (INT_PTR)TEXT("Save") }//,*/
	};

	SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)iButtons, (LPARAM)&tbButtons);

	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
	SendMessage(hWndToolbar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_MIXEDBUTTONS);
	ShowWindow(hWndToolbar, TRUE);
	g.hToolBar = hWndToolbar;
	return TRUE;
}

static BOOL
CreateStatusBar()
{
	g.hStatusBar = CreateWindowEx(0, STATUSCLASSNAME, L"",
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		-100, -100, 10, 10,
		g.hWin, NULL, g.hInstance, NULL);

	int iPart[3] = { 200, 250, -1 };
	SendMessage(g.hStatusBar, SB_SETPARTS, (WPARAM)3, (LPARAM)iPart);
	return TRUE;
}


static INT_PTR CALLBACK
Preferences(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_SHOWCONSOLE, g.bShowConsole ? BST_CHECKED : BST_UNCHECKED);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			g.bShowConsole = (IsDlgButtonChecked(hDlg, IDC_SHOWCONSOLE) == BST_CHECKED) ? TRUE : FALSE;
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

static INT_PTR CALLBACK
About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	static HFONT hFont;

	switch (message)
	{
	case WM_INITDIALOG:
		hFont = CreateFontW(24, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
		SendDlgItemMessage(hDlg, IDC_ABOUTNAME, WM_SETFONT, (WPARAM)hFont, TRUE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			DeleteObject(hFont);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

static BOOL
FileOpen()
{
	static wchar_t szFilter[] = L"Pylax Files (*.PX)\0*.px\0"  \
		L"All Files (*.*)\0*.*\0\0";
	static wchar_t szOpenFileNamePath[MAX_PATH];
	static OPENFILENAME ofn;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g.hWin;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = szOpenFileNamePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL; //g.szOpenFileName;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL; //L"Open Ledger"
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; //OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = L"px";
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	OutputDebugString(L"\nGot open ");
	//MessageBox(g.hWin, szFilter, L"szFilter", MB_OK);
	if (GetOpenFileNameW(&ofn)) {
		//MessageBox(g.hWin, g.szOpenFileNamePath, g.szOpenFileName, MB_OK);
		return OpenAppW(szOpenFileNamePath);
	}
	return TRUE;
}

static BOOL
FileClose()
{
	if (g.pyBeforeCloseCB) {
		PyObject* r = PyObject_CallObject(g.pyBeforeCloseCB, NULL);
		if (r == NULL) {
			ShowPythonError();
			return FALSE;
		}
		int iR = (r == Py_False ? FALSE : TRUE);
		Py_XDECREF(r);
		if (!iR)
			return FALSE;
	}

	HMENU hFileMenu = GetSubMenu(g.hMainMenu, 0);

	SendMessage(g.hWin, WM_COMMAND, IDM_WINDOW_CLOSEALL, 0);
	if (GetWindow(g.hMDIClientArea, GW_CHILD) != NULL)
		return FALSE;

	PyObject* pyResult = NULL;
	if ((pyResult = PyObject_CallMethod(g.pyConnection, "close", NULL)) == NULL) {
		PyErr_Print();
		MessageBox(NULL, L"Can not close the database.", L"Error", MB_ICONERROR);
		return FALSE;
	}
	Py_DECREF(pyResult);

	// TO DO: get rid of all imported stuff

	if (SendMessage(g.hWin, WM_SETTEXT, 0, (LPARAM)g.szAppName)) {
		ShowLastError(L"Cannot reset the window title.");
		return FALSE;
	}
	g.szOpenFileName = NULL;

	EnableMenuItem(hFileMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hFileMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hFileMenu, IDM_FILE_CLOSE, MF_BYCOMMAND | MF_GRAYED);
	SendMessage(g.hToolBar, TB_ENABLEBUTTON, (WPARAM)IDM_FILE_OPEN, TRUE);

	PxMenuItems_DeleteAll();
	return TRUE;
}

static BOOL
OpenAppW(TCHAR* szFileNamePath)
{
	HMENU hFileMenu = GetSubMenu(g.hMainMenu, 0);

	lstrcpy(g.szOpenFileNamePath, szFileNamePath);
	lstrcpy(g.szOpenFilePath, szFileNamePath);
	g.szOpenFileName = PathFindFileName(g.szOpenFileNamePath);
	PathRemoveFileSpec(g.szOpenFilePath);

	LPTSTR szModulePathOld = Py_GetPath();
	LPTSTR pth[3] = { szModulePathOld, L";", g.szOpenFilePath };
	LPTSTR szModulePath = StringArrayCat(pth, 3);
	PySys_SetPath(szModulePath);
	SetCurrentDirectory(g.szOpenFilePath); //++
	//HFree(szModulePath);
	PyMem_RawFree(szModulePath);

	PyObject* pFunc = PyObject_GetAttrString(g.pySQLiteModule, "connect");
	//PyObject* pyArgList = Py_BuildValue("(s,i)", ":memory:", PARSE_DECLTYPES|PARSE_COLNAMES);

	LPCSTR strText = toU8(g.szOpenFileNamePath);
	//PyObject* pyArgList = Py_BuildValue("(s)", strText);
	PyObject* pyArgList = Py_BuildValue("(sii)", strText, TIMEOUT, PARSE_DECLTYPES | PARSE_COLNAMES); // timeout, detect_types
	//HFree(strText);
	PyMem_RawFree(strText);
	g.pyConnection = PyObject_CallObject(pFunc, pyArgList);
	//g.pyConnection = (PyObject*)PyObject_CallObject((PyObject*)pySQLiteConnectionType, pyArgList);
	if (g.pyConnection == NULL) {
		PyErr_PrintEx(1);
		MessageBox(NULL, L"Can not connect to database.", L"Error", MB_ICONERROR);
		return FALSE;
	}
	OutputDebugString(L"** Connection established\n");

	g.iCurrentUser = 0;

	// Check if Px tables exist.
	PyObject* pyCursor = NULL, *pyResult = NULL;
	if ((pyCursor = PyObject_CallMethod(g.pyConnection, "cursor", NULL)) == NULL) {
		MessageBox(NULL, L"Can not access database (cursor creation failed).", L"Error", MB_ICONERROR);
		return FALSE;
	}

	if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(s)", "SELECT name FROM sqlite_master WHERE type='table' AND name='PxWindow';")) == NULL) {
		MessageBox(NULL, L"Can not access database (execution failed).", L"Error", MB_ICONERROR);
		return FALSE;
	}
	Py_DECREF(pyResult);
	if ((pyResult = PyObject_CallMethod(pyCursor, "fetchone", NULL)) == NULL) {
		MessageBox(NULL, L"Can not access database (fetch failed).", L"Error", MB_ICONERROR);
		return FALSE;
	}
	g.bConnectionHasPxTables = pyResult == Py_None ? FALSE : TRUE;

	if (pyCursor == NULL || PyObject_CallMethod(pyCursor, "close", NULL) == NULL) {
		MessageBox(NULL, L"Can not access database (cursor close failed).", L"Error", MB_ICONERROR);
		return FALSE;
	}
	Py_DECREF(pyCursor);
	Py_DECREF(pyResult);
	//MessageBox(NULL, g.bConnectionHasPxTables ? L"Tables exist." : L"Tables missing.", L"Cnx", MB_ICONERROR);

	/*
	if (PyModule_AddObject(g.pyPylaxModule, "cnx", g.pyConnection) != 0) {
	PyErr_PrintEx(1);
	MessageBox(NULL, L"Can not add connection.", L"Python Error", MB_ICONERROR);
	return NULL;
	};

	RunScript("E:\\C\\Pylax\\Data\\Example.pxa\\script.py");// g.szOpenFileNamePath);
	*/

	OutputDebugString(L"main.py starting -\n");
	PyObject* pyUserModule = PyImport_ImportModule("main");
	if (pyUserModule == NULL) {
		PyErr_PrintEx(1);
		//MessageBox(NULL, L"Error in Python script", L"Error", MB_ICONERROR);
		//ShowPythonError();
		return FALSE;
	}

	LPTSTR arr[3] = { g.szOpenFileName, L" - ", g.szAppName };
	LPTSTR szTitle = StringArrayCat(arr, 3);
	if (SendMessage(g.hWin, WM_SETTEXT, 0, (LPARAM)szTitle)) {
		ShowLastError(L"Cannot set the window title.");
		return FALSE;
	}
	//HFree(szTitle);
	PyMem_RawFree(szTitle);

	EnableMenuItem(hFileMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hFileMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hFileMenu, IDM_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	SendMessage(g.hToolBar, TB_ENABLEBUTTON, (WPARAM)IDM_FILE_OPEN, FALSE);

	return TRUE;
}

BOOL
StatusBarMessageW(const LPWSTR szMessage)
{
	return SendMessage(g.hStatusBar, SB_SETTEXT, 0, (LPARAM)szMessage);
}

BOOL
StatusBarMessageU(const LPCSTR strMessage)
{
	LPWSTR szMessage = toW(strMessage);
	LRESULT result = StatusBarMessageW(szMessage);
	//HFree(szMessage);
	PyMem_RawFree(szMessage);
	return result;
}

BOOL
MessageU(const LPCSTR strMessage, const LPCSTR strTitle, UINT uType)
{
	LPWSTR szMessage = toW(strMessage);
	LPWSTR szTitle = toW(strTitle);
	int result = MessageBox(g.hWin, szMessage, szTitle, uType);
	//HFree(szMessage);
	//HFree(szTitle);
	PyMem_RawFree(szMessage);
	PyMem_RawFree(szTitle);
	return result;
}

/*
BOOL
RunScript(const LPWSTR szName)
{
FILE *pFile;
pFile = _wfopen(szName, L"r");
if (!pFile) {
ShowLastError(L"Error opening file");
return FALSE;
}

PyRun_SimpleFile(pFile, szName);
if (PyErr_Occurred()) {
PyErr_Print();
fprintf(stderr, "Cannot open script");
return FALSE;
}

if (fclose(pFile))
{
ShowLastError(L"Error closing file");
return FALSE;
}
return TRUE;
}

static BOOL
RecentlyUsedFile()
{
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa365200%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
//     SHAddToRecentDocs()
PWSTR pPath;
WIN32_FIND_DATA ffd;
TCHAR szDir[MAX_PATH];

if (SHGetKnownFolderPath(&FOLDERID_Recent, 0, NULL, &pPath) == S_OK) {
StringCchCopy(szDir, MAX_PATH, pPath);
StringCchCat(szDir, MAX_PATH, TEXT("\\*.px"));
CoTaskMemFree(pPath);

if (FindFirstFile(szDir, &ffd) == INVALID_HANDLE_VALUE) {
//MessageBox(g.hWin, szDir, L"szFileNamePath", MB_OK);
ShowLastError(L"Cannot find last file");
return FALSE;
}
StringCchCopy(g.szRecentFileNamePath, MAX_PATH, ffd.cFileName);
return TRUE;
}
else
return FALSE;
}

*/

static void
RedirectIOToConsole()
{
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console

	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen(hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);
	// redirect unbuffered STDERR to the console

	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");

	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);
}
// ---- module functions -----------------------------------------------------

PyObject*
Pylax_message(PyObject *self, PyObject *args)
{
	const char *strMessage, *strTitle = NULL;

	if (!PyArg_ParseTuple(args, "s|s", &strMessage, &strTitle))
		return NULL;

	if (strTitle == NULL)
		strTitle = "Information";

	MessageU(strMessage, strTitle, MB_ICONINFORMATION);
	return PyLong_FromLong(1);
}

PyObject*
Pylax_ask(PyObject *self, PyObject *args, PyObject *kwds)
{
	const char *strMessage, *strTitle = NULL;
	BOOL bNo = FALSE, bCancel = FALSE;
	int iButtons, iAnswer;
	static char *kwlist[] = { "message", "caption", "no", "cancel", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|spp", kwlist,
		&strMessage,
		&strTitle,
		&bNo,
		&bCancel))
		return NULL;

	if (bNo && bCancel)
		iButtons = MB_YESNOCANCEL;
	else if (bNo)
		iButtons = MB_YESNO;
	else if (bCancel)
		iButtons = MB_OKCANCEL;
	else
		iButtons = MB_OK;

	if (strTitle == NULL)
		strTitle = "Question";

	LPWSTR szMessage = toW(strMessage);
	LPWSTR szTitle = toW(strTitle);
	iAnswer = MessageBoxW(g.hWin, szMessage, szTitle, iButtons | MB_ICONQUESTION);
	PyMem_RawFree(szMessage);
	PyMem_RawFree(szTitle);

	if (iAnswer == IDOK || iAnswer == IDYES)
		Py_RETURN_TRUE;
	if (iAnswer == IDNO)
		Py_RETURN_FALSE;
	Py_RETURN_NONE;
}

PyObject*
Pylax_status_message(PyObject *self, PyObject *args)
{
	const char* strMessage;

	if (!PyArg_ParseTuple(args, "s", &strMessage))
		return NULL;

	StatusBarMessageU(strMessage);
	return PyLong_FromLong(1);
}

PyObject*
Pylax_set_icon(PyObject *self, PyObject* args)
{
	PyObject *pyIcon = NULL;

	if (!PyArg_ParseTuple(args, "O", &pyIcon))
		return NULL;

	if (pyIcon == Py_None) {
		Py_XDECREF(g.pyIcon);
		Py_INCREF(Py_None);
		g.pyIcon = (PxImageObject*)Py_None;
		SendMessage(g.hWin, WM_SETICON, ICON_SMALL, (LPARAM)g.hIcon);
		SendMessage(g.hWin, WM_SETICON, ICON_BIG, (LPARAM)g.hIcon);
	}
	else if (PyObject_TypeCheck(pyIcon, &PxImageType)) {
		Py_XDECREF(g.pyIcon);
		Py_INCREF(pyIcon);
		g.pyIcon = (PxImageObject*)pyIcon;
		SendMessage(g.hWin, WM_SETICON, ICON_SMALL, (LPARAM)((PxWidgetObject*)pyIcon)->hWin);
		SendMessage(g.hWin, WM_SETICON, ICON_BIG, (LPARAM)((PxWidgetObject*)pyIcon)->hWin);
	}
	else {
		PyErr_SetString(PyExc_TypeError, "Parameter must be a Icon.");
		return NULL;
	}
	Py_RETURN_NONE;
}
/*
VOID CALLBACK
TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
if (lpParam == NULL) {
PyErr_SetString(PyExc_RuntimeError, "Timer fired without callback.");
}
else {
//  WM_APP_TIMER
PyObject* pyResult = PyObject_CallObject((PyObject*)lpParam, NULL);
if (pyResult == NULL)
ShowPythonError();
Py_XDECREF(pyResult);
}
}

PyObject*
Pylax_create_timer(PyObject *self, PyObject *args, PyObject *kwds)
{
HANDLE hTimer = NULL;
int iInterval = 0, iWait = 0;
PyObject *pyCallback = NULL,*pyWindow = NULL;
static char *kwlist[] = { "window", "callback", "interval", "wait", NULL };

if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|ii", kwlist,
&pyWindow,
&pyCallback,
&iInterval,
&iWait))
return NULL;

if (!PyCallable_Check(pyCallback)) {
PyErr_SetString(PyExc_TypeError, "Parameter 2 ('callback') must be callable.");
return NULL;
}

Py_INCREF(pyCallback);
if (!CreateTimerQueueTimer(&hTimer, NULL, (WAITORTIMERCALLBACK)TimerRoutine, pyCallback, iWait, iInterval, WT_EXECUTEDEFAULT)) {
PyErr_SetFromWindowsErr(0);
ShowPythonError();
return NULL;
}

return PyLong_FromLong(hTimer);
}

PyObject*
Pylax_delete_timer(PyObject *self, PyObject *args, PyObject *kwds)
{
HANDLE hTimer = NULL;
static char *kwlist[] = { "timer", NULL };

if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist,
&hTimer))
return NULL;

if (DeleteTimerQueueTimer(NULL, hTimer, NULL) == 0) {
PyErr_SetFromWindowsErr(0);
ShowPythonError();
return NULL;
}

/ *if (!CloseHandle(hTimer)) {
PyErr_SetFromWindowsErr(0);
ShowPythonError();
return NULL;
}* /

Py_RETURN_NONE;
}*/

PyObject*
Pylax_append_menu_item(PyObject *self, PyObject *args)
{
	PyObject *pyMenuItem = NULL;
	if (!PyArg_ParseTuple(args, "O", &pyMenuItem))
		return NULL;

	if (PyObject_TypeCheck(pyMenuItem, &PxMenuItemType)) {
		if (((PxMenuItemObject*)pyMenuItem)->iIdentifier == FIRST_CUSTOM_MENU_ID) {
			EnableMenuItem(g.hMainMenu, IDM_WINDOWMENUPOS + 1, MF_BYPOSITION | MF_ENABLED);
			DrawMenuBar(g.hWin);
		}
		AppendMenuW(g.hAppMenu, MF_STRING, ((PxMenuItemObject*)pyMenuItem)->iIdentifier, ((PxMenuItemObject*)pyMenuItem)->szCaption);
	}
	else {
		PyErr_SetString(PyExc_TypeError, "Parameter must be a MenuItem.");
		return NULL;
	}
	Py_RETURN_NONE;
}

PyObject*
Pylax_set_before_close(PyObject *self, PyObject *args)
{
	PyObject *pyBeforeCloseCB = NULL;

	if (!PyArg_ParseTuple(args, "O", &pyBeforeCloseCB))
		return NULL;

	if (PyCallable_Check(pyBeforeCloseCB)) {
		Py_XINCREF(pyBeforeCloseCB);
		Py_XDECREF(g.pyBeforeCloseCB);
		g.pyBeforeCloseCB = pyBeforeCloseCB;
		Py_RETURN_NONE;
	}
	else {
		PyErr_SetString(PyExc_TypeError, "Parameter must be callable");
		return NULL;
	}
}