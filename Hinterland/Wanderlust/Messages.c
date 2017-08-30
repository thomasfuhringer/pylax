// Messages.c  | Wanderlust © 2017 by Thomas Führinger

#include "Wanderlust.h"

// forward declarations

enum {
	MSGLISTCOLUMN_ID = 0,
	MSGLISTCOLUMN_SENDER,
	MSGLISTCOLUMN_HEADING,
	MSGLISTCOLUMN_TEXT,
	MSGLISTCOLUMN_DATE,
	MSGLISTCOLUMN_ENCRYPTED,
	MSGLIST_NUM_COLS
};

static void
TreeView_SelectionChangedCB(GtkWidget* gtkWidget, ClientSession* pSession)
{
	GtkTreeIter gtkTreeIter;
	GtkTreeModel* gtkTreeModel;
	gint iID;
	gchar* sHeading = NULL;
	gchar* sText = NULL;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(gtkWidget), &gtkTreeModel, &gtkTreeIter)) {
		gtk_tree_model_get(gtkTreeModel, &gtkTreeIter, MSGLISTCOLUMN_ID, &iID, MSGLISTCOLUMN_HEADING, &sHeading, MSGLISTCOLUMN_TEXT, &sText, -1);
		gtk_label_set_text(pSession->gtkLabelMessageHeading, sHeading);
		gtk_text_buffer_set_markup(pSession->gtkTextBufferMessageText, sText);
		g_free(sHeading);
		g_free(sText);
	}
	else {
	}
}

GtkWidget*
Session_ConstructMessagesPane(ClientSession* pSession)
{
	GtkTreeSelection* gtkTreeSelection;
	GtkTreeViewColumn* gtkTreeViewColumn;
	GtkCellRenderer* gtkCellRenderer;

	GtkWidget* gtkMainVBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget* gtkPanedMessages = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_position(gtkPanedMessages, 300);

	pSession->gtkListStoreMessages = gtk_list_store_new(MSGLIST_NUM_COLS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	pSession->gtkTreeViewMessages = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pSession->gtkListStoreMessages));
	g_object_unref(pSession->gtkListStoreMessages);
	gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSession->gtkTreeViewMessages));
	g_signal_connect(gtkTreeSelection, "changed", G_CALLBACK(TreeView_SelectionChangedCB), pSession);

	gtkTreeViewColumn = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumn, "Sender");
	//gtk_tree_view_column_set_fixed_width(gtkTreeViewColumn, 90);
	//gtk_tree_view_column_set_resizable(gtkTreeViewColumn, TRUE);
	gtk_tree_view_column_set_expand(gtkTreeViewColumn, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pSession->gtkTreeViewMessages), gtkTreeViewColumn);
	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumn, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumn, gtkCellRenderer, "text", MSGLISTCOLUMN_SENDER);

	gtkTreeViewColumn = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumn, "Subject");
	gtk_tree_view_column_set_resizable(gtkTreeViewColumn, TRUE);
	gtk_tree_view_column_set_expand(gtkTreeViewColumn, TRUE);
	//gtk_tree_view_append_column(GTK_TREE_VIEW(pSession->gtkTreeViewMessages), gtkTreeViewColumn);
	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumn, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumn, gtkCellRenderer, "text", MSGLISTCOLUMN_HEADING);

	gtkTreeViewColumn = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(gtkTreeViewColumn, "Sent");
	gtk_tree_view_column_set_resizable(gtkTreeViewColumn, TRUE);
	gtk_tree_view_column_set_fixed_width(gtkTreeViewColumn, 90);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pSession->gtkTreeViewMessages), gtkTreeViewColumn);
	gtkCellRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumn, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumn, gtkCellRenderer, "text", MSGLISTCOLUMN_DATE);

	gtkTreeViewColumn = gtk_tree_view_column_new();
	GtkImage* gtkImage = gtk_image_new_from_pixbuf(g.gdkPixbufKey);
    gtk_widget_show (gtkImage);
    gtk_tree_view_column_set_widget(gtkTreeViewColumn, gtkImage);
	//gtk_tree_view_column_set_resizable(gtkTreeViewColumn, TRUE);
	gtk_tree_view_column_set_fixed_width(gtkTreeViewColumn, 30);
	gtk_tree_view_append_column(GTK_TREE_VIEW(pSession->gtkTreeViewMessages), gtkTreeViewColumn);
	gtkCellRenderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(gtkTreeViewColumn, gtkCellRenderer, TRUE);
	gtk_tree_view_column_add_attribute(gtkTreeViewColumn, gtkCellRenderer, "active", MSGLISTCOLUMN_ENCRYPTED);

	gtk_paned_pack1(gtkPanedMessages, pSession->gtkTreeViewMessages, TRUE, TRUE);

	GtkWidget* gtkVBoxMessage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	pSession->gtkLabelMessageHeading = gtk_label_new(NULL);
	gtk_box_pack_start(gtkVBoxMessage, pSession->gtkLabelMessageHeading, FALSE, FALSE, 0);

	GtkWidget* gtkScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gtkScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	pSession->gtkTextBufferMessageText = gtk_text_buffer_new(NULL);
	GtkWidget* gtkTextView = gtk_text_view_new_with_buffer(pSession->gtkTextBufferMessageText);
	gtk_text_view_set_editable(gtkTextView, FALSE);
	gtk_container_add(GTK_CONTAINER(gtkScrolledWindow), gtkTextView);
	gtk_box_pack_start(gtkVBoxMessage, gtkScrolledWindow, TRUE, TRUE, 0);

	gtk_paned_pack2(gtkPanedMessages, gtkVBoxMessage, TRUE, TRUE);

	gtk_box_pack_start(gtkMainVBox, gtkPanedMessages, TRUE, TRUE, 0);
	gtk_widget_show_all(gtkMainVBox);
	return gtkMainVBox;
}

