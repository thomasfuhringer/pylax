// main.c  | Wanderlust © 2017 by Thomas Führinger

#include "Wanderlust.h"

Globals       g;            // global singleton
HinterlandMsg hlMsg;

// forward declarations
static gint Connection_New(const char* sSite, const int iPort, const int iPage);
static void ActionConnectCB(GtkAction* gtkAction, gpointer pUserData);
static void ActionLogInCB(GtkAction* gtkAction, gpointer pUserData);
void gtk_text_buffer_set_markup(GtkTextBuffer* gtkTextBuffer, char* sMarkup);
static void GtkWidget_SetPadding(GtkWidget* gtkWidget, int iLeftRight, int iTopBottom);
static void SessionClose_ClickedCB(GtkButton* gtkButton, gpointer pUserData);
static void Session_DestroyCB(GtkWidget* gtkWidget, ClientSession* pSession);
static void Session_RenderPage(ClientSession* pSession, int iPageID);
static void MenuItemSignOnCB(GtkMenuItem *gtkMenuItem, gpointer pUserData);
static void NotebookSwitchPageCB(GtkNotebook* gtkNotebook, GtkWidget* gtkWidget, guint iPage, gpointer pUserData);
static gboolean GtkMainWindow_DeleteEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer pUserData);
static void ActionHelpAboutCB(GtkAction* gtkAction, gpointer pUserData);
static void ActionFileQuitCB(GtkAction* gtkAction, gpointer pUserData);
static void ActionSitePropertiesCB(GtkAction* gtkAction, gpointer pUserData);
static void UpdateControls();
GtkWidget* Session_ConstructPagesPane(ClientSession* pSession);
GtkWidget* Session_ConstructMessagesPane(ClientSession* pSession);

