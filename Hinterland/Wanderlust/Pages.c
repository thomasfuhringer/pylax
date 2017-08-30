// Pages.c  | Wanderlust © 2017 by Thomas Führinger

#include "Wanderlust.h"

// forward declarations
void Session_InitTreeOrg(ClientSession* pSession);
void Session_InitTreeKeyWord(ClientSession* pSession);
static void Session_DestroyCB(GtkWidget* gtkWidget, gpointer pUserData);
static void Session_EditPage(ClientSession* pSession, gboolean bNew);
static void MenuItemDelete_ActivateCB(GtkWidget* gtkMenuItem, ClientSession* pSession);
static void SessionSelectPerson_ClickedCB(GtkButton* gtkButton, ClientSession* pSession);
static void Session_RenderPage(ClientSession* pSession);
static void Session_ClearPage(ClientSession* pSession);
static void TreeView_SelectionChangedOrgCB(GtkWidget* gtkWidget, ClientSession* pSession);
static void TreeView_SelectionChangedKeyWordCB(GtkWidget* gtkWidget, ClientSession* pSession);
static Py_ssize_t TreeIter_PopulateChildrenOrg(GtkTreeIter* gtkTreeIter, ClientSession* pSession, gint iID);
static Py_ssize_t TreeIter_PopulateChildrenKeyWord(GtkTreeIter* gtkTreeIter, ClientSession* pSession, gint iID);

enum {
	PAGETREECOLUMN_ID = 0,
	PAGETREECOLUMN_KEYWORD,
	PAGETREE_NUM_COLS
};

enum {
	ORGTREECOLUMN_ID = 0,
	ORGTREECOLUMN_NAME,
	ORGTREE_NUM_COLS
};

static void
MenuItemEdit_ActivateCB(GtkWidget* gtkMenuItem, ClientSession* pSession)
{
	Session_EditPage(pSession, FALSE);
}

static void
MenuItemDelete_ActivateCB(GtkWidget* gtkMenuItem, ClientSession* pSession)
{
	GtkTreeIter gtkTreeIter;
	GtkTreeModel* gtkTreeModel;
	GtkTreeSelection* gtkTreeSelection;
	PyObject* pyID;

	GtkWidget* gtkDialog = gtk_message_dialog_new(g.gtkMainWindow, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, "Delete page and all children?");
	//gtk_window_set_title(GTK_WINDOW(gtkDialog), "Error");
	if (gtk_dialog_run(GTK_DIALOG(gtkDialog)) == GTK_RESPONSE_OK) {
		Py_XDECREF(g.pyMsg);
		g.pyMsg = PyDict_New();
		PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.DeletePage);
		pyID = PyLong_FromLong(pSession->iPage);
		PyDict_SetItemString(g.pyMsg, "ID", pyID);
y("ID", g.pyMsg);
		if (!Session_Exchange(pSession)) {
			ErrorDialog(pSession->sStatusMessage);
		}
		else {
			gtk_statusbar_push(g.gtkStatusbar, 1, "Page deleted.");
			gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSession->gtkTreeKeyWord));

			if (gtk_tree_selection_get_selected(gtkTreeSelection, &gtkTreeModel, &gtkTreeIter)) {
				gtk_tree_store_remove(gtkTreeModel, &gtkTreeIter);
			}
		}
	}
	gtk_widget_destroy(gtkDialog);
}

static void
MenuItemNew_ActivateCB(GtkWidget* gtkMenuItem, ClientSession* pSession)
{
	Session_EditPage(pSession, TRUE);
}

static void
TreeView_ShowPopUpMenu(GtkTreeView* gtkTreeView, ClientSession* pSession)
{
	GtkMenu* gtkMenu;
	GtkMenuItem* gtkMenuItemNew, *gtkMenuItemEdit, *gtkMenuItemDelete;
	gtkMenu = gtk_menu_new();

	if (pSession->iPage > -1) {
		gtkMenuItemEdit = gtk_menu_item_new_with_label("Edit");
		g_signal_connect(gtkMenuItemEdit, "activate", (GCallback)MenuItemEdit_ActivateCB, pSession);
		gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), gtkMenuItemEdit);
		gtkMenuItemDelete = gtk_menu_item_new_with_label("Delete");
		g_signal_connect(gtkMenuItemDelete, "activate", (GCallback)MenuItemDelete_ActivateCB, pSession);
		gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), gtkMenuItemDelete);
	}
	gtkMenuItemNew = gtk_menu_item_new_with_label("New");
	g_signal_connect(gtkMenuItemNew, "activate", (GCallback)MenuItemNew_ActivateCB, pSession);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), gtkMenuItemNew);

	gtk_widget_show_all(gtkMenu);
	gtk_menu_popup(GTK_MENU(gtkMenu), NULL, NULL, NULL, NULL, NULL, gtk_get_current_event_time());
}

