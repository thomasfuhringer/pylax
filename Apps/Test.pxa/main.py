# Pylax example program, 2018 by Thomas Führinger

import pylax

def sayHelloMenuItem__on_click():
    pylax.status_message("Hellö!")

def aboutBoxMenuItem__on_click():
    aboutBox = pylax.Window(None,  0, 0, 320, 160, "About")
    aboutBox.minHeight = 100
    label = pylax.Label(aboutBox, 30, 30, -20, -20, "Pylax test form")
    label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center
    aboutBox.visible = True

sayHelloMenuItem = pylax.MenuItem("Say Hello", sayHelloMenuItem__on_click)
pylax.append_menu_item(sayHelloMenuItem)
aboutBoxMenuItem = pylax.MenuItem("About...", aboutBoxMenuItem__on_click)
pylax.append_menu_item(aboutBoxMenuItem)

ds = pylax.Dynaset("Item", "SELECT ItemID, Name, Description, Picture, Price FROM Item WHERE Item.Name LIKE :Name ORDER BY Name DESC LIMIT 100;")
ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True) # part of primary key
ds.add_column("Name")
ds.add_column("Description", str)
ds.add_column("Picture", bytes)
ds.add_column("Price", float)

dsDetail = pylax.Dynaset("ItemSold", "SELECT ItemSold.rowid, Item, Customer, Customer.Name AS CustomerName, Quantity FROM ItemSold JOIN Customer ON ItemSold.Customer=Customer.CustomerID WHERE ItemSold.Item=:ItemID;", parent=ds)
dsDetail.autoColumn = dsDetail.add_column("rowid", int, key=True)
dsDetail.add_column("Item", int, parent = "ItemID")
dsDetail.add_column("Customer", int)
dsDetail.add_column("CustomerName", str, key=None) # non database column
dsDetailColumnQuantity = dsDetail.add_column("Quantity", int, format="{:,}")

def dsDetail__on_changed(ds, row, column):
    if column == dsDetailColumnQuantity or column is None:
        entryTotalQuanity.data = ds.get_column_data_sum("Quantity")
dsDetail.on_changed = dsDetail__on_changed

form = pylax.Form(None, 20, 10, 680, 480, "Item", ds)

labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dynaset=ds, column="Name", visible=False)
labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

ds.buttonEdit = pylax.Button(form, -360, -40, 60, 20, "Edit")
ds.buttonNew = pylax.Button(form, -290, -40, 60, 20, "New")
ds.buttonDelete = pylax.Button(form, -220, -40, 60, 20, "Delete")
ds.buttonUndo = pylax.Button(form, -150, -40, 60, 20, "Undo")
ds.buttonSave = pylax.Button(form, -80, -40, 60, 20, "Save")

tab = pylax.Tab(form, 2, 2, -2, -55)
tabPageMain = pylax.TabPage(tab, 0, 0, 0, 0, "Main")
tabPagePicture = pylax.TabPage(tab, 0, 0, 0, 0, "Picture")

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0

def entryCustomer__on_click_button(self):

    def selectorSearchButton__on_click(data):
        if entrySearch.data == None:
            r = ds.execute(query = "SELECT CustomerID, Name FROM Customer;")
        else:
            r = ds.execute({"Name": entrySearch.data}, "SELECT CustomerID, Name FROM Customer WHERE Name LIKE :Name;")

    def selectorOkButton__on_click(data):
        self.dynaset.set_data("Customer", ds.get_row_data()["CustomerID"])
        self.dynaset.set_data("CustomerName", ds.get_row_data()["Name"])
        selector.close()

    ds = pylax.Dynaset("LE", "SELECT CustomerID, Name FROM Customer;")
    ds.autoColumn = ds.add_column("CustomerID", int, key=True)
    ds.add_column("Name")
    selector = pylax.Window(None, 0, 0, 320, 220, "Select Customer", ds)

    entrySearch= pylax.Entry(selector, 20, 20, -70, 20, "Search Name")
    ds.buttonSearch = pylax.Button(selector, -60, 20, 20, 20, "⏎")
    ds.buttonSearch.on_click = selectorSearchButton__on_click;

    table = pylax.Table(selector, 20, 60, -20, -70, dynaset=ds)
    table.add_column("Name", 150, "Name")
    table.add_column("ID", 30, "CustomerID")
    ds.buttonOK = pylax.Button(selector, -60, -50, 40, 20, "OK")
    ds.buttonOK.on_click = selectorOkButton__on_click;
    #selector.buttonCancel = pylax.Button(selector, -60, -40, 50, 20, "Cancel")
    r=ds.execute()