static void
GtkAppActivateEventCB(GtkApplication* app, gpointer pUserData)
{
	GtkWidget* gtkMainVBox;
	gint iX, iY, iWidth, iHeight;

	g.pCurrentSession = NULL;
	//g.gdkPixbufLoader = gdk_pixbuf_loader_new();
	g.gtkImageClose = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	g.gtkImageRefresh = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
	g.gtkImageFind = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
	g.gtkImagePerson = gtk_image_new_from_file("Person.ico");
	g.gdkPixbufKey = gdk_pixbuf_new_from_file("Key.ico",NULL);
	g.gdkPixbufPlaceHolder = gdk_pixbuf_new_from_file_at_size("Placeholder.bmp", PAGE_PICTURE_WIDTH, PAGE_PICTURE_HEIGHT, NULL);
	g.gtkMainWindow = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(g.gtkMainWindow), "Wanderlust");
	GdkPixbuf* gdkPixbufIcon = gdk_pixbuf_new_from_file("App.ico", NULL);
	gtk_window_set_default_icon(gdkPixbufIcon);
	g_signal_connect(G_OBJECT(g.gtkMainWindow), "delete-event", G_CALLBACK(GtkMainWindow_DeleteEventCB), g.gtkMainWindow);
	g.gSettings = g_settings_new("org.wanderlust");

	iX = g_settings_get_int(g.gSettings, "main-window-x");
	iY = g_settings_get_int(g.gSettings, "main-window-y");
	iWidth = g_settings_get_int(g.gSettings, "main-window-width");
	iHeight = g_settings_get_int(g.gSettings, "main-window-height");

	gtk_window_move(GTK_WINDOW(g.gtkMainWindow), iX, iY);
	gtk_window_set_default_size(GTK_WINDOW(g.gtkMainWindow), iWidth, iHeight);

	gtkMainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkAccelGroup* gtkAccelGroup = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(g.gtkMainWindow), gtkAccelGroup);

	GtkAction* gtkActionHelpAbout = gtk_action_new("About", "About", "About Pylax", GTK_STOCK_ABOUT);
	g_signal_connect(G_OBJECT(gtkActionHelpAbout), "activate", ActionHelpAboutCB, GTK_WINDOW(g.gtkMainWindow));
	GtkAction* gtkActionFileQuit = gtk_action_new("Quit", "Quit", "Exit Wanderlust", GTK_STOCK_QUIT);
	g_signal_connect(G_OBJECT(gtkActionFileQuit), "activate", ActionFileQuitCB, GTK_WINDOW(g.gtkMainWindow));

	GtkAction* gtkActionConnect = gtk_action_new("Connect", "Connect", "Connect to site.", GTK_STOCK_CONNECT);
	g_signal_connect(G_OBJECT(gtkActionConnect), "activate", ActionConnectCB, GTK_WINDOW(g.gtkMainWindow));
	//gdkPixbufIcon = gdk_pixbuf_new_from_file("App.ico", NULL);
	g.gtkActionLogIn = gtk_action_new("Log In", "Log In", "Log in as user.", GTK_STOCK_DIALOG_AUTHENTICATION); //GTK_STOCK_DIALOG_AUTHENTICATION
	g_signal_connect(G_OBJECT(g.gtkActionLogIn), "activate", ActionLogInCB, GTK_WINDOW(g.gtkMainWindow));
	GtkAction* gtkActionProperties = gtk_action_new("Properties", "Properties", "Data related to site.", GTK_STOCK_PROPERTIES);
	g_signal_connect(G_OBJECT(gtkActionProperties), "activate", ActionSitePropertiesCB, GTK_WINDOW(g.gtkMainWindow));
	g.gtkActionRefreshMessages = gtk_action_new("Refresh", "Refresh", "Refresh messages.", GTK_STOCK_REFRESH);
	g_signal_connect(G_OBJECT(g.gtkActionRefreshMessages), "activate", ActionMessagesRefreshCB, GTK_WINDOW(g.gtkMainWindow));

	GtkWidget* gtkMenuBar = gtk_menu_bar_new();
	GtkWidget* gtkFileMenu = gtk_menu_new();
	GtkWidget* gtkFileMenuItem = gtk_menu_item_new_with_mnemonic("_File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtkFileMenuItem), gtkFileMenu);

	GtkWidget* gtkQuitMenuItem = gtk_action_create_menu_item(gtkActionFileQuit);
	gtk_widget_add_accelerator(gtkQuitMenuItem, "activate", gtkAccelGroup, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkFileMenu), GTK_WIDGET(gtkQuitMenuItem));
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenuBar), GTK_WIDGET(gtkFileMenuItem));

	GtkWidget* gtkSiteMenu = gtk_menu_new();
	g.gtkSiteMenuItem = gtk_menu_item_new_with_mnemonic("_Site");
	gtk_menu_item_set_use_underline(g.gtkSiteMenuItem, TRUE);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(g.gtkSiteMenuItem), gtkSiteMenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenuBar), GTK_WIDGET(g.gtkSiteMenuItem));

	g.gtkSignOnMenuItem = gtk_menu_item_new_with_mnemonic("Sign _On");
	g_signal_connect(G_OBJECT(g.gtkSignOnMenuItem), "activate", MenuItemSignOnCB, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkSiteMenu), GTK_WIDGET(g.gtkSignOnMenuItem));

	GtkWidget* gtkLogInMenuItem = gtk_action_create_menu_item(g.gtkActionLogIn);
	gtk_widget_add_accelerator(gtkLogInMenuItem, "activate", gtkAccelGroup, GDK_KEY_l, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkSiteMenu), GTK_WIDGET(gtkLogInMenuItem));

	GtkWidget* gtkPropertiesMenuItem = gtk_action_create_menu_item(gtkActionProperties);
	gtk_widget_add_accelerator(gtkPropertiesMenuItem, "activate", gtkAccelGroup, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkSiteMenu), GTK_WIDGET(gtkPropertiesMenuItem));

	GtkWidget* gtkHelpMenu = gtk_menu_new();
	GtkWidget* gtkHelpMenuItem = gtk_menu_item_new_with_mnemonic("_Help");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtkHelpMenuItem), gtkHelpMenu);
	GtkWidget* gtkAboutMenuItem = gtk_action_create_menu_item(gtkActionHelpAbout);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkHelpMenu), GTK_WIDGET(gtkAboutMenuItem));
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenuBar), GTK_WIDGET(gtkHelpMenuItem));
	gtk_box_pack_start(gtkMainVBox, gtkMenuBar, FALSE, FALSE, 0);

	GtkWidget* gtkToolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(gtkToolbar), GTK_TOOLBAR_ICONS);
	GtkWidget_SetPadding(gtkToolbar, 2, 0);

	g.gtkToolItemEncrypted = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_widget(g.gtkToolItemEncrypted, gtk_image_new_from_pixbuf(g.gdkPixbufKey));
	gtk_tool_item_set_tooltip_text(g.gtkToolItemEncrypted, "Use Encryption");
	gtk_widget_set_margin_end(g.gtkToolItemEncrypted, 4);
	gtk_toolbar_insert(GTK_TOOLBAR(GTK_TOOLBAR(gtkToolbar)), GTK_TOOL_ITEM(g.gtkToolItemEncrypted), -1);

	GtkToolItem* gtkToolItem = gtk_tool_item_new();
	g.gtkEntrySite = gtk_entry_new();
	gtk_widget_set_margin_end(g.gtkEntrySite, 4);
	gtk_entry_set_text(g.gtkEntrySite, DEFAULT_SITE);
	gtk_container_add(GTK_CONTAINER(gtkToolItem), GTK_WIDGET(g.gtkEntrySite));
	gtk_toolbar_insert(GTK_TOOLBAR(GTK_TOOLBAR(gtkToolbar)), GTK_TOOL_ITEM(gtkToolItem), -1);
	gtkToolItem = gtk_action_create_tool_item(gtkActionConnect);
	gtk_toolbar_insert(GTK_TOOLBAR(gtkToolbar), gtkToolItem, -1);
	gtkToolItem = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(gtkToolItem, TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(gtkToolbar), gtkToolItem, -1);
	gtkToolItem = gtk_action_create_tool_item(g.gtkActionRefreshMessages);
	gtk_toolbar_insert(GTK_TOOLBAR(gtkToolbar), gtkToolItem, -1);

	gtkToolItem = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(gtkToolItem, FALSE);
	gtk_tool_item_set_expand(gtkToolItem, TRUE);  // right align following items
	gtk_toolbar_insert(GTK_TOOLBAR(GTK_TOOLBAR(gtkToolbar)), gtkToolItem, -1);
	gtkToolItem = gtk_tool_item_new();
	g.gtkLabelUserName = gtk_label_new(NULL);
	gtk_widget_set_margin_end(g.gtkLabelUserName, 4);
	gtk_container_add(GTK_CONTAINER(gtkToolItem), GTK_WIDGET(g.gtkLabelUserName));
	gtk_toolbar_insert(GTK_TOOLBAR(GTK_TOOLBAR(gtkToolbar)), gtkToolItem, -1);
	gtkToolItem = gtk_action_create_tool_item(g.gtkActionLogIn);
	gtk_toolbar_insert(GTK_TOOLBAR(gtkToolbar), gtkToolItem, -1);
	gtk_box_pack_start(gtkMainVBox, gtkToolbar, FALSE, FALSE, 0);

	g.gtkNotebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(g.gtkNotebook, GTK_POS_BOTTOM);
	g_signal_connect(G_OBJECT(g.gtkNotebook), "switch-page", NotebookSwitchPageCB, NULL);
	gtk_box_pack_start(gtkMainVBox, g.gtkNotebook, TRUE, TRUE, 0);

	g.gtkStatusbar = gtk_statusbar_new();
	gtk_box_pack_start(gtkMainVBox, g.gtkStatusbar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g.gtkMainWindow), gtkMainVBox);
	gtk_widget_set_margin_top(GTK_WIDGET(g.gtkStatusbar), 0);
	gtk_widget_set_margin_bottom(GTK_WIDGET(g.gtkStatusbar), 0);

	UpdateControls();
	gtk_widget_show_all(g.gtkMainWindow);

	Py_NoSiteFlag = 1;
	Py_InitializeEx(0);
	g.pyStdDateTimeFormat = PyUnicode_FromString("{:%Y-%m-%d}");

	//PyRun_SimpleString("import sys\nsys.path.append('/usr/lib/python3/dist-packages')\n");  // for Debian
	/*g.pyNaclModule = PyImport_ImportModule("nacl");
	if (g.pyNaclModule == NULL)
		gtk_widget_set_sensitive(g.gtkToolItemEncrypted, FALSE);*/

	g.pyHinterlandModule = PyImport_ImportModule("hinterland");
	if (g.pyHinterlandModule == NULL) {
		PyErr_PrintEx(1);
		ErrorDialog("Can not import Python module 'hinterland'.");
		return;
	}

	g.pyHinterlandClientType = PyObject_GetAttrString(g.pyHinterlandModule, "Client");
	if (!g.pyHinterlandClientType) {
		ErrorDialog("Can not load Python type 'Client'");
		return;
	}

	PyObject* pyHlMsg = PyObject_GetAttrString(g.pyHinterlandModule, "Msg");
	hlMsg.Type = PyObject_GetAttrString(pyHlMsg, "Type");
	hlMsg.SetPage = PyObject_GetAttrString(pyHlMsg, "SetPage");
	hlMsg.GetPage = PyObject_GetAttrString(pyHlMsg, "GetPage");
	hlMsg.GetPageChildren = PyObject_GetAttrString(pyHlMsg, "GetPageChildren");
	hlMsg.GetOrgChildren = PyObject_GetAttrString(pyHlMsg, "GetOrgChildren");
	hlMsg.DeletePage = PyObject_GetAttrString(pyHlMsg, "DeletePage");
	hlMsg.SearchPerson = PyObject_GetAttrString(pyHlMsg, "SearchPerson");
	hlMsg.GetMessageList = PyObject_GetAttrString(pyHlMsg, "GetMessageList");
	hlMsg.NotFound = PyObject_GetAttrString(pyHlMsg, "NotFound");
	hlMsg.Disconnect = PyObject_GetAttrString(pyHlMsg, "Disconnect");
	Py_DECREF(pyHlMsg);
}

