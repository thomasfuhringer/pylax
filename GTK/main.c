// main.c  | Pylax © 2017 by Thomas Führinger

#include "Pylax.h"

PxGlobals g;   // global singleton
PxHinterlandGlobals hl;
static bool OpenApp(char* sFileNamePath);

PyMODINIT_FUNC PyInit_pylax(void);

static void
ActionFileOpenCB(GtkAction* action, gpointer gUserData)
{
	GtkWidget* gtkFileChooser;
	gint iResponse;
	gtkFileChooser = gtk_file_chooser_dialog_new("Open Ledger", g.gtkMainWindow, GTK_FILE_CHOOSER_ACTION_OPEN,
		"_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

	GtkFileFilter* gtkFileFilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gtkFileFilter, "*.px");
	gtk_file_filter_set_name(gtkFileFilter, "Pylax Ledger");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkFileChooser), gtkFileFilter);

	iResponse = gtk_dialog_run(GTK_DIALOG(gtkFileChooser));
	if (iResponse == GTK_RESPONSE_ACCEPT) {
		char* sFileName;
		sFileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkFileChooser));
		OpenApp(sFileName);
		g_free(sFileName);
	}

	gtk_widget_destroy(gtkFileChooser);
	g_object_unref(gtkFileFilter);
}

static void
ActionFileCloseCB(GtkAction* action, gpointer gUserData)
{
	g_debug("Close File");
	GtkWidget* gtkPage;
	PxWidgetObject* pyForm;
	gint nPage, nPages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(g.gtkNotebook));
	for (nPage = 0; nPage < nPages; nPage++) {
		gtkPage = gtk_notebook_get_nth_page(GTK_NOTEBOOK(g.gtkNotebook), nPage);
		pyForm = g_object_get_qdata(gtkPage, g.gQuark);
		if (pyForm)
			PxForm_Close(pyForm);
	}

	Py_Finalize();
	/*if(Py_FinalizeEx()==-1){
		g_debug("Unloading of Python interpreter failed.");
		PythonErrorDialog();
		return;
	}*/
	gtk_action_set_sensitive(GTK_ACTION(g.gtkActionFileOpen), TRUE);
	gtk_action_set_sensitive(GTK_ACTION(g.gtkActionFileClose), FALSE);
}

static void
ActionFileQuitCB(GtkAction* action, gpointer gUserData)
{
	g.gtkMainWindow = gtk_application_get_active_window(g.gtkApp);
	gtk_window_close(g.gtkMainWindow);
}

static void
ActionHelpAboutCB(GtkAction* action, gpointer gUserData)
{
	GStrv sAuthors[] = AUTHORS;
	gtk_show_about_dialog(g.gtkMainWindow,
		"program-name", "Pylax",
		"comments", "Build date:  " __DATE__ "\n\nDatabase front end for SQLite, PostgreSQL and MySQL",
		"version", VER_PRODUCTVERSION_STR,
		"authors", sAuthors,
		"website", "https://github.com/thomasfuhringer/pylax",
		NULL);
}

static gboolean
GtkMainWindow_DeleteEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	gint x, y, width, height;

	gtk_window_get_position(g.gtkMainWindow, &x, &y);
	gtk_window_get_size(g.gtkMainWindow, &width, &height);

	g_settings_set(g.gSettings, "main-window-x", "i", x);
	g_settings_set(g.gSettings, "main-window-y", "i", y);
	g_settings_set(g.gSettings, "main-window-width", "i", width);
	g_settings_set(g.gSettings, "main-window-height", "i", height);

	return FALSE;
}

static gboolean
GtkMainWindow_ConfigureEventCB(GtkWidget* gdkWidget, GdkEvent* gdkEvent, gpointer gUserData)
{
	GtkWidget* gtkPage;
	PxWidgetObject* pyForm;
	gint nPage, nPages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(g.gtkNotebook));
	for (nPage = 0; nPage < nPages; nPage++) {
		gtkPage = gtk_notebook_get_nth_page(GTK_NOTEBOOK(g.gtkNotebook), nPage);
		pyForm = g_object_get_qdata(gtkPage, g.gQuark);
		if (pyForm)
			if (!PxWidget_RepositionChildren(pyForm)) {
				PythonErrorDialog();
				return TRUE;
			}
	}
	return FALSE;
}