gboolean
TreeView_PopupMenuCB(GtkTreeView* gtkTreeView, gpointer pUserData)
{
	TreeView_ShowPopUpMenu(gtkTreeView, pUserData);
	return TRUE;
}

gboolean
TreeView_ButtonPressCB(GtkWidget *gtkTreeView, GdkEventButton *gdkEventButton, gpointer pUserData)
{
	if (gdkEventButton->type == GDK_BUTTON_PRESS  &&  gdkEventButton->button == 3)
	{
		GtkTreeSelection* gtkTreeSelection;
		gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtkTreeView));
		if (gtk_tree_selection_count_selected_rows(gtkTreeSelection) <= 1)
		{
			GtkTreePath* gtkTreePath;
			if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(gtkTreeView),
				gdkEventButton->x, gdkEventButton->y,
				&gtkTreePath, NULL, NULL, NULL))
			{
				gtk_tree_selection_unselect_all(gtkTreeSelection);
				gtk_tree_selection_select_path(gtkTreeSelection, gtkTreePath);
				gtk_tree_path_free(gtkTreePath);
			}
		}
		TreeView_ShowPopUpMenu(gtkTreeView, pUserData);
		return TRUE;
	}
	return FALSE;
}

GtkWidget*
Session_ConstructPagesPane(ClientSession* pSession)
{
	pSession->iPageOrg = -1;
	pSession->iPagePerson = -1;
	pSession->iPage = -1;
	pSession->iPageParent = -1;
	pSession->iLanguage = -1;
	GtkTreeViewColumn* gtkTreeViewColumnKeyWord;
	GtkTreeViewColumn* gtkTreeViewColumnOrg;
	GtkCellRenderer* gtkCellRenderer;
	GtkTreeSelection* gtkTreeSelection;
	GtkTreeView* gtkTreeOrg;
	GtkWidget* gtkScrolledWindow;
	GtkWidget* gtkPagesMainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget* gtkPageViewVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(gtkPageViewVBox), 6);
	GtkWidget* gtkPageSelectVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(gtkPageSelectVBox), 6);

	pSession->gtkEntryPageAddress = gtk_entry_new();
	GtkWidget_SetBorder(pSession->gtkEntryPageAddress, 0);
	gtk_editable_set_editable(pSession->gtkEntryPageAddress, FALSE);
	gtk_box_pack_start(gtkPagesMainVBox, pSession->gtkEntryPageAddress, FALSE, FALSE, 0);
	gtk_box_pack_start(gtkPagesMainVBox, gtk_frame_new(NULL), FALSE, FALSE, 0); // line

	GtkWidget* gtkPanedPages = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_position(gtkPanedPages, 150);
	gtk_box_pack_start(gtkPagesMainVBox, gtkPanedPages, TRUE, TRUE, 0);

	GtkWidget* gtkPanedSelectPage = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_set_position(gtkPanedSelectPage, 120);

	// Organization tree view
	pSession->gtkTreeStoreOrg = gtk_tree_store_new(ORGTREE_NUM_COLS, G_TYPE_INT, G_TYPE_STRING);
	gtkTreeOrg = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pSession->gtkTreeStoreOrg));
	g_object_unref(pSession->gtkTreeStoreOrg);
	gtkTreeViewColumnOrg = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumnOrg, "Organization");
	gtk_tree_view_append_column(GTK_TREE_VIEW(gtkTreeOrg), gtkTreeViewColumnOrg);
	gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtkTreeOrg));
	g_signal_connect(gtkTreeSelection, "changed", G_CALLBACK(TreeView_SelectionChangedOrgCB), pSession);
	//g_signal_connect(gtkTreeOrg, "button-press-event", (GCallback)TreeView_ButtonPressCB, pSession);
	//g_signal_connect(gtkTreeOrg, "popup-menu", (GCallback)TreeView_PopupMenuCB, pSession);

	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumnOrg, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumnOrg, gtkCellRenderer, "text", ORGTREECOLUMN_NAME);
	//gtk_box_pack_start(gtkPageSelectVBox, gtkTreeOrg, TRUE, TRUE, 0);

	gtkScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(gtkScrolledWindow, 70, 80);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(gtkScrolledWindow), gtkTreeOrg);
	gtk_paned_pack1(gtkPanedSelectPage, gtkScrolledWindow, FALSE, FALSE);

	// Person view
	GtkWidget* gtkPersonViewHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkButton* gtkButtonRefresh = gtk_button_new();
	gtk_button_set_image(gtkButtonRefresh, g.gtkImagePerson);
	gtk_widget_set_tooltip_text(gtkButtonRefresh, "Select Person");
	//gtk_button_set_relief(gtkButtonRefresh, GTK_RELIEF_NONE);
	//gtk_button_set_focus_on_click(gtkButtonRefresh, FALSE);
	g_signal_connect(G_OBJECT(gtkButtonRefresh), "clicked", G_CALLBACK(SessionSelectPerson_ClickedCB), pSession);
	gtk_box_pack_start(gtkPersonViewHBox, gtkButtonRefresh, FALSE, FALSE, 0);
	pSession->gtkLabelPagePerson = gtk_label_new(NULL);
	gtk_box_pack_start(gtkPersonViewHBox, pSession->gtkLabelPagePerson, TRUE, TRUE, 0);
	gtk_widget_set_halign(pSession->gtkLabelPagePerson, 1.0);
	gtk_box_pack_start(gtkPageSelectVBox, gtkPersonViewHBox, FALSE, FALSE, 10);

	// Key Word tree view
	pSession->gtkTreeStoreKeyWord = gtk_tree_store_new(PAGETREE_NUM_COLS, G_TYPE_INT, G_TYPE_STRING);
	pSession->gtkTreeKeyWord = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pSession->gtkTreeStoreKeyWord));
	g_object_unref(pSession->gtkTreeStoreKeyWord);
	gtkTreeViewColumnKeyWord = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumnKeyWord, "Key Word");
	gtk_tree_view_append_column(GTK_TREE_VIEW(pSession->gtkTreeKeyWord), gtkTreeViewColumnKeyWord);
	gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSession->gtkTreeKeyWord));
	g_signal_connect(gtkTreeSelection, "changed", G_CALLBACK(TreeView_SelectionChangedKeyWordCB), pSession);
	g_signal_connect(pSession->gtkTreeKeyWord, "button-press-event", (GCallback)TreeView_ButtonPressCB, pSession);
	g_signal_connect(pSession->gtkTreeKeyWord, "popup-menu", (GCallback)TreeView_PopupMenuCB, pSession);

	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumnKeyWord, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumnKeyWord, gtkCellRenderer, "text", PAGETREECOLUMN_KEYWORD);

	gtkScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(gtkScrolledWindow, 70, 80);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(gtkScrolledWindow), pSession->gtkTreeKeyWord);
	gtk_box_pack_start(gtkPageSelectVBox, gtkScrolledWindow, TRUE, TRUE, 0);
	gtk_paned_pack2(gtkPanedSelectPage, gtkPageSelectVBox, TRUE, TRUE);
	gtk_paned_pack1(gtkPanedPages, gtkPanedSelectPage, TRUE, TRUE);

	pSession->gtkLabelPageHeading = gtk_label_new(NULL);
	PangoAttrList* pAttrList = pango_attr_list_new();
	PangoAttribute* pAttribute = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
	pango_attr_list_insert(pAttrList, pAttribute);
	gtk_label_set_attributes(pSession->gtkLabelPageHeading, pAttrList);
	gtk_box_pack_start(gtkPageViewVBox, pSession->gtkLabelPageHeading, FALSE, FALSE, 0);
	pSession->gtkImagePageImage = gtk_image_new_from_pixbuf(g.gdkPixbufPlaceHolder);
	gtk_box_pack_start(gtkPageViewVBox, pSession->gtkImagePageImage, FALSE, TRUE, 0);

	gtkScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	pSession->gtkTextBufferPageText = gtk_text_buffer_new(NULL);
	GtkWidget* gtkTextView = gtk_text_view_new_with_buffer(pSession->gtkTextBufferPageText);
	gtk_container_add(GTK_CONTAINER(gtkScrolledWindow), gtkTextView);
	gtk_box_pack_start(gtkPageViewVBox, gtkScrolledWindow, TRUE, TRUE, 0);
	gtk_paned_pack2(gtkPanedPages, gtkPageViewVBox, TRUE, TRUE);

	gtk_widget_show_all(gtkPagesMainVBox);
	return gtkPagesMainVBox;
}