static void
ActionFileQuitCB(GtkAction* gtkAction, gpointer pUserData)
{
	g.gtkMainWindow = gtk_application_get_active_window(g.gtkApp);
	gtk_window_close(g.gtkMainWindow);
}

static void
ActionHelpAboutCB(GtkAction* gtkAction, gpointer pUserData)
{
	GStrv sAuthors[] = AUTHORS;
	gtk_show_about_dialog(g.gtkMainWindow,
		"program-name", "Wanderlust",
		"comments", "Browser for Hinterland",
		"version", VER_PRODUCTVERSION_STR,
		"authors", sAuthors,
		"website", "https://github.com/thomasfuhringer/pylax/hinterland/wanderlust",
		NULL);
}

static gboolean
GtkMainWindow_DeleteEventCB(GtkWidget* gtkWidget, GdkEvent* gdkEvent, gpointer pUserData)
{
	gint iX, iY, iWidth, iHeight;
	gtk_window_get_position(g.gtkMainWindow, &iX, &iY);
	gtk_window_get_size(g.gtkMainWindow, &iWidth, &iHeight);
	g_settings_set(g.gSettings, "main-window-x", "i", iX);
	g_settings_set(g.gSettings, "main-window-y", "i", iY);
	g_settings_set(g.gSettings, "main-window-width", "i", iWidth);
	g_settings_set(g.gSettings, "main-window-height", "i", iHeight);
	return FALSE;
}