entrySearch = pylax.Entry(tabPageMain, 70, 20, -420, 20, label = pylax.Label(tabPageMain, 20, 22, 40, 20, "Search"))
entrySearch.data = "%"
ds.buttonSearch = pylax.Button(tabPageMain, -410, 20, 20, 20, "⏎")
ds.buttonSearch.on_click = buttonSearch__on_click
ds.buttonSearch.defaultEnter = True

selectionTable = pylax.Table(tabPageMain, 20, 80, -360, -130, dynaset=ds, label = pylax.Label(tabPageMain, 20, 60, 90, 20, "Select"))
selectionTable.add_column("Name", 70, "Name")
selectionTable.add_column("Description", 100, "Description")
selectionTable.showRowIndicator = True

image = pylax.Image(tabPagePicture, 20, 40, -20, 320, dynaset=ds, column="Picture", label = pylax.Label(tabPagePicture, 20, 20, 70, 20, "Picture"))

entryID = pylax.Entry(tabPageMain, 100, -100, -360, 20, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(tabPageMain, 20, -100, 70, 20, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(tabPageMain, 100, -70, -360, 20, dynaset=ds, column="Name", dataType=str, label = pylax.Label(tabPageMain, 20, -70, 70, 20, "Name"))
entryPrice = pylax.Entry(tabPageMain, 100, -40, -360, 20, dynaset=ds, column="Price", dataType=float, label = pylax.Label(tabPageMain, 20, -40, 70, 20, "Price"))
entryPrice.format="{0:,.2f}"

detailTable = pylax.Table(tabPageMain, -340, 40, -20, -170, dynaset=dsDetail, label = pylax.Label(tabPageMain, -340, 20, 90, 20, "Sold To"))
detailTable.showRowIndicator = True
#detailTable.add_column("Item", 70, "Item")
detailTable.add_column("Customer Name", 120, "CustomerName")
detailTable.add_column("Quantity", 100, "Quantity", editable = True)

entryTotalQuanity = pylax.Entry(tabPageMain, -185, -160, 50, 20, dataType=int, label = pylax.Label(tabPageMain, -340, -160, 70, 20, "Total Quantity"))
entryTotalQuanity.format="{:,}"

entryCustomer = pylax.Entry(tabPageMain, -260, -120, 60, 20, dynaset=dsDetail, column="CustomerName", dataType=str, label = pylax.Label(tabPageMain, -340, -120, 70, 20, "Customer"))
entryCustomer.on_click_button = entryCustomer__on_click_button
entryQuanity = pylax.Entry(tabPageMain, -260, -90, 60, 20, dynaset=dsDetail, column="Quantity", dataType=int, label = pylax.Label(tabPageMain, -340, -90, 70, 20, "Quantity"))

dsDetail.buttonEdit = pylax.Button(tabPageMain, -170, -40, 20, 20)
dsDetail.buttonNew = pylax.Button(tabPageMain, -140, -40, 20, 20)
dsDetail.buttonDelete = pylax.Button(tabPageMain, -110, -40, 20, 20)
dsDetail.buttonUndo = pylax.Button(tabPageMain, -80, -40, 20, 20)
dsDetail.buttonSave = pylax.Button(tabPageMain, -50, -40, 20, 20)

r = ds.execute({"Name": entrySearch.data})