void
Session_InitTreeOrg(ClientSession* pSession)
{
	gtk_tree_store_clear(pSession->gtkTreeStoreOrg);
	TreeIter_PopulateChildrenOrg(NULL, pSession, -1);
}

void
Session_InitTreeKeyWord(ClientSession* pSession)
{
	GtkTreePath* gtkTreePath;
	GtkTreeSelection* gtkTreeSelection;
	gtk_tree_store_clear(pSession->gtkTreeStoreKeyWord);
	if (TreeIter_PopulateChildrenKeyWord(NULL, pSession, -1) > 0) {
		gtkTreePath = gtk_tree_path_new_from_indices(0, -1);
		gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSession->gtkTreeKeyWord));
		gtk_tree_selection_select_path(gtkTreeSelection, gtkTreePath);
		gtk_tree_path_free(gtkTreePath);
	}
}

static void
TreeView_SelectionChangedOrgCB(GtkWidget* gtkWidget, ClientSession* pSession)
{
	GtkTreeIter gtkTreeIter, gtkTreeIterChild, gtkTreeIterParent;
	GtkTreeModel* gtkTreeModel;
	gint iID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtkWidget), &gtkTreeModel, &gtkTreeIter)) {
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, ORGTREECOLUMN_ID, &iID, -1);
		pSession->iPageOrg = iID;

		if (!gtk_tree_model_iter_has_child(gtkTreeModel, &gtkTreeIter)) {
			TreeIter_PopulateChildrenOrg(&gtkTreeIter, pSession, iID);
		}
	}
	else {
		pSession->iPageOrg = -1;
	}
	Session_InitTreeKeyWord(pSession);
}