static void
ActionSitePropertiesCB(GtkAction* gtkAction, gpointer pUserData)
{
	GtkWidget* gtkDialog, *gtkDialogContentArea;
	GtkWidget* gtkLabel, *gtkLabelPrev, *gtkWidget;
	GtkGrid* gtkGrid;
	PyObject* pyProperty, *pyKey, *pyFunction, *pyFingerprint;

	gtkDialog = gtk_dialog_new_with_buttons("Site Properties", g.gtkMainWindow, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"_OK", GTK_RESPONSE_ACCEPT/*, "_Cancel", GTK_RESPONSE_REJECT*/, NULL);
	gtk_window_set_default_size(GTK_WINDOW(gtkDialog), 400, 320);
	gtk_widget_set_size_request(GTK_WINDOW(gtkDialog), 360, 320);
	gtkDialogContentArea = gtk_dialog_get_content_area(GTK_DIALOG(gtkDialog));

	GtkWidget* gtkMainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

	gtkGrid = gtk_grid_new();
	gtk_grid_set_row_spacing(gtkGrid, 4);
	gtk_grid_set_column_spacing(gtkGrid, 20);

	gtkLabel = gtk_label_new("Server ID");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, NULL, GTK_POS_LEFT, 1, 1);
	pyProperty = PyObject_GetAttrString(g.pCurrentSession->pySession, "server_id");
	gtkWidget = gtk_label_new(PyUnicode_AsUTF8(pyProperty));
	Py_DECREF(pyProperty);
	gtk_widget_set_halign(gtkWidget, 1.0);
	gtk_widget_set_hexpand(gtkWidget, TRUE);
	gtk_grid_attach_next_to(gtkGrid, gtkWidget, gtkLabel, GTK_POS_RIGHT, 1, 1);
	gtkLabelPrev = gtkLabel;

	gtkLabel = gtk_label_new("IP Address");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 1, 1);

	pyFingerprint = PyObject_GetAttrString(g.pCurrentSession->pySession, "socket");
	pyFunction = PyObject_GetAttrString(pyFingerprint, "getpeername");
	pyKey = PyObject_CallFunctionObjArgs(pyFunction, NULL);
	pyProperty = PySequence_ITEM(pyKey, 0);
	gtkWidget = gtk_label_new(PyUnicode_AsUTF8(pyProperty));
	Py_DECREF(pyProperty);
	Py_DECREF(pyKey);
	Py_DECREF(pyFunction);
	Py_DECREF(pyFingerprint);
	gtk_widget_set_halign(gtkWidget, 1.0);
	gtk_grid_attach_next_to(gtkGrid, gtkWidget, gtkLabel, GTK_POS_RIGHT, 1, 1);
	gtkLabelPrev = gtkLabel;

	gtkLabel = gtk_label_new("Client ID");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 1, 1);
	pyProperty = PyObject_GetAttrString(g.pCurrentSession->pySession, "token");
	gtkWidget = gtk_label_new(PyUnicode_AsUTF8(pyProperty));
	Py_DECREF(pyProperty);
	gtk_widget_set_halign(gtkWidget, 1.0);
	gtk_grid_attach_next_to(gtkGrid, gtkWidget, gtkLabel, GTK_POS_RIGHT, 1, 1);
	gtkLabelPrev = gtkLabel;

	pyKey = PyObject_GetAttrString(g.pCurrentSession->pySession, "key");
	if (pyKey == Py_None) {
		gtkLabel = gtk_label_new("Communication not encrypted");
		gtk_widget_set_halign(gtkLabel, 1.0);
		gtk_widget_set_margin_top(gtkLabel, 10);
		gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 2, 1);
		gtkLabelPrev = gtkLabel;
	}
	else {
		gtkLabel = gtk_label_new("Secure connection, public key fingerprints:");
		gtk_widget_set_halign(gtkLabel, 1.0);
		gtk_widget_set_margin_top(gtkLabel, 10);
		gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 2, 1);
		gtkLabelPrev = gtkLabel;

		gtkLabel = gtk_label_new("Server");
		gtk_widget_set_halign(gtkLabel, 1.0);
		gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 1, 1);
		gtkLabelPrev = gtkLabel;
		pyFunction = PyObject_GetAttrString(g.pyHinterlandModule, "fingerprint");
		pyFingerprint = PyObject_CallFunctionObjArgs(pyFunction, pyKey, NULL);
		gtkWidget = gtk_label_new(PyUnicode_AsUTF8(pyFingerprint));
		Py_DECREF(pyKey);
		Py_DECREF(pyFingerprint);
		gtk_widget_set_halign(gtkWidget, 1.0);
		gtk_grid_attach_next_to(gtkGrid, gtkWidget, gtkLabel, GTK_POS_RIGHT, 1, 1);

		gtkLabel = gtk_label_new("Client");
		gtk_widget_set_halign(gtkLabel, 1.0);
		gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 1, 1);
		gtkLabelPrev = gtkLabel;
		pyKey = PyObject_GetAttrString(g.pCurrentSession->pySession, "public_key");
		pyFingerprint = PyObject_CallFunctionObjArgs(pyFunction, pyKey, NULL);
		gtkWidget = gtk_label_new(PyUnicode_AsUTF8(pyFingerprint));
		gtk_widget_set_halign(gtkWidget, 1.0);
		gtk_grid_attach_next_to(gtkGrid, gtkWidget, gtkLabel, GTK_POS_RIGHT, 1, 1);
		gtkLabelPrev = gtkLabel;
		Py_DECREF(pyKey);
		Py_DECREF(pyFingerprint);
		Py_DECREF(pyFunction);
	}

	if (g.pCurrentSession->pyUserID == Py_None || g.pCurrentSession->pyUserID == NULL) {
		gtkLabel = gtk_label_new("Not Logged In");
		gtk_widget_set_halign(gtkLabel, 1.0);
		gtk_widget_set_margin_top(gtkLabel, 10);
		gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 1, 1);
	}
	else {
		gtkLabel = gtk_label_new("Logged In");
		gtk_widget_set_halign(gtkLabel, 1.0);
		gtk_widget_set_margin_top(gtkLabel, 10);
		gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkLabelPrev, GTK_POS_BOTTOM, 1, 1);

		int iUserID = PyLong_AsLong(g.pCurrentSession->pyUserID);
		char sUser[256];
		sprintf(sUser, "%s (ID %d)", g.pCurrentSession->sCurrentUserHandle, iUserID);
		gtkWidget = gtk_label_new(sUser);
		gtk_widget_set_halign(gtkWidget, 1.0);
		gtk_widget_set_margin_top(gtkWidget, 10);
		gtk_grid_attach_next_to(gtkGrid, gtkWidget, gtkLabel, GTK_POS_RIGHT, 1, 1);
	}

	gtk_container_add(GTK_CONTAINER(gtkDialogContentArea), gtkGrid);
	gtk_container_set_border_width(GTK_CONTAINER(gtkDialogContentArea), 30);
	gtk_container_set_border_width(GTK_CONTAINER(gtkGrid), 6);

	gtk_widget_show_all(gtkDialog);
	if (gtk_dialog_run(GTK_DIALOG(gtkDialog)) == GTK_RESPONSE_ACCEPT) {
	}
	gtk_widget_destroy(gtkDialog);
}

static void
ActionConnectCB(GtkAction* gtkAction, gpointer pUserData)
{
	Connection_New(gtk_entry_get_text(g.gtkEntrySite), 0, 0);
}

