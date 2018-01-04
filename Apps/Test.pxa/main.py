# Pylax example program, 2017 by Thomas Führinger

import pylax

def sayHelloMenuItem__on_click():
    pylax.status_message("Hellö!")

sayHelloMenuItem = pylax.MenuItem("Say Hello", sayHelloMenuItem__on_click)
pylax.append_menu_item(sayHelloMenuItem)

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
dsDetail.add_column("Quantity", int, format="{:,}")

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

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0

entrySearch = pylax.Entry(tabPageMain, 70, 20, -420, 20, label = pylax.Label(tabPageMain, 20, 22, 40, 20, "Search"))
entrySearch.data = "%"
ds.buttonSearch = pylax.Button(tabPageMain, -410, 20, 20, 20, "Go")
ds.buttonSearch.on_click = buttonSearch__on_click
ds.buttonSearch.defaultEnter = True

selectionTable = pylax.Table(tabPageMain, 20, 80, -360, -130, dynaset=ds, label = pylax.Label(tabPageMain, 20, 60, 90, 20, "Select"))
selectionTable.add_column("Name", 70, "Name")
selectionTable.add_column("Description", 100, "Description")
selectionTable.showRowIndicator = True

image = pylax.Image(tabPageMain, -340, 40, -20, 320, dynaset=ds, column="Picture", label = pylax.Label(tabPageMain, -340, 20, 70, 20, "Picture"))
entryID = pylax.Entry(tabPageMain, 100, -100, -360, 20, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(tabPageMain, 20, -100, 70, 20, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(tabPageMain, 100, -70, -360, 20, dynaset=ds, column="Name", dataType=str, label = pylax.Label(tabPageMain, 20, -70, 70, 20, "Name"))
entryPrice = pylax.Entry(tabPageMain, 100, -40, -360, 20, dynaset=ds, column="Price", dataType=float, label = pylax.Label(tabPageMain, 20, -40, 70, 20, "Price"))
entryPrice.format="{0:,.2f}"


detailTable = pylax.Table(tabPageMain, -340, 410, -20, -60, dynaset=dsDetail, label = pylax.Label(tabPageMain, -340, 390, 90, 20, "Sold To"))
detailTable.showRowIndicator = True
#detailTable.add_column("Item", 70, "Item")
detailTable.add_column("Customer Name", 120, "CustomerName")
detailTable.add_column("Quantity", 100, "Quantity", editable = True)

dsDetail.buttonEdit = pylax.Button(tabPageMain, -170, -40, 20, 20)
dsDetail.buttonNew = pylax.Button(tabPageMain, -140, -40, 20, 20)
dsDetail.buttonDelete = pylax.Button(tabPageMain, -110, -40, 20, 20)
dsDetail.buttonUndo = pylax.Button(tabPageMain, -80, -40, 20, 20)
dsDetail.buttonSave = pylax.Button(tabPageMain, -50, -40, 20, 20)

r = ds.execute({"Name": entrySearch.data})