static void
TreeView_SelectionChangedKeyWordCB(GtkWidget* gtkWidget, ClientSession* pSession)
{
	GtkTreeIter gtkTreeIter, gtkTreeIterChild, gtkTreeIterParent;
	GtkTreeModel* gtkTreeModel;
	gint iID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtkWidget), &gtkTreeModel, &gtkTreeIter)) {
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, PAGETREECOLUMN_ID, &iID, -1);
		g_free(pSession->sKeyWord);
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, PAGETREECOLUMN_KEYWORD, &pSession->sKeyWord, -1);
		if (gtk_tree_model_iter_parent(gtkTreeModel, &gtkTreeIterParent, &gtkTreeIter))
			gtk_tree_model_get(gtkTreeModel, &gtkTreeIterParent, PAGETREECOLUMN_ID, &pSession->iPageParent, -1);
		else
			pSession->iPageParent = -1;

		pSession->iPage = iID;
		Session_PullPage(pSession, iID);

		if (!gtk_tree_model_iter_has_child(gtkTreeModel, &gtkTreeIter)) {
			TreeIter_PopulateChildrenKeyWord(&gtkTreeIter, pSession, iID);
		}
	}
	else {
		Session_ClearPage(pSession);
		pSession->iPage = -1;
		pSession->iPageParent = -1;
	}
}

static Py_ssize_t
TreeIter_PopulateChildrenKeyWord(GtkTreeIter* gtkTreeIter, ClientSession* pSession, gint iID)
{
	GtkTreeIter gtkTreeIterChild;
	Py_ssize_t n, nLen;
	PyObject* pyData, *pyPage, *pyPageID, *pyPageKeyWord, *pyID;

	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.GetPageChildren);

	if (iID > -1) {
		pyID = PyLong_FromLong(iID);
		PyDict_SetItemString(g.pyMsg, "ID", pyID);
	}
	else {
		if (pSession->iPageOrg > -1) {
			pyID = PyLong_FromLong(pSession->iPageOrg);
			PyDict_SetItemString(g.pyMsg, "Org", pyID);
			//Py_DECREF(pyID);
		}
		if (pSession->iPagePerson > -1) {
			pyID = PyLong_FromLong(pSession->iPagePerson);
			PyDict_SetItemString(g.pyMsg, "Person", pyID);
			//Py_DECREF(pyID);
		}
		if (pSession->iLanguage > -1) {
			pyID = PyLong_FromLong(pSession->iLanguage);
			PyDict_SetItemString(g.pyMsg, "Language", pyID);
			//Py_DECREF(pyID);
		}
	}

	if (Session_Exchange(pSession)) {
		pyData = PyDict_GetItemString(pSession->pyResultMsg, "Data");
		nLen = PySequence_Size(pyData);
		for (n = 0; n < nLen; n++) {
			pyPage = PySequence_ITEM(pyData, n);
			pyPageID = PySequence_ITEM(pyPage, 0);
			pyPageKeyWord = PySequence_ITEM(pyPage, 1);
			gtk_tree_store_append(pSession->gtkTreeStoreKeyWord, &gtkTreeIterChild, gtkTreeIter);
			gtk_tree_store_set(pSession->gtkTreeStoreKeyWord, &gtkTreeIterChild, PAGETREECOLUMN_ID, PyLong_AsLong(pyPageID), -1);
			gtk_tree_store_set(pSession->gtkTreeStoreKeyWord, &gtkTreeIterChild, PAGETREECOLUMN_KEYWORD, PyUnicode_AsString(pyPageKeyWord), -1);
			Py_DECREF(pyPage);
			Py_DECREF(pyPageID);
			Py_DECREF(pyPageKeyWord);
		}
	}
	else {
		PyObject* pyType = PyDict_GetItem(pSession->pyResultMsg, hlMsg.Type);
		if (pyType != hlMsg.NotFound)
			ErrorDialog(pSession->sStatusMessage);
		else
			Py_DECREF(pyType);
	}
	return nLen;
}

