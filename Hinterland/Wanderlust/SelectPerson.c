// SelectPerson.c  | Wanderlust © 2017 by Thomas Führinger

#include "Wanderlust.h"

// forward declarations


enum {
	TREECOLUMN_ID = 0,
	TREECOLUMN_HANDLE,
	TREECOLUMN_FIRSTNAME,
	TREECOLUMN_LASTNAME,
	TREE_NUM_COLS
};

typedef struct _Data
{
	ClientSession* pSession;
	GtkWidget* gtkEntryName;
	GtkTreeStore* gtkTreeStore;
	int iSelection;
}
Data;

static void
Search_ClickedCB(GtkButton* gtkButton, Data* pData)
{
	Py_ssize_t n, nLen;
	PyObject* pyName, *pyData, *pyPerson, *pyPersonID, *pyPersonHandle, *pyPersonFirstName, *pyPersonLastName, *pyMsgResultType;
	GtkTreeIter gtkTreeIter;

	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.SearchPerson);
	pyName = PyUnicode_FromFormat("%s", gtk_entry_get_text(pData->gtkEntryName));
	PyDict_SetItemString(g.pyMsg, "Name", pyName);
	//Py_DECREF(pyName);
	gtk_list_store_clear(pData->gtkTreeStore);

	if (!Session_Exchange(pData->pSession)) {
		pyMsgResultType = PyDict_GetItem(pData->pSession->pyResultMsg, hlMsg.Type);
		if (pyMsgResultType == hlMsg.NotFound) {
			g_debug("Nothing found");
		}
		else
			ErrorDialog(pData->pSession->sStatusMessage);
	}
	else {
		pyData = PyDict_GetItemString(pData->pSession->pyResultMsg, "Data");
		nLen = PySequence_Size(pyData);
		for (n = 0; n < nLen; n++) {
			pyPerson = PySequence_ITEM(pyData, n);
			pyPersonID = PySequence_ITEM(pyPerson, 0);
			pyPersonHandle = PySequence_ITEM(pyPerson, 1);
			pyPersonFirstName = PySequence_ITEM(pyPerson, 2);
			pyPersonLastName = PySequence_ITEM(pyPerson, 4);
			//y("Pers", pyPersonLastName);
			//char *p = PyUnicode_AsUTF8(pyPersonLastName);
			gtk_list_store_append(pData->gtkTreeStore, &gtkTreeIter);
			gtk_list_store_set(pData->gtkTreeStore, &gtkTreeIter, TREECOLUMN_ID, PyLong_AsLong(pyPersonID), -1);
			gtk_list_store_set(pData->gtkTreeStore, &gtkTreeIter, TREECOLUMN_HANDLE, PyUnicode_AsUTF8(pyPersonHandle), -1);
			gtk_list_store_set(pData->gtkTreeStore, &gtkTreeIter, TREECOLUMN_FIRSTNAME, PyUnicode_AsUTF8(pyPersonFirstName), -1);
			gtk_list_store_set(pData->gtkTreeStore, &gtkTreeIter, TREECOLUMN_LASTNAME, PyUnicode_AsUTF8(pyPersonLastName), -1);
			Py_DECREF(pyPerson);
			Py_DECREF(pyPersonID);
			Py_DECREF(pyPersonHandle);
			Py_DECREF(pyPersonFirstName);
			Py_DECREF(pyPersonLastName);
		}
	}
}

static void
TreeView_SelectionChangedCB(GtkWidget* gtkWidget, Data* pData)
{
	GtkTreeIter gtkTreeIter;
	GtkTreeModel* gtkTreeModel;
	gint iID;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtkWidget), &gtkTreeModel, &gtkTreeIter)) {
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, TREECOLUMN_ID, &iID, -1);
		pData->iSelection = iID;
	}
	else {
		pData->iSelection = -1;
	}
}

static void
TreeView_RowActivatedCB(GtkTreeView* gtkTreeView, GtkTreePath* gtkTreePath, GtkTreeViewColumn* gtkTreeViewColumn, GtkDialog* gtkDialog)
{
	gtk_dialog_response(gtkDialog, GTK_RESPONSE_ACCEPT);
}