void
ActionMessagesRefreshCB(GtkAction* gtkAction, gpointer pUserData)
{
	Py_ssize_t n, nLen;
	gint iID;
	GValue gRowID = G_VALUE_INIT;
	PyObject* pyMessage, *pyMessageID, *pyMessageSenderName, *pyMessageHeader, *pyMessageText, *pyEncrypted, *pyMessageDate, *pyMessageDateText, *pyID;
	GtkTreeIter gtkTreeIter;
	GtkTreePath* gtkTreePath;
	GtkTreeSelection* gtkTreeSelection;
	gboolean bExists;

	if (g.pCurrentSession->pyUserID == Py_None || g.pCurrentSession->pyUserID == NULL) {
		g_debug("Not logged in!");
		return;
	}

	Py_XDECREF(g.pyMsg);
	g.pyMsg = PyDict_New();
	PyDict_SetItem(g.pyMsg, hlMsg.Type, hlMsg.GetMessageList);

	if (!Session_Exchange(g.pCurrentSession)) {
		PyObject* pyType = PyDict_GetItem(g.pCurrentSession->pyResultMsg, hlMsg.Type);
		if (pyType == hlMsg.NotFound)
			gtk_statusbar_push(g.gtkStatusbar, 1, "No new messages");
		else
			ErrorDialog(g.pCurrentSession->sStatusMessage);
		Py_DECREF(pyType);
	}
	else {
		PyObject* pyData = PyDict_GetItemString(g.pCurrentSession->pyResultMsg, "Data");
		nLen = PySequence_Size(pyData);
		for (n = 0; n < nLen; n++) {
			pyMessage = PySequence_ITEM(pyData, n);
			pyMessageID = PySequence_ITEM(pyMessage, 0);
			iID = PyLong_AsLong(pyMessageID);
			pyMessageSenderName = PySequence_ITEM(pyMessage, 4);
			pyMessageHeader = PySequence_ITEM(pyMessage, 1);
			pyMessageText = PySequence_ITEM(pyMessage, 2);
			pyEncrypted = PySequence_ITEM(pyMessage, 7);
			pyMessageDate = PySequence_ITEM(pyMessage, 8);
			pyMessageDateText = PyObject_CallMethod(g.pyStdDateTimeFormat, "format", "(O)", pyMessageDate);
			bExists = FALSE;
			if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g.pCurrentSession->gtkListStoreMessages), &gtkTreeIter)) {
				do {
					gtk_tree_model_get_value(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_ID, &gRowID);
					if (g_value_get_int(&gRowID) == iID)
						bExists = TRUE;
					g_value_unset(&gRowID);
				} while (gtk_tree_model_iter_next(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter));
			}

			if (!bExists) {
				gtk_list_store_append(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter);
				gtk_list_store_set(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_ID, iID, -1);
				gtk_list_store_set(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_SENDER, PyUnicode_AsString(pyMessageSenderName), -1);
				gtk_list_store_set(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_HEADING, PyUnicode_AsString(pyMessageHeader), -1);
				gtk_list_store_set(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_TEXT, PyUnicode_AsString(pyMessageText), -1);
				gtk_list_store_set(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_DATE, PyUnicode_AsString(pyMessageDateText), -1);
				gtk_list_store_set(g.pCurrentSession->gtkListStoreMessages, &gtkTreeIter, MSGLISTCOLUMN_ENCRYPTED, pyEncrypted == Py_True, -1);
			}
			Py_DECREF(pyMessage);
			Py_DECREF(pyMessageID);
			Py_DECREF(pyMessageSenderName);
			Py_DECREF(pyMessageHeader);
			Py_DECREF(pyMessageText);
			Py_DECREF(pyEncrypted);
			Py_DECREF(pyMessageDate);
			Py_DECREF(pyMessageDateText);
		}
		if (nLen > 0) {
			/*gtkTreePath = gtk_tree_path_new_from_indices(0, -1);
			gtkTreeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pSession->gtkTreeKeyWord));
			gtk_tree_selection_select_path(gtkTreeSelection, gtkTreePath);
			gtk_tree_path_free(gtkTreePath);*/
		}
	}
}