static gboolean
Connection_New(const char* sSite, const int iPort, const int iPage)
{
	ClientSession* pSession = g_new0(ClientSession, 1);
	GtkToolItem* gtkToolItem;
	GtkLabel* gtkLabel;
	GtkWidget* gtkMainVBox;

	PyObject* pyArgList = Py_BuildValue("(s)", sSite);
	PyObject* pyKwDict = PyDict_New();
	if (gtk_toggle_tool_button_get_active(g.gtkToolItemEncrypted))
		PyDict_SetItemString(pyKwDict, "encrypted", Py_True);
	pSession->pySession = PyObject_Call(g.pyHinterlandClientType, pyArgList, pyKwDict);
	Py_DECREF(pyArgList);
	Py_DECREF(pyKwDict);
	if (pSession->pySession == NULL) {
		PythonErrorDialog("Can not connect to Hinterland host.");
		//PyErr_PrintEx(1);
		return FALSE;
	}
	PyObject* pyError = PyObject_GetAttrString(pSession->pySession, "status_message");
	if (pyError != Py_None) {
		ErrorDialog(PyUnicode_AsUTF8(pyError));
		Py_DECREF(pyError);
		return FALSE;
	}

	PyObject* pyServerID = PyObject_GetAttrString(pSession->pySession, "server_id");
	if (pyServerID == NULL)
		PyErr_PrintEx(1);
	pSession->gtkLabelSite = gtk_label_new(PyUnicode_AsUTF8(pyServerID));
	Py_DECREF(pyServerID);

	GtkWidget* gtkBoxTabLabel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkButton* gtkButtonClose = gtk_button_new();
	gtk_button_set_image(gtkButtonClose, g.gtkImageClose);
	gtk_widget_set_tooltip_text(gtkButtonClose, "Close");
	gtk_button_set_relief(gtkButtonClose, GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(gtkButtonClose, FALSE);
	g_signal_connect(G_OBJECT(gtkButtonClose), "clicked", G_CALLBACK(SessionClose_ClickedCB), pSession);
	gtk_box_pack_start(gtkBoxTabLabel, pSession->gtkLabelSite, FALSE, FALSE, 4);
	gtk_box_pack_start(gtkBoxTabLabel, gtkButtonClose, FALSE, FALSE, 0);
	gtk_widget_show_all(gtkBoxTabLabel);

	pSession->gtkNotebook = gtk_notebook_new();
	g_signal_connect(G_OBJECT(pSession->gtkNotebook), "destroy", G_CALLBACK(Session_DestroyCB), pSession);
	g_object_set_qdata(G_OBJECT(pSession->gtkNotebook), g.gQuark, pSession);

	GtkWidget* gtkPagesMainVBox = Session_ConstructPagesPane(pSession);
	gtkLabel = gtk_label_new("Pages");
	gtk_notebook_append_page(GTK_NOTEBOOK(pSession->gtkNotebook), gtkPagesMainVBox, gtkLabel);

	GtkWidget* gtkMessagesMainVBox = Session_ConstructMessagesPane(pSession);
	gtkLabel = gtk_label_new("Messages");
	gtk_notebook_append_page(GTK_NOTEBOOK(pSession->gtkNotebook), gtkMessagesMainVBox, gtkLabel);

	pSession->iIndex = gtk_notebook_append_page(GTK_NOTEBOOK(g.gtkNotebook), pSession->gtkNotebook, gtkBoxTabLabel);
	gtk_widget_show_all(pSession->gtkNotebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(g.gtkNotebook), pSession->iIndex);
	GdkRGBA gdkRGBA = { .1,.1,.1,.0 };
	gtk_widget_override_background_color(g.gtkNotebook, GTK_STATE_FLAG_NORMAL, &gdkRGBA);

	Session_InitTreeOrg(pSession);
	Session_InitTreeKeyWord(pSession);
	//Session_PullHomePage(pSession);
	return pSession->iIndex;
}

static void
NotebookSwitchPageCB(GtkNotebook* gtkNotebook, GtkWidget* gtkWidget, guint iPage, gpointer pUserData)
{
	g.pCurrentSession = (ClientSession*)g_object_get_qdata(G_OBJECT(gtkWidget), g.gQuark);
	//g_debug("Notebook page %d Session %p selected.", iPage, g.pCurrentSession);
	UpdateControls();
}

static void
SessionClose_ClickedCB(GtkButton* gtkButton, gpointer pUserData)
{
	ClientSession* pSession = (ClientSession*)pUserData;
	gtk_notebook_remove_page(GTK_NOTEBOOK(g.gtkNotebook), pSession->iIndex);
}

static void
UpdateControls()
{
	//g_debug("UpdateControlsn %p selected.", g.pCurrentSession);
	if (g.pCurrentSession == NULL) {
		gtk_widget_set_sensitive(g.gtkSiteMenuItem, FALSE);
		gtk_label_set_text(g.gtkLabelUserName, "");
		gtk_action_set_sensitive(g.gtkActionLogIn, FALSE);
		gtk_action_set_sensitive(g.gtkActionRefreshMessages, FALSE);
	}
	else {
		gtk_widget_set_sensitive(g.gtkSiteMenuItem, TRUE);
		if (g.pCurrentSession->pyUserID == Py_None || g.pCurrentSession->pyUserID == NULL) {
			gtk_label_set_text(g.gtkLabelUserName, "Anonymous");
			gtk_action_set_sensitive(g.gtkActionLogIn, TRUE);
			gtk_widget_set_sensitive(g.gtkSignOnMenuItem, TRUE);
			gtk_action_set_sensitive(g.gtkActionRefreshMessages, FALSE);
		}
		else {
			gtk_label_set_text(g.gtkLabelUserName, g.pCurrentSession->sCurrentUserHandle);
			gtk_action_set_sensitive(g.gtkActionLogIn, FALSE);
			gtk_widget_set_sensitive(g.gtkSignOnMenuItem, FALSE);
			gtk_action_set_sensitive(g.gtkActionRefreshMessages, TRUE);
		}
	}
}

static void
Session_DestroyCB(GtkWidget* gtkWidget, ClientSession* pSession)
{
	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.Disconnect);
	PyObject* pyResult = Session_Send(pSession);
	Py_XDECREF(pyResult);
	g_debug("Disconnected");
	free(pSession);
	g.pCurrentSession = NULL;
	UpdateControls();
}