static void
GtkAppActivateEventCB(GtkApplication* app, gpointer gUserData)
{
	GtkWidget* gtkBox;
	gint x, y, width, height;

	g.gtkMainWindow = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(g.gtkMainWindow), "Pylax");
	GdkPixbuf* gdkPixbufIcon = gdk_pixbuf_new_from_file("App.ico", NULL);
	//#include "Icon.c"
	gtk_window_set_default_icon(gdkPixbufIcon);
	g_signal_connect(G_OBJECT(g.gtkMainWindow), "delete-event", G_CALLBACK(GtkMainWindow_DeleteEventCB), g.gtkMainWindow);
	g_signal_connect(G_OBJECT(g.gtkMainWindow), "configure-event", G_CALLBACK(GtkMainWindow_ConfigureEventCB), g.gtkMainWindow);
	g.gSettings = g_settings_new("org.pylax");

	x = g_settings_get_int(g.gSettings, "main-window-x");
	y = g_settings_get_int(g.gSettings, "main-window-y");
	width = g_settings_get_int(g.gSettings, "main-window-width");
	height = g_settings_get_int(g.gSettings, "main-window-height");

	gtk_window_move(GTK_WINDOW(g.gtkMainWindow), x, y);
	gtk_window_set_default_size(GTK_WINDOW(g.gtkMainWindow), width, height);

	gtkBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkAccelGroup* gtkAccelGroup = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(g.gtkMainWindow), gtkAccelGroup);

	//GtkActionGroup* gtkActionGroup = gtk_action_group_new("General");

	g.gtkActionFileOpen = gtk_action_new("Open", "Open", "Open a ledger", GTK_STOCK_OPEN);
	g_signal_connect(G_OBJECT(g.gtkActionFileOpen), "activate", ActionFileOpenCB, GTK_WINDOW(g.gtkMainWindow));
	g.gtkActionFileClose = gtk_action_new("Close", "Close", "Close the ledger", GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(g.gtkActionFileClose), "activate", ActionFileCloseCB, GTK_WINDOW(g.gtkMainWindow));
	gtk_action_set_sensitive(GTK_ACTION(g.gtkActionFileClose), FALSE);
	GtkAction* gtkActionFileQuit = gtk_action_new("Quit", "Quit", "Exit Pylax", GTK_STOCK_QUIT);
	g_signal_connect(G_OBJECT(gtkActionFileQuit), "activate", ActionFileQuitCB, GTK_WINDOW(g.gtkMainWindow));
	GtkAction* gtkActionHelpAbout = gtk_action_new("About", "About", "About Pylax", GTK_STOCK_ABOUT);
	g_signal_connect(G_OBJECT(gtkActionHelpAbout), "activate", ActionHelpAboutCB, GTK_WINDOW(g.gtkMainWindow));
	//gtk_action_group_add_action_with_accel(gtkActionGroup, gtkActionFileOpen

	GtkWidget* gtkMenuBar = gtk_menu_bar_new();
	GtkWidget* gtkFileMenu = gtk_menu_new();
	GtkWidget* gtkFileMenuItem = gtk_menu_item_new_with_mnemonic("_File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtkFileMenuItem), gtkFileMenu);

	GtkWidget* gtkOpenMenuItem = gtk_action_create_menu_item(g.gtkActionFileOpen);
	gtk_widget_add_accelerator(gtkOpenMenuItem, "activate", gtkAccelGroup, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkFileMenu), GTK_WIDGET(gtkOpenMenuItem));

	GtkWidget* gtkCloseMenuItem = gtk_action_create_menu_item(g.gtkActionFileClose);
	gtk_widget_add_accelerator(gtkCloseMenuItem, "activate", gtkAccelGroup, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkFileMenu), GTK_WIDGET(gtkCloseMenuItem));

	GtkWidget* gtkQuitMenuItem = gtk_action_create_menu_item(gtkActionFileQuit);
	gtk_widget_add_accelerator(gtkQuitMenuItem, "activate", gtkAccelGroup, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkFileMenu), GTK_WIDGET(gtkQuitMenuItem));
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenuBar), GTK_WIDGET(gtkFileMenuItem));

	g.gtkAppMenu = gtk_menu_new();
	g.gtkAppMenuItem = gtk_menu_item_new_with_mnemonic("_App");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(g.gtkAppMenuItem), g.gtkAppMenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenuBar), GTK_WIDGET(g.gtkAppMenuItem));
	gtk_widget_set_sensitive(GTK_WIDGET(g.gtkAppMenuItem), false);

	GtkWidget* gtkHelpMenu = gtk_menu_new();
	GtkWidget* gtkHelpMenuItem = gtk_menu_item_new_with_mnemonic("_Help");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtkHelpMenuItem), gtkHelpMenu);
	GtkWidget* gtkAboutMenuItem = gtk_action_create_menu_item(gtkActionHelpAbout);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkHelpMenu), GTK_WIDGET(gtkAboutMenuItem));
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenuBar), GTK_WIDGET(gtkHelpMenuItem));
	gtk_box_pack_start(gtkBox, gtkMenuBar, false, false, 0);

	GtkWidget* gtkToolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(gtkToolbar), GTK_TOOLBAR_ICONS);
	GtkToolItem* gtkToolItem = gtk_action_create_tool_item(g.gtkActionFileOpen);
	gtk_toolbar_insert(gtkToolbar, gtkToolItem, -1);
	gtk_box_pack_start(gtkBox, gtkToolbar, false, false, 0);

	g.gtkNotebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(g.gtkNotebook, GTK_POS_BOTTOM);
	gtk_box_pack_start(gtkBox, g.gtkNotebook, TRUE, TRUE, 0);

	g.gtkStatusbar = gtk_statusbar_new();
	gtk_box_pack_start(gtkBox, g.gtkStatusbar, false, false, 0);
	gtk_container_add(GTK_CONTAINER(g.gtkMainWindow), gtkBox);
	gtk_widget_set_margin_top(GTK_WIDGET(g.gtkStatusbar), 0);
	gtk_widget_set_margin_bottom(GTK_WIDGET(g.gtkStatusbar), 0);

	g.wcModulePathOld = Py_GetPath();
	gtk_widget_show_all(g.gtkMainWindow);

	//-------------------------------------
	//OpenApp("/media/tfu/OTG/Pylax/Apps/Southwind.pxa/Southwind.px");
	//OpenApp("/media/tfu/OTG/Pylax/Apps/Test.pxa/Test.px");
	//OpenApp("/media/tfu/OTG/Pylax/Apps/PostgreSQL.pxa/PostgreSQL.px");
	//OpenApp("/media/tfu/OTG/Pylax/Apps/MySQL.pxa/MySQL.px");
	//OpenApp("/media/tfu/OTG/Pylax/Apps/Hinterland.pxa/HinterlandTest.px");
	//OpenApp("/media/tfu/OTG/Pylax/GTK/Test.px");
}