static Py_ssize_t
TreeIter_PopulateChildrenOrg(GtkTreeIter* gtkTreeIter, ClientSession* pSession, gint iID)
{
	//g_debug("child fer %p", gtkTreeIter);
	GtkTreeIter gtkTreeIterChild;
	Py_ssize_t n, nLen;
	PyObject* pyData, *pyOrg, *pyOrgID, *pyOrgName, *pyID;

	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.GetOrgChildren);

	if (iID > -1) {
		pyID = PyLong_FromLong(iID);
		PyDict_SetItemString(g.pyMsg, "ID", pyID);
	}

	if (Session_Exchange(pSession)) {
		pyData = PyDict_GetItemString(pSession->pyResultMsg, "Data");
		nLen = PySequence_Size(pyData);
		for (n = 0; n < nLen; n++) {
			pyOrg = PySequence_ITEM(pyData, n);
			pyOrgID = PySequence_ITEM(pyOrg, 0);
			pyOrgName = PySequence_ITEM(pyOrg, 1);
			//y("pyData", pyOrgName);
			gtk_tree_store_append(pSession->gtkTreeStoreOrg, &gtkTreeIterChild, gtkTreeIter);
			gtk_tree_store_set(pSession->gtkTreeStoreOrg, &gtkTreeIterChild, ORGTREECOLUMN_ID, PyLong_AsLong(pyOrgID), -1);
			gtk_tree_store_set(pSession->gtkTreeStoreOrg, &gtkTreeIterChild, ORGTREECOLUMN_NAME, PyUnicode_AsString(pyOrgName), -1);
			Py_DECREF(pyOrg);
			Py_DECREF(pyOrgID);
			Py_DECREF(pyOrgName);
		}
	}
	else {
		PyObject* pyType = PyDict_GetItem(pSession->pyResultMsg, hlMsg.Type);
		if (pyType != hlMsg.NotFound)
			ErrorDialog(pSession->sStatusMessage);
		else
			Py_DECREF(pyType);
	}
	return nLen;
}

static void GtkImageButton_ClickedCB(GtkButton* gtkWidget, gboolean* pbPictureSet);

gboolean
Session_PullHomePage(ClientSession* pSession)
{
	PyObject* pyID;

	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.GetPage);
	if (pSession->iPageOrg > -1) {
		pyID = PyLong_FromLong(pSession->iPageOrg);
		PyDict_SetItemString(g.pyMsg, "Org", pyID);
	}
	if (pSession->iPagePerson > -1) {
		pyID = PyLong_FromLong(pSession->iPagePerson);
		PyDict_SetItemString(g.pyMsg, "Person", pyID);
	}
	if (!Session_Exchange(pSession)) {
		ErrorDialog(pSession->sStatusMessage);
		return FALSE;
	}
	else {
		Session_RenderPage(pSession);
		return TRUE;
	}
}

gboolean
Session_PullPage(ClientSession* pSession, int iPage)
{
	PyObject* pyID;

	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.GetPage);
	pyID = PyLong_FromLong(iPage);
	PyDict_SetItemString(g.pyMsg, "ID", pyID);
	//Py_DECREF(pyID);

	if (!Session_Exchange(pSession)) {
		ErrorDialog(pSession->sStatusMessage);
		return FALSE;
	}
	else {
		Session_RenderPage(pSession);
		return TRUE;
	}
}

static void
Session_ClearPage(ClientSession* pSession)
{
	gtk_entry_set_text(pSession->gtkEntryPageAddress, "");
	gtk_widget_hide(pSession->gtkLabelPageHeading);
	gtk_text_buffer_set_text(pSession->gtkTextBufferPageText, "", 0);
	gtk_widget_hide(pSession->gtkImagePageImage);
}

static void
Session_RenderPage(ClientSession* pSession)
{
	PyObject* pyData, *pyMsgData;
	GdkPixbuf* gdkPixbuf;
GdkPixbufLoader* gdkPixbufLoader;
	char* sBuffer;

	pyMsgData = PyDict_GetItemString(pSession->pyResultMsg, "Data");

	pyData = PyDict_GetItemStringX(pyMsgData, "Address");
	if (PyUnicode_Check(pyData)) {
		gtk_entry_set_text(pSession->gtkEntryPageAddress, PyUnicode_AsUTF8(pyData));
	}

	pyData = PyDict_GetItemStringX(pyMsgData, "Heading");
	if (PyUnicode_Check(pyData)) {
		gtk_label_set_text(pSession->gtkLabelPageHeading, PyUnicode_AsUTF8(pyData));
		gtk_widget_show(pSession->gtkLabelPageHeading);
	}
	else
		gtk_widget_hide(pSession->gtkLabelPageHeading);

	pyData = PyDict_GetItemStringX(pyMsgData, "Text");
	if (PyUnicode_Check(pyData)) {
		gtk_text_buffer_set_markup(pSession->gtkTextBufferPageText, PyUnicode_AsUTF8(pyData));
	}

	pyData = PyDict_GetItemStringX(pyMsgData, "Data");
	if (PyBytes_CheckExact(pyData)) {
		sBuffer = PyBytes_AsString(pyData);
	    gdkPixbufLoader = gdk_pixbuf_loader_new();
		if (!gdk_pixbuf_loader_write(gdkPixbufLoader, sBuffer, PyBytes_Size(pyData), NULL)) {
			g_debug("Error loading image buffer");
			return;
		}
		gdkPixbuf = gdk_pixbuf_loader_get_pixbuf(gdkPixbufLoader);
		gtk_image_set_from_pixbuf(pSession->gtkImagePageImage, gdkPixbuf);
		gtk_widget_show(pSession->gtkImagePageImage);
        gdk_pixbuf_loader_close(gdkPixbufLoader,NULL);
        g_object_unref(gdkPixbufLoader);
	}
	else
		gtk_widget_hide(pSession->gtkImagePageImage);
}