static void
MenuItemSignOnCB(GtkMenuItem *gtkMenuItem, gpointer pUserData)
{
	GtkWidget* gtkGrid, *gtkDialog, *gtkDialogContentArea;
	GtkLabel* gtkLabel;
	GtkEntry* gtkEntryHandle, *gtkEntryFirstName, *gtkEntryLastName, *gtkEntryPassword;
	PyObject* pyResult;
	char* sHandle;

	gtkDialog = gtk_dialog_new_with_buttons("Sign On", g.gtkMainWindow, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);
	gtkDialogContentArea = gtk_dialog_get_content_area(GTK_DIALOG(gtkDialog));

	gtkGrid = gtk_grid_new();
	gtk_grid_set_row_spacing(gtkGrid, 4);
	gtk_grid_set_column_spacing(gtkGrid, 14);

	gtkLabel = gtk_label_new("Handle");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtkEntryHandle = gtk_entry_new();
	gtk_widget_set_hexpand(gtkEntryHandle, TRUE);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, NULL, GTK_POS_LEFT, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkEntryHandle, gtkLabel, GTK_POS_RIGHT, 1, 1);

	gtkLabel = gtk_label_new("First Name");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtkEntryFirstName = gtk_entry_new();
	gtk_grid_attach_next_to(gtkGrid, gtkEntryFirstName, gtkEntryHandle, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkEntryFirstName, GTK_POS_LEFT, 1, 1);

	gtkLabel = gtk_label_new("Last Name");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtkEntryLastName = gtk_entry_new();
	gtk_grid_attach_next_to(gtkGrid, gtkEntryLastName, gtkEntryFirstName, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkEntryLastName, GTK_POS_LEFT, 1, 1);

	gtkLabel = gtk_label_new("Password");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtkEntryPassword = gtk_entry_new();
	gtk_entry_set_invisible_char(gtkEntryPassword, '*');
	gtk_entry_set_visibility(gtkEntryPassword, FALSE);
	gtk_widget_set_size_request(GTK_WIDGET(gtkEntryPassword), 240, 0);
	gtk_grid_attach_next_to(gtkGrid, gtkEntryPassword, gtkEntryLastName, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkEntryPassword, GTK_POS_LEFT, 1, 1);

	gtk_container_add(GTK_CONTAINER(gtkDialogContentArea), gtkGrid);
	gtk_container_set_border_width(GTK_CONTAINER(gtkDialogContentArea), 30);
	gtk_container_set_border_width(GTK_CONTAINER(gtkGrid), 6);
	gtk_widget_show_all(gtkDialog);

	while (gtk_dialog_run(GTK_DIALOG(gtkDialog)) == GTK_RESPONSE_ACCEPT) {

		if ((pyResult = PyObject_CallMethod(g.pCurrentSession->pySession, "sign_on", "(ssss)",
			gtk_entry_get_text(gtkEntryHandle),
			gtk_entry_get_text(gtkEntryFirstName),
			gtk_entry_get_text(gtkEntryLastName),
			gtk_entry_get_text(gtkEntryPassword))) == NULL) {
			PythonErrorDialog("Can not sign on.");
			return FALSE;
		}
		else {
			if (pyResult == Py_True) {
				g.pCurrentSession->pyUserID = PyObject_GetAttrString(g.pCurrentSession->pySession, "user");
				Py_DECREF(g.pCurrentSession->pyUserID);  // borrow reference
				g_free(g.pCurrentSession->sCurrentUserHandle);
				g.pCurrentSession->sCurrentUserHandle = g_strdup(gtk_entry_get_text(gtkEntryHandle));
				//g_debug(g.pCurrentSession->sCurrentUserHandle);
				gtk_label_set_text(g.gtkLabelUserName, gtk_entry_get_text(gtkEntryHandle));
				gtk_statusbar_push(g.gtkStatusbar, 1, "Signed on");
				UpdateControls();
				break;
			}
			else {
				PyObject* pyError = PyObject_GetAttrString(g.pCurrentSession->pySession, "status_message");
				ErrorDialog(PyUnicode_AsUTF8(pyError));
				Py_DECREF(pyError);
			}
			Py_DECREF(pyResult);
		}
	}
	gtk_widget_destroy(gtkDialog);
}

static void
Dialog_AcceptOnEnterCB(GtkWidget* gtkWidget, GtkWidget* gtkDialog)
{
	g_signal_emit_by_name(G_OBJECT(gtkDialog), "response", GTK_RESPONSE_ACCEPT);
}