static bool
OpenApp(char* sFileNamePath)
{
	Py_NoSiteFlag = 1;
	PyImport_AppendInittab("pylax", PyInit_pylax);
	Py_InitializeEx(0);

	g.pyStdDateTimeFormat = PyUnicode_FromString("{:%Y-%m-%d}");
	g.pyBeforeCloseCB = NULL;

	// initialize g.pyEnumType
	g.pySQLiteModule = PyImport_ImportModule("enum");
	if (g.pySQLiteModule == NULL) {
		ErrorDialog("Can not import Python module 'enum'");
		return;
	}
	g.pyEnumType = PyObject_GetAttrString(g.pySQLiteModule, "Enum");
	if (!g.pyEnumType) {
		ErrorDialog("Can not load Python type 'Enum'");
		return;
	}

	// initialize g.pySQLiteModule
	g.pySQLiteModule = PyImport_ImportModule("sqlite3");
	if (g.pySQLiteModule == NULL) {
		ErrorDialog("Can not import Python module sqlite3");
		return;
	}

	g.pySQLiteConnectionType = (PyTypeObject*)PyObject_GetAttrString(g.pySQLiteModule, "Connection");
	if (g.pySQLiteConnectionType == NULL) {
		ErrorDialog("Cannot get SQLite Connection type");
		return;
	}

	if (readlink("/proc/self/exe", g.sExecutablePath, PATH_MAX) == -1) {
		ErrorDialog("Could not determine path of program executable.");
		return;
	}

	char *sFileName = sFileNamePath, *next;
	while ((next = strpbrk(sFileName + 1, "\\/")))
		sFileName = next;
	if (sFileNamePath != sFileName)
		sFileName++;
	memcpy(g.sAppPath, sFileNamePath, sFileName - sFileNamePath);
	// append to Python module library path
	char* sModulePathOld = Py_EncodeLocale(g.wcModulePathOld, NULL);
	char* sParts[3] = { sModulePathOld, ":", g.sAppPath };
	char* sModulePath = StringArrayCat(sParts, 3);
	wchar_t* swModulePath = Py_DecodeLocale(sModulePath, NULL);
	if (swModulePath == NULL) {
		PyErr_PrintEx(1);
		ErrorDialog("Can not convert path.");
		return false;
	}

	PySys_SetPath(swModulePath);
	PyMem_Free(sModulePathOld);
	PyMem_RawFree(swModulePath);

	// connect to database
	PyObject* pyFunc = PyObject_GetAttrString(g.pySQLiteModule, "connect");
	PyObject* pyArgList = Py_BuildValue("(sii)", sFileNamePath, TIMEOUT, PARSE_DECLTYPES | PARSE_COLNAMES); // timeout, detect_types

	g.pyConnection = PyObject_CallObject(pyFunc, pyArgList);
	if (g.pyConnection == NULL) {
		PyErr_PrintEx(1);
		ErrorDialog("Can not connect to database.");
		return false;
	}

	g.iCurrentUser = 0;

	PyObject* pyCursor = NULL, *pyResult = NULL;
	if ((pyCursor = PyObject_CallMethod(g.pyConnection, "cursor", NULL)) == NULL) {
		ErrorDialog("Can not access database (cursor creation failed).");
		return false;
	}

	// Check if Px tables exist.
	if ((pyResult = PyObject_CallMethod(pyCursor, "execute", "(s)",
		"SELECT name FROM sqlite_master WHERE type='table' AND name='PxWindow';")) == NULL) {
		ErrorDialog("Can not access database (execution failed).");
		return false;
	}
	Py_DECREF(pyResult);

	if ((pyResult = PyObject_CallMethod(pyCursor, "fetchone", NULL)) == NULL) {
		ErrorDialog("Can not access database (fetch failed).");
		return false;
	}
	g.bConnectionHasPxTables = pyResult == Py_None ? false : true;

	if (pyCursor == NULL || PyObject_CallMethod(pyCursor, "close", NULL) == NULL) {
		ErrorDialog("Can not access database (cursor close failed).");
		return false;
	}
	Py_DECREF(pyCursor);
	Py_DECREF(pyResult);

	PyObject* pyUserModule = PyImport_ImportModule("main");
	if (pyUserModule == NULL) {
		PyErr_PrintEx(1);
		ErrorDialog("Error in Python script");
		return false;
	}

	Hinterland_Init();
	PyObject* pyModuleDict;

	// check if script imported psycopg2 and obtain pointer to class 'connection'
	g.pyPsycopg2ConnectionType = NULL;
	pyModuleDict = PyImport_GetModuleDict();
	g.pyPsycopg2Module = PyDict_GetItemString(pyModuleDict, "psycopg2");
	if (g.pyPsycopg2Module) {
		PyObject* pyPsycopg2ExtensionsModule = PyObject_GetAttrString(g.pyPsycopg2Module, "extensions");
		g.pyPsycopg2ConnectionType = (PyTypeObject*)PyObject_GetAttrString(pyPsycopg2ExtensionsModule, "connection");
		if (g.pyPsycopg2ConnectionType == NULL)
			ErrorDialog("Cannot get Psycopg2 connection type");
	}

	// check if script imported mysql and obtain pointer to class 'connection'
	g.pyMysqlConnectionType = NULL;
	g.pyMysqlModule = PyDict_GetItemString(pyModuleDict, "mysql");
	if (g.pyMysqlModule) {
		PyObject* pyMysqlConnectorModule = PyObject_GetAttrString(g.pyMysqlModule, "connector");
		PyObject* pyMysqlConnectionModule = PyObject_GetAttrString(pyMysqlConnectorModule, "connection");
		g.pyMysqlConnectionType = (PyTypeObject*)PyObject_GetAttrString(pyMysqlConnectionModule, "MySQLConnection");
		if (g.pyMysqlConnectionType == NULL)
			ErrorDialog("Cannot get MySQL connection type");
	}

    // call funtion 'on_load' if it exists in script
	if (PyObject_HasAttrString(pyUserModule, "on_load")) {
        PyObject* pyOnLoadCB = PyObject_GetAttrString(pyUserModule, "on_load");
		pyResult = PyObject_CallObject(pyOnLoadCB, NULL);
		Py_DECREF(pyOnLoadCB);
		if (pyResult == NULL)
			return false;
		Py_DECREF(pyResult);
	}

	gchar* sTitle = g_strconcat(sFileName, " - Pylax", NULL);
	gtk_window_set_title(GTK_WINDOW(g.gtkMainWindow), sTitle);
	g_free(sTitle);

	gint x, y, width, height;
	gtk_window_get_position(GTK_WINDOW(g.gtkMainWindow), &x, &y);
	gtk_window_move(GTK_WINDOW(g.gtkMainWindow), x, y);   // trigger a configure-event

	gtk_action_set_sensitive(GTK_ACTION(g.gtkActionFileOpen), FALSE);
	gtk_action_set_sensitive(GTK_ACTION(g.gtkActionFileClose), TRUE);
	return true;
}