static void
Session_EditPage(ClientSession* pSession, gboolean bNew)
{
	GtkWidget* gtkDialog, *gtkDialogContentArea;
	GtkImage* gtkImage;
	GtkButton* gtkButton;
	GtkLabel* gtkLabel, *gtkLabelId;
	GtkEntry* gtkEntryKeyWord, *gtkEntryHeading;
	GtkTextView* gtkTextView;
	GtkTextBuffer* gtkTextBuffer;
	GtkTextIter gtkTextIterStart, gtkTextIterEnd;
	GdkPixbuf* gdkPixbuf = NULL;
    GdkPixbufLoader* gdkPixbufLoader;
	gboolean bPictureSet = FALSE;
	char* sPictureBuffer = NULL;
	char* sTextBuffer = NULL;
	char* sKeyWord = NULL;
	GError* gError = NULL;
	gsize nSize;
	PyObject* pyPageID = NULL;
	PyObject* pyData, *pyMsgData;

	if (!bNew) {
		Py_XDECREF(g.pyMsg);
		g.pyMsg = PyDict_New();
		PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.GetPage);
		pyPageID = PyLong_FromLong(pSession->iPage);
		PyDict_SetItemString(g.pyMsg, "ID", pyPageID);
		//Py_DECREF(pyID);

		if (!Session_Exchange(pSession)) {
			ErrorDialog(pSession->sStatusMessage);
			return;
		}
	}

	gtkDialog = gtk_dialog_new_with_buttons("Page", g.gtkMainWindow, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);
	gtk_window_set_default_size(gtkDialog, 480, 560);
	gtkDialogContentArea = gtk_dialog_get_content_area(GTK_DIALOG(gtkDialog));

	GtkGrid* gtkGrid = gtk_grid_new();
	gtk_grid_set_row_spacing(gtkGrid, 2);

	gtkButton = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(gtkButton), GTK_RELIEF_NONE);

	if (!bNew) {
		pyMsgData = PyDict_GetItemString(pSession->pyResultMsg, "Data");
		pyData = PyDict_GetItemStringX(pyMsgData, "Data");
		if (PyBytes_CheckExact(pyData)) {
			char* sBuffer = PyBytes_AsString(pyData);
	        gdkPixbufLoader = gdk_pixbuf_loader_new();
			if (!gdk_pixbuf_loader_write(gdkPixbufLoader, sBuffer, PyBytes_Size(pyData), NULL)) {
				g_debug("Error loading image buffer");
				return;
			}
			gdkPixbuf = gdk_pixbuf_loader_get_pixbuf(gdkPixbufLoader);
			gtkImage = gtk_image_new_from_pixbuf(gdkPixbuf);
            gdk_pixbuf_loader_close(gdkPixbufLoader,NULL);
	        g_object_unref(G_OBJECT(gdkPixbufLoader));
			bPictureSet = TRUE;
		}
		else
			gtkImage = gtk_image_new_from_pixbuf(g.gdkPixbufPlaceHolder);
	}
	else
		gtkImage = gtk_image_new_from_pixbuf(g.gdkPixbufPlaceHolder);

	g_object_set(gtkImage, "halign", GTK_ALIGN_CENTER, "valign", GTK_ALIGN_CENTER, NULL);
	gtk_button_set_image(GTK_BUTTON(gtkButton), gtkImage);
	gtk_button_set_image_position(GTK_BUTTON(gtkButton), GTK_POS_LEFT);
	g_signal_connect(G_OBJECT(gtkButton), "clicked", G_CALLBACK(GtkImageButton_ClickedCB), &bPictureSet);
	gdkPixbuf = gtk_image_get_pixbuf(gtkImage);

	GtkWidget* gtkTopHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gtkLabel = gtk_label_new("Key Word");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_box_pack_start(gtkTopHBox, gtkLabel, FALSE, FALSE, 0);
	gtkEntryKeyWord = gtk_entry_new();
	gtk_widget_set_sensitive(gtkEntryKeyWord, bNew);
	gtk_box_pack_start(gtkTopHBox, gtkEntryKeyWord, TRUE, TRUE, 10);

	gtkLabel = gtk_label_new("ID");
	gtk_box_pack_start(gtkTopHBox, gtkLabel, FALSE, FALSE, 10);
	gtkLabelId = gtk_label_new(NULL);
	gtk_widget_set_halign(gtkLabelId, 1.0);
	gtk_box_pack_start(gtkTopHBox, gtkLabelId, FALSE, FALSE, 0);

	gtk_grid_attach_next_to(gtkGrid, gtkTopHBox, NULL, GTK_POS_LEFT, 2, 1);

	gtkLabel = gtk_label_new("Heading");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_widget_set_margin_top(gtkLabel, 10);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkTopHBox, GTK_POS_BOTTOM, 1, 1);
	gtkEntryHeading = gtk_entry_new();
	gtk_grid_attach_next_to(gtkGrid, gtkEntryHeading, gtkLabel, GTK_POS_BOTTOM, 1, 1);

	gtkLabel = gtk_label_new("Picture");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_widget_set_margin_top(gtkLabel, 10);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkEntryHeading, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkButton, gtkLabel, GTK_POS_BOTTOM, 1, 1);

	gtkLabel = gtk_label_new("Body");
	gtk_widget_set_halign(gtkLabel, 1.0);
	gtk_widget_set_margin_top(gtkLabel, 10);
	GtkWidget* gtkScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(gtkScrolledWindow, 400, 120);
	gtk_widget_set_vexpand(gtkScrolledWindow, TRUE);
	gtk_widget_set_hexpand(gtkScrolledWindow, TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtkTextBuffer = gtk_text_buffer_new(NULL);
	gtkTextView = gtk_text_view_new_with_buffer(gtkTextBuffer);
	gtk_container_add(GTK_CONTAINER(gtkScrolledWindow), gtkTextView);
	gtk_grid_attach_next_to(gtkGrid, gtkLabel, gtkButton, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(gtkGrid, gtkScrolledWindow, gtkLabel, GTK_POS_BOTTOM, 1, 1);

	if (!bNew) {
		pyData = PyDict_GetItemStringX(pyMsgData, "ID");
		if (PyLong_Check(pyData)) {
			char sID[30];
			sprintf(sID, "%d", PyLong_AsLong(pyData));
			gtk_label_set_text(gtkLabelId, sID);
		}
		pyData = PyDict_GetItemStringX(pyMsgData, "KeyWord");
		if (PyUnicode_Check(pyData))
			gtk_entry_set_text(gtkEntryKeyWord, PyUnicode_AsUTF8(pyData));
		pyData = PyDict_GetItemStringX(pyMsgData, "Heading");
		if (PyUnicode_Check(pyData))
			gtk_entry_set_text(gtkEntryHeading, PyUnicode_AsUTF8(pyData));

		pyData = PyDict_GetItemStringX(pyMsgData, "Text");
		if (PyUnicode_Check(pyData))
			gtk_text_buffer_set_text(gtkTextBuffer, PyUnicode_AsUTF8(pyData), -1);
	}

	gtk_container_add(GTK_CONTAINER(gtkDialogContentArea), gtkGrid);
	gtk_container_set_border_width(GTK_CONTAINER(gtkDialogContentArea), 10);
	gtk_container_set_border_width(GTK_CONTAINER(gtkGrid), 6);
	gtk_widget_show_all(gtkDialog);

	PyObject* pyImageData = NULL;
	if (gtk_dialog_run(GTK_DIALOG(gtkDialog)) == GTK_RESPONSE_ACCEPT) {
		g.pyMsg = PyDict_New();
		PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.SetPage);

		if (!bNew) {
			pyPageID = PyLong_FromLong(pSession->iPage);
			PyDict_SetItemString(g.pyMsg, "ID", pyPageID);
			if (pSession->iPageParent > -1) {
				pyPageID = PyLong_FromLong(pSession->iPageParent);
				PyDict_SetItemString(g.pyMsg, "Parent", pyPageID);
			}
		}
		else {
			if (pSession->iPage > -1) {
				pyPageID = PyLong_FromLong(pSession->iPage);
				PyDict_SetItemString(g.pyMsg, "Parent", pyPageID);
			}
		}
		if (pSession->iPagePerson > -1) {
			pyPageID = PyLong_FromLong(pSession->iPagePerson);
			PyDict_SetItemString(g.pyMsg, "Person", pyPageID);
		}
		if (pSession->iPageOrg > -1) {
			pyPageID = PyLong_FromLong(pSession->iPageOrg);
			PyDict_SetItemString(g.pyMsg, "Org", pyPageID);
		}
		if (pSession->iLanguage > -1) {
			pyPageID = PyLong_FromLong(pSession->iLanguage);
			PyDict_SetItemString(g.pyMsg, "Language", pyPageID);
		}
		sKeyWord = gtk_entry_get_text(gtkEntryKeyWord);
		if (strlen(sKeyWord) > 0) {
			PyDict_SetItemString(g.pyMsg, "KeyWord", PyUnicode_FromString(sKeyWord));
		}
		sKeyWord = gtk_entry_get_text(gtkEntryHeading);
		if (strlen(sKeyWord) > 0) {
			PyDict_SetItemString(g.pyMsg, "Heading", PyUnicode_FromString(sKeyWord));
		}

		gtk_text_buffer_get_bounds(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd);
		sTextBuffer = gtk_text_buffer_get_text(gtkTextBuffer, &gtkTextIterStart, &gtkTextIterEnd, FALSE);
		PyDict_SetItemString(g.pyMsg, "Text", PyUnicode_FromString(sTextBuffer));

		if (bPictureSet) {
			gdkPixbuf = gtk_image_get_pixbuf(gtkImage);
			if (!gdk_pixbuf_save_to_buffer(gdkPixbuf, &sPictureBuffer, &nSize, "jpeg", &gError, NULL)) {
				g_printerr("%s\n", gError->message);
				g_error_free(gError);
				return false;
			}
			g_assert(sPictureBuffer != NULL);

			pyImageData = PyBytes_FromStringAndSize(sPictureBuffer, nSize);
			g_free(sPictureBuffer);
			PyDict_SetItemString(g.pyMsg, "Data", pyImageData);
		}

		if (!Session_Exchange(pSession)) {
			ErrorDialog(pSession->sStatusMessage);
		}
		else {
			gtk_statusbar_push(g.gtkStatusbar, 1, "Page posted.");
		}
		//Py_XDECREF(pyPageID);
		if (bNew) {
			GtkTreeStore gtkTreeModel;
			GtkTreeIter gtkTreeIter;
			GtkTreeSelection* gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSession->gtkTreeKeyWord));
			if (gtk_tree_selection_get_selected(gtkTreeSelection, &gtkTreeModel, &gtkTreeIter))
				TreeIter_PopulateChildrenKeyWord(&gtkTreeIter, pSession, pSession->iPage);
		}
	}
	gtk_widget_destroy(gtkDialog);
	if (!bNew)
		Session_PullPage(pSession, pSession->iPage);
}