static void
ActionLogInCB(GtkAction* gtkAction, gpointer pUserData)
{
	GtkWidget* gtkGrid, *gtkDialog, *gtkDialogContentArea;
	GtkWidget* gtkEntryUserName, *gtkEntryPassword, *gtkLabel;
	char* sUser, *sPassword;
	PyObject* pyResult;
	long iMe;
	gint iResult;

	g_assert(g.pCurrentSession != NULL);
	//ClientSession* pSession = (ClientSession*)pUserData;
	gtkEntryUserName = gtk_entry_new();
	gtkEntryPassword = gtk_entry_new();

	gtk_entry_set_visibility(gtkEntryPassword, FALSE);
	gtk_entry_set_invisible_char(gtkEntryPassword, '*');
	gtk_widget_set_size_request(GTK_WIDGET(gtkEntryPassword), 240, 0);

	gtkDialog = gtk_dialog_new_with_buttons("Log In", g.gtkMainWindow, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);
	gtk_dialog_set_default_response(gtkDialog, GTK_RESPONSE_ACCEPT);
	g_signal_connect(gtkEntryPassword, "activate", G_CALLBACK(Dialog_AcceptOnEnterCB), (gpointer)gtkDialog);
	gtkDialogContentArea = gtk_dialog_get_content_area(GTK_DIALOG(gtkDialog));

	gtkGrid = gtk_grid_new();
	gtk_grid_set_row_spacing(gtkGrid, 4);
	gtk_grid_set_column_spacing(gtkGrid, 14);

	gtkLabel = gtk_label_new("User");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtkEntryUserName = gtk_entry_new();
	gtk_widget_set_size_request(GTK_WIDGET(gtkEntryUserName), 240, 0);
	gtk_widget_set_hexpand(gtkEntryUserName, TRUE);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, NULL, GTK_POS_LEFT, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkEntryUserName, gtkLabel, GTK_POS_RIGHT, 1, 1);

	gtkLabel = gtk_label_new("Password");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtkEntryPassword = gtk_entry_new();
	gtk_entry_set_visibility(gtkEntryPassword, FALSE);
	gtk_entry_set_invisible_char(gtkEntryPassword, '*');
	gtk_grid_attach_next_to(gtkGrid, gtkEntryPassword, gtkEntryUserName, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkEntryPassword, GTK_POS_LEFT, 1, 1);

	if ((pyResult = PyObject_CallMethod(g.pCurrentSession->pySession, "get_me", NULL)) == NULL) {
		PythonErrorDialog("Can not retrieve last user ID.");
		return;
	}
	else {
		if (pyResult != Py_None) {
			iMe = PyLong_AsLong(pyResult);
			char sMe[30];
			int value = 4564;
			sprintf(sMe, "%d", iMe);
			gtk_entry_set_text(gtkEntryUserName, sMe);
		}
		Py_DECREF(pyResult);
	}

	gtk_container_add(GTK_CONTAINER(gtkDialogContentArea), gtkGrid);
	gtk_container_set_border_width(GTK_CONTAINER(gtkDialogContentArea), 30);
	gtk_container_set_border_width(GTK_CONTAINER(gtkGrid), 6);
	gtk_widget_set_margin_bottom(gtkGrid, 10);
	gtk_widget_show_all(gtkDialog);
	gtk_widget_grab_focus(gtkEntryPassword);

	while (gtk_dialog_run(GTK_DIALOG(gtkDialog)) == GTK_RESPONSE_ACCEPT) {
		sUser = gtk_entry_get_text(gtkEntryUserName);
		sPassword = gtk_entry_get_text(gtkEntryPassword);
		//g_debug(sPassword);
		if (strlen(sPassword) == 0)
			pyResult = PyObject_CallMethod(g.pCurrentSession->pySession, "log_in", "(s)", sUser);
		else
			pyResult = PyObject_CallMethod(g.pCurrentSession->pySession, "log_in", "(ss)", sUser, sPassword);

		if (pyResult == NULL) {
			PythonErrorDialog("Can not log in.");
			return;
		}
		else {
			if (pyResult == Py_True) {
				g.pCurrentSession->pyUserID = PyObject_GetAttrString(g.pCurrentSession->pySession, "user");
				Py_DECREF(g.pCurrentSession->pyUserID);  // borrow reference
				PyObject* pyLastName = Session_GetMessageAttribute(g.pCurrentSession, "Handle");
				g_free(g.pCurrentSession->sCurrentUserHandle);
				g.pCurrentSession->sCurrentUserHandle = g_strdup(PyUnicode_AsUTF8(pyLastName));
				gtk_label_set_text(g.gtkLabelUserName, g.pCurrentSession->sCurrentUserHandle);
				gtk_statusbar_push(g.gtkStatusbar, 1, "Logged in");
				UpdateControls();
				break;
			}
			else {
				PyObject* pyError = PyObject_GetAttrString(g.pCurrentSession->pySession, "status_message");
				ErrorDialog(PyUnicode_AsUTF8(pyError));
				Py_DECREF(pyError);
			}
			Py_DECREF(pyResult);
		}
	}
	gtk_widget_destroy(gtkDialog);
}

static void GtkImageButton_ClickedCB(GtkButton* gtkWidget, gpointer pUserData);



PyObject*
Session_GetMessageAttribute(ClientSession* pSession, char* sKey)
{
	PyObject* pyResult;
	if ((pyResult = PyObject_GetAttrString(pSession->pySession, "msg")) == NULL) {
		PythonErrorDialog("Can not get dict key 'msg'.");
		return NULL;
	}
	Py_DECREF(pyResult); // just borrow reference

	if ((pyResult = PyDict_GetItemString(pyResult, sKey)) == NULL) {
		PythonErrorDialog("Can not get dict key.");
		return NULL;
	}
	Py_DECREF(pyResult);
	return pyResult;
}

PyObject*
Session_Send(ClientSession* pSession)
{
	PyObject* pyResult;
	if ((pyResult = PyObject_CallMethod(pSession->pySession, "send", "(O)", g.pyMsg)) == NULL) {
		g_debug("Session_Send failed");
		PyErr_Print();
		//PythonErrorDialog("Can not send message.");
		return NULL;
	}
	else
		Py_DECREF(g.pyMsg);
	return pyResult;
}

gboolean
Session_Exchange(ClientSession* pSession)
{
	PyObject* pyResult;
	if ((pyResult = PyObject_CallMethod(pSession->pySession, "exchange", "(O)", g.pyMsg)) == NULL) {
		g_debug("Session_Exchange failed");
		//PythonErrorDialog("Can not exchange message.");
		PyErr_Print();
		return FALSE;
	}
	else {
		Py_DECREF(g.pyMsg);
		gboolean bResult = TRUE ? pyResult == Py_True : FALSE;
		Py_DECREF(pyResult);
		pSession->pyResultMsg = PyObject_GetAttrString(pSession->pySession, "msg");
		Py_DECREF(pSession->pyResultMsg);    // just borrow reference
		if (bResult) {
			pSession->sStatusMessage = NULL;
			return TRUE;
		}
		else {
			PyObject* pyStatusMessage = PyObject_GetAttrString(pSession->pySession, "status_message");
			if (pyStatusMessage == Py_None)
				pSession->sStatusMessage = NULL;
			else
				pSession->sStatusMessage = PyUnicode_AsUTF8(pyStatusMessage);
			Py_DECREF(pyStatusMessage);
			return FALSE;
		}
	}
}