int
main(int argc, char **argv)
{
	int iResult;
	//wchar_t* wcargv0 = Py_DecodeLocale(argv[0], NULL);
	//Py_SetProgramName(wcargv0);
	Py_SetProgramName(argv[0]);
	g.gtkApp = gtk_application_new("org.pylax", G_APPLICATION_FLAGS_NONE);
	g_object_set(g.gtkApp, "register-session", TRUE, NULL);

	g.gQuark = g_quark_from_static_string("Pylax");

	// initialize EggMarkdown
	g.pDownmarker = egg_markdown_new();
	egg_markdown_set_output(g.pDownmarker, EGG_MARKDOWN_OUTPUT_PANGO);
	egg_markdown_set_escape(g.pDownmarker, TRUE);
	egg_markdown_set_smart_quoting(g.pDownmarker, TRUE);

	g_signal_connect(g.gtkApp, "activate", G_CALLBACK(GtkAppActivateEventCB), NULL);
	iResult = g_application_run(G_APPLICATION(g.gtkApp), argc, argv);
	g_object_unref(g.gtkApp);

	return iResult;
}

// ---- module functions -----------------------------------------------------

PyObject*
Pylax_message(PyObject* self, PyObject* args)
{
	const char* sMessage, *sTitle = NULL;

	if (!PyArg_ParseTuple(args, "s|s", &sMessage, &sTitle))
		return NULL;

	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow, GTK_DIALOG_DESTROY_WITH_PARENT, 0, GTK_BUTTONS_CLOSE, sMessage);
	if (sTitle == NULL)
		gtk_window_set_title(GTK_WINDOW(gtkDialog), "Information");
	else
		gtk_window_set_title(GTK_WINDOW(gtkDialog), sTitle);
	gtk_dialog_run(GTK_DIALOG(gtkDialog));
	gtk_widget_destroy(gtkDialog);
	Py_RETURN_NONE;
}

PyObject*
Pylax_status_message(PyObject* self, PyObject* args)
{
	const char* sMessage;

	if (!PyArg_ParseTuple(args, "s", &sMessage))
		return NULL;

	gtk_statusbar_push(g.gtkStatusbar, 1, sMessage);
	Py_RETURN_NONE;
}

PyObject*
Pylax_append_menu_item(PyObject* self, PyObject* args)
{
	PyObject *pyMenuItem = NULL;
	if (!PyArg_ParseTuple(args, "O", &pyMenuItem))
		return NULL;

	if (PyObject_TypeCheck(pyMenuItem, &PxMenuItemType)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(g.gtkAppMenu), GTK_MENU_ITEM(((PxMenuItemObject*)pyMenuItem)->gtk));
		gtk_widget_set_sensitive(GTK_WIDGET(g.gtkAppMenuItem), true);
		gtk_widget_show_all(g.gtkAppMenu);
	}
	else {
		PyErr_SetString(PyExc_TypeError, "Parameter must be a MenuItem.");
		return NULL;
	}
	Py_RETURN_NONE;
}