static void
GtkImageButton_ClickedCB(GtkButton* gtkButton, gboolean* pbPictureSet)
{
	GtkWidget* gtkFileChooser;
	GdkPixbuf* gdkPixbuf = NULL;
	char* sFileName;
	GError* gError = NULL;
	gint iResponse;
	gsize nSize;
	PyObject* pyImageData = NULL;
	GtkImage* gtkImage = GTK_IMAGE(gtk_button_get_image(gtkButton));

	gtkFileChooser = gtk_file_chooser_dialog_new("Select Image", g.gtkMainWindow, GTK_FILE_CHOOSER_ACTION_OPEN,
		"_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_CLOSE, NULL); //GTK_RESPONSE_ACCEPT

	GtkFileFilter* gtkFileFilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gtkFileFilter, "*.jpg");
	gtk_file_filter_set_name(gtkFileFilter, "JPG");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkFileChooser), gtkFileFilter);

	iResponse = gtk_dialog_run(GTK_DIALOG(gtkFileChooser));
	if (iResponse == GTK_RESPONSE_CLOSE) {
		sFileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkFileChooser));
		if (sFileName == NULL) {
			gtk_image_set_from_pixbuf(gtkImage, g.gdkPixbufPlaceHolder);
			*pbPictureSet = FALSE;
		}
		else {
			gdkPixbuf = gdk_pixbuf_new_from_file_at_size(sFileName, PAGE_PICTURE_WIDTH, PAGE_PICTURE_HEIGHT, &gError);
			g_assert_no_error(gError);
			if (!gdkPixbuf) {
				ErrorDialog(gError->message);
				g_printerr("%s\n", gError->message);
				g_error_free(gError);
				return false;
			}
			gtk_image_set_from_pixbuf(gtkImage, gdkPixbuf);
			*pbPictureSet = TRUE;
			g_object_unref(gdkPixbuf);
			g_free(sFileName);
		}
	}

	//g_object_unref(G_OBJECT(gtkFileFilter));
	gtk_widget_destroy(gtkFileChooser);
}

static void
SessionSelectPerson_ClickedCB(GtkButton* gtkButton, ClientSession* pSession)
{
	gchar* sFirstName = NULL;
	gchar* sLastName = NULL;
	gchar* sFullName = NULL;
	if (SelectPerson(pSession, &pSession->iPagePerson, &sFirstName, &sLastName)) {
		if (sFirstName == NULL)
			gtk_label_set_text(pSession->gtkLabelPagePerson, sLastName);
		else {
			sFullName = g_strconcat(sFirstName, " ", sLastName, NULL);
			gtk_label_set_text(pSession->gtkLabelPagePerson, sFullName);
			g_free(sFullName);
			g_free(sFirstName);
		}
		g_free(sLastName);
		Session_InitTreeKeyWord(pSession);
		//Session_PullHomePage(pSession);
	}
}