PyObject*
PyDict_GetItemStringX(PyObject* pyDict, char* sKey)
{
	PyObject* pyKey, *pyResult;
	pyKey = PyUnicode_FromString(sKey);
	if (PyDict_Contains(pyDict, pyKey) == 1)
		pyResult = PyDict_GetItem(pyDict, pyKey);
	else
		pyResult = PyExc_KeyError;
	Py_DECREF(pyKey);
	return pyResult;
}

char*
PyUnicode_AsString(PyObject* pyString)
{
	if (PyUnicode_Check(pyString))
		return PyUnicode_AsUTF8(pyString);
	else
		return NULL;
}

void
gtk_text_buffer_set_markup(GtkTextBuffer* gtkTextBuffer, char* sMarkup)
{
	egg_markdown_clear(g.pDownmarker);
	egg_markdown_set_extensions(g.pDownmarker, EGG_MARKDOWN_EXTENSION_GITHUB);
	char* sPango = egg_markdown_parse(g.pDownmarker, sMarkup);
	GtkTextIter gtkTextIterStart, gtkTextIterEnd;
	//gtk_text_buffer_set_text(gtkTextBuffer,NULL,0);
	gtk_text_buffer_get_bounds(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	gtk_text_buffer_delete(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	gtk_text_buffer_get_bounds(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
	gtk_text_buffer_insert_markup(gtkTextBuffer, &gtkTextIterStart, sPango, -1);
	g_free(sPango);
}

static void
GtkWidget_SetPadding(GtkWidget* gtkWidget, int iLeftRight, int iTopBottom)
{
	char* str = g_strdup_printf(
		"GtkWidget { padding-left: %dpx; padding-right: %dpx; "
		"padding-top: %dpx; padding-bottom: %dpx; }",
		iLeftRight, iLeftRight, iTopBottom, iTopBottom);
	GtkCssProvider* gtkCssProvider = gtk_css_provider_get_default();
	gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(gtkCssProvider), str, -1, NULL);
	GtkStyleContext* gtkStyleContext = gtk_widget_get_style_context(gtkWidget);
	gtk_style_context_add_provider(gtkStyleContext, GTK_STYLE_PROVIDER(gtkCssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_free(str);
}

void
GtkWidget_SetBorder(GtkWidget* gtkWidget, int iBorder)
{
	char* str = g_strdup_printf("GtkWidget { border: %dpx; }", iBorder);
	GtkCssProvider* gtkCssProvider = gtk_css_provider_get_default();
	gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(gtkCssProvider), str, -1, NULL);
	GtkStyleContext* gtkStyleContext = gtk_widget_get_style_context(gtkWidget);
	gtk_style_context_add_provider(gtkStyleContext, GTK_STYLE_PROVIDER(gtkCssProvider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_free(str);
}

void
MessageDialog(char* sMessage)
{
	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, sMessage);
	gtk_dialog_run(GTK_DIALOG(gtkDialog));
	gtk_widget_destroy(gtkDialog);
}

void
ErrorDialog(char* sMessage)
{
	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, sMessage);
	gtk_window_set_title(GTK_WINDOW(gtkDialog), "Error");
	gtk_dialog_run(GTK_DIALOG(gtkDialog));
	gtk_widget_destroy(gtkDialog);
}

void
PythonErrorDialog(char* sMessage)
{
	PyErr_Print();
	PyObject *pyType, *pyValue, *pyTraceback;
	PyErr_Fetch(&pyType, &pyValue, &pyTraceback);

	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		"in Python script\n%s", sMessage);
	gtk_window_set_title(GTK_WINDOW(gtkDialog), "Error");
	//gtk_message_dialog_format_secondary_text(gtkDialog, "Python error\n%s", PyUnicode_AsUTF8(pyValue));
	gtk_dialog_run(GTK_DIALOG(gtkDialog));
	gtk_widget_destroy(gtkDialog);
}

void
y(char* sText, PyObject* pyObject)
{
	if (pyObject) {
		PyObject* pyRepresentation = PyObject_Repr(pyObject);
		const char* sRepresentation = PyUnicode_AsUTF8(pyRepresentation);
		g_debug("%s | %s Ref Count: %d", sText, sRepresentation, pyObject->ob_refcnt);
		Py_DECREF(pyRepresentation);
	}
	else {
		g_debug("%s | NULL Object", sText);
	}
}

int
main(int argc, char** argv)
{
	int iResult;
	Py_SetProgramName(argv[0]);
	g.gtkApp = gtk_application_new("org.wanderlust", G_APPLICATION_FLAGS_NONE);
	g_object_set(g.gtkApp, "register-session", TRUE, NULL);

	g.gQuark = g_quark_from_static_string("Wanderlust");

	// initialize EggMarkdown
	g.pDownmarker = egg_markdown_new();
	egg_markdown_set_output(g.pDownmarker, EGG_MARKDOWN_OUTPUT_PANGO);
	egg_markdown_set_escape(g.pDownmarker, TRUE);
	//egg_markdown_set_autocode(g.pDownmarker, TRUE);
	egg_markdown_set_smart_quoting(g.pDownmarker, TRUE);

	g_signal_connect(g.gtkApp, "activate", G_CALLBACK(GtkAppActivateEventCB), NULL);
	iResult = g_application_run(G_APPLICATION(g.gtkApp), argc, argv);
	g_object_unref(g.gtkApp);
	//g_object_unref(G_OBJECT(g.gdkPixbufLoader));

	return iResult;
}