gboolean
SelectPerson(ClientSession* pSession, int* piPerson, char** psFirstName, char** psLastName)
{
	GtkWidget* gtkDialog, *gtkDialogContentArea;
	GtkLabel* gtkLabel;
	GtkEntry* gtkEntryHandle, *gtkEntryFirstName, *gtkEntryLastName, *gtkEntryPassword;
	PyObject* pyResult;
	//char* sHandle;
	GtkTreeViewColumn* gtkTreeViewColumnHandle, *gtkTreeViewColumnFirstName, *gtkTreeViewColumnLastName;
	GtkCellRenderer* gtkCellRenderer;
	GtkTreeSelection* gtkTreeSelection;
	GtkTreeView* gtkTree;

	Data data;
	data.pSession = pSession;
	data.iSelection = -2;   // canceled
	GtkTreeIter gtkTreeIter;
	gboolean bResponse = FALSE;

	//ClientSession* pSession = (ClientSession*)pUserData;
	gtkDialog = gtk_dialog_new_with_buttons("Select Person", g.gtkMainWindow, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);
	gtk_window_set_default_size(GTK_WINDOW(gtkDialog), 400, 400);
	gtk_widget_set_size_request(GTK_WINDOW(gtkDialog), 360, 320);
	gtkDialogContentArea = gtk_dialog_get_content_area(GTK_DIALOG(gtkDialog));

	GtkWidget* gtkMainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkWidget* gtkTopHBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

	data.gtkEntryName = gtk_entry_new();
	gtk_widget_set_hexpand(data.gtkEntryName, TRUE);
	gtk_box_pack_start(gtkTopHBox, data.gtkEntryName, TRUE, TRUE, 0);

	GtkButton* gtkButtonRefresh = gtk_button_new();
	gtk_button_set_image(gtkButtonRefresh, g.gtkImageFind);
	g_object_ref(g.gtkImageFind);
	gtk_widget_set_tooltip_text(gtkButtonRefresh, "Search");
	//gtk_button_set_relief(gtkButtonRefresh, GTK_RELIEF_NONE);
	//gtk_button_set_focus_on_click(gtkButtonRefresh, FALSE);
	g_signal_connect(G_OBJECT(gtkButtonRefresh), "clicked", G_CALLBACK(Search_ClickedCB), &data);
	gtk_box_pack_start(gtkTopHBox, gtkButtonRefresh, FALSE, FALSE, 0);
	gtk_box_pack_start(gtkMainVBox, gtkTopHBox, FALSE, FALSE, 0);

	data.gtkTreeStore = gtk_list_store_new(TREE_NUM_COLS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtkTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(data.gtkTreeStore));
	g_object_unref(data.gtkTreeStore);
	gtkTreeViewColumnHandle = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumnHandle, "Handle");
	gtk_tree_view_column_set_resizable(gtkTreeViewColumnHandle, TRUE);
	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumnHandle, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumnHandle, gtkCellRenderer, "text", TREECOLUMN_HANDLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gtkTree), gtkTreeViewColumnHandle);
	gtkTreeViewColumnFirstName = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumnFirstName, "First Name");
	gtk_tree_view_column_set_resizable(gtkTreeViewColumnFirstName, TRUE);
	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumnFirstName, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumnFirstName, gtkCellRenderer, "text", TREECOLUMN_FIRSTNAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gtkTree), gtkTreeViewColumnFirstName);
	gtkTreeViewColumnLastName = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumnLastName, "Last Name");
	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumnLastName, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumnLastName, gtkCellRenderer, "text", TREECOLUMN_LASTNAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gtkTree), gtkTreeViewColumnLastName);
	gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtkTree));
	g_signal_connect(gtkTree, "row-activated", G_CALLBACK(TreeView_RowActivatedCB), gtkDialog);
	//g_signal_connect(gtkTreeSelection, "changed", G_CALLBACK(TreeView_SelectionChangedCB), &data.pSession);

	GtkWidget* gtkScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(gtkScrolledWindow, TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(gtkScrolledWindow), gtkTree);


	gtk_container_set_border_width(GTK_CONTAINER(gtkMainVBox), 6);
	gtk_box_pack_start(gtkMainVBox, gtkScrolledWindow, TRUE, TRUE, 8);

	gtk_container_add(GTK_CONTAINER(gtkDialogContentArea), gtkMainVBox);
	gtk_container_set_border_width(GTK_CONTAINER(gtkDialogContentArea), 30);
	//gtk_container_set_border_width(GTK_CONTAINER(gtkGrid), 6);
	gtk_widget_show_all(gtkDialog);

	if (gtk_dialog_run(GTK_DIALOG(gtkDialog)) == GTK_RESPONSE_ACCEPT) {
		if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtkTreeSelection), &data.gtkTreeStore, &gtkTreeIter)) {
			gtk_tree_model_get(GTK_TREE_MODEL(data.gtkTreeStore), &gtkTreeIter, TREECOLUMN_ID, piPerson,
				TREECOLUMN_FIRSTNAME, psFirstName,
				TREECOLUMN_LASTNAME, psLastName, -1);
		}
		else {
			*piPerson = -1;
		}
		bResponse = TRUE;
	}
	gtk_widget_destroy(gtkDialog);
	return bResponse;
}
