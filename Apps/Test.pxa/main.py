# Pylax example program, 2018 by Thomas Führinger

import pylax

def aboutBoxMenuItem__on_click():
    aboutBox = pylax.Window(None,  0, 0, 320, 160, "About")
    aboutBox.minHeight = 100
    label = pylax.Label(aboutBox, 30, 30, -20, -20, "Pylax test form")
    label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center
    aboutBox.visible = True

def sayHelloMenuItem__on_click():
    pylax.status_message("Hellö!")

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0

def detailColumnQuantity__default():
    return 12

def dsDetail__on_changed(ds, row, column):
    if column == dsDetailColumnQuantity or column is None:
        entryTotalQuanity.data = ds.get_column_data_sum("Quantity")

def entryQuanity__validate(widget, data):
    if not data >= 0:
        pylax.message("Please enter a positive number!")
        return False
    return True

def entryCustomer__validate(self, data):
    selector = Selector()
    if selector.validate(data):
        return True
    else:
        selector.dialog(data)
        if selector.ok:
            self.dynaset.set_data("Customer", selector.dynaset.get_row_data()["CustomerID"])
            self.dynaset.set_data("CustomerName", selector.dynaset.get_row_data()["Name"])
            return Warning
        else:
            return False

def entryCustomer__on_click_button(self):
    selector = Selector()
    selector.dialog("")
    if selector.ok:
        self.dynaset.set_data("Customer", selector.dynaset.get_row_data()["CustomerID"])
        self.dynaset.set_data("CustomerName", selector.dynaset.get_row_data()["Name"])


class Selector():
    def __init__(self):
        self.query_precise = "SELECT CustomerID, Name FROM Customer WHERE Name = :Name;"
        self.query_browse = "SELECT CustomerID, Name FROM Customer WHERE Name LIKE :Name LIMIT 100;"
        self.ok = False
        self.dynaset = pylax.Dynaset("LE", self.query_precise)
        self.dynaset.add_column("CustomerID", int)
        self.dynaset.add_column("Name", str)

    def validate(self, data):
        rowcount = self.dynaset.execute({"Name": data}, self.query_precise)
        if rowcount == 1:
            return True
        else:
            return False

    def dialog(self, data):
        self.window = pylax.Window(None, 0, 0, 320, 220, "Select Customer", self.dynaset)

        self.entrySearch= pylax.Entry(self.window, 20, 20, -70, 20, "Search Name")
        self.entrySearch.data = data + "%"
        buttonSearch = pylax.Button(self.window, -60, 20, 20, 20, "⏎")
        buttonSearch.on_click = self.buttonSearch__on_click

        table = pylax.Table(self.window, 20, 60, -20, -70, dynaset=self.dynaset)
        table.add_column("Name", 150, "Name")
        table.add_column("ID", 30, "CustomerID")
        self.dynaset.buttonOK = pylax.Button(self.window, -60, -50, 40, 20, "OK")
        self.dynaset.buttonOK.on_click = self.buttonOK__on_click;
        self.dynaset.execute({"Name": self.entrySearch.data}, self.query_browse)
        self.window.wait_for_close()

    def buttonSearch__on_click(self, buttonSearch):
        text = self.entrySearch.data
        r = self.dynaset.execute({"Name": text if len(text) > 0 else "%"}, self.query_browse)

    def buttonOK__on_click(self, buttonOK):
        self.ok = True
        self.window.close()


sayHelloMenuItem = pylax.MenuItem("Say Hello", sayHelloMenuItem__on_click)
pylax.append_menu_item(sayHelloMenuItem)
aboutBoxMenuItem = pylax.MenuItem("About...", aboutBoxMenuItem__on_click)
pylax.append_menu_item(aboutBoxMenuItem)

ds = pylax.Dynaset("Item", "SELECT ItemID, Name, Description, Picture, Price FROM Item WHERE Item.Name LIKE :Name ORDER BY Name DESC LIMIT 100;")
ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True) # part of primary key
ds.add_column("Name", default = "Another One")
ds.add_column("Description", str)
ds.add_column("Picture", bytes)
ds.add_column("Price", float)

dsDetail = pylax.Dynaset("ItemSold", "SELECT ItemSold.rowid, Item, Customer, Customer.Name AS CustomerName, Quantity FROM ItemSold JOIN Customer ON ItemSold.Customer=Customer.CustomerID WHERE ItemSold.Item=:ItemID LIMIT 100;", parent=ds)
dsDetail.autoColumn = dsDetail.add_column("rowid", int, key=True)
dsDetail.add_column("Item", int, parent = "ItemID")
dsDetail.add_column("Customer", int)
dsDetail.add_column("CustomerName", str, key=None) # non database column
dsDetail.whoCols = True
dsDetailColumnQuantity = dsDetail.add_column("Quantity", int, format="{:,}", defaultFunction = detailColumnQuantity__default)
dsDetail.on_changed = dsDetail__on_changed

# GUI
form = pylax.Form(None, 20, 10, 680, 480, "Item", ds)

labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dynaset=ds, column="Name", visible=False)
labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

ds.buttonEdit = pylax.Button(form, -328, -36, 60, 24, "Edit")
ds.buttonNew = pylax.Button(form, -264, -36, 60, 24, "New")
ds.buttonDelete = pylax.Button(form, -200, -36, 60, 24, "Delete")
ds.buttonUndo = pylax.Button(form, -136, -36, 60, 24, "Undo")
ds.buttonSave = pylax.Button(form, -72, -36, 60, 24, "Save")

tab = pylax.Tab(form, 2, 2, -2, -55)
tabPageMain = pylax.TabPage(tab, 0, 0, 0, 0, "Main")
tabPagePicture = pylax.TabPage(tab, 0, 0, 0, 0, "Picture")

entrySearch = pylax.Entry(tabPageMain, 62, 12, -389, 24, label = pylax.Label(tabPageMain, 12, 12, 40, 24, "Search"))
entrySearch.data = "%"
buttonSearch = pylax.Button(tabPageMain, -379, 12, 24, 24, "⏎")
buttonSearch.on_click = buttonSearch__on_click
buttonSearch.defaultEnter = True

selectionTable = pylax.Table(tabPageMain, 12, 88, -344, -170, dynaset=ds, label = pylax.Label(tabPageMain, 12, 60, 90, 24, "Select"))
selectionTable.add_column("Name", 70, "Name") # title, width, Dynaset column name
selectionTable.add_column("Description", 100, "Description")
selectionTable.showRowIndicator = True


entryID = pylax.Entry(tabPageMain, 82, -160, 80, 24, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(tabPageMain, 12, -160, 70, 24, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(tabPageMain, 82, -132, 180, 24, dynaset=ds, column="Name", dataType=str, label = pylax.Label(tabPageMain, 12, -132, 70, 24, "Name"))
#entryDescription.plain = True
entryPrice = pylax.Entry(tabPageMain, 82, -104, 180, 24, dynaset=ds, column="Price", dataType=float, label = pylax.Label(tabPageMain, 12, -104, 70, 24, "Price"))
entryPrice.format="{0:,.2f}"

detailTable = pylax.Table(tabPageMain, -320, 88, -12, -196, dynaset=dsDetail, label = pylax.Label(tabPageMain, -320, 60, 90, 24, "Sold To"))
detailTable.showRowIndicator = True
detailTable.add_column("Customer Name", 120, "CustomerName")
detailTable.add_column("Quantity", 100, "Quantity", editable = True)

#entryTotalQuanity = pylax.Label(tabPageMain, -185, -190, 50, 24, dataType=int)
#label = pylax.Label(tabPageMain, -330, -190, 70, 20, "Total Quantity")
entryTotalQuanity = pylax.Entry(tabPageMain, -112, -190, 100, 24, dataType=int, label = pylax.Label(tabPageMain, -320, -190, 70, 20, "Total Quantity"))
entryTotalQuanity.format="{:,}"
entryTotalQuanity.readOnly = True

entryCustomer = pylax.Entry(tabPageMain, -250, -132, 180, 24, dynaset=dsDetail, column="CustomerName", dataType=str, label = pylax.Label(tabPageMain, -320, -132, 70, 20, "Customer"))
entryCustomer.on_click_button = entryCustomer__on_click_button
entryCustomer.validate = entryCustomer__validate
entryQuanity = pylax.Entry(tabPageMain, -250, -104, 180, 24, dynaset=dsDetail, column="Quantity", dataType=int, label = pylax.Label(tabPageMain, -320, -104, 70, 20, "Quantity"))
entryQuanity.validate = entryQuanity__validate

splitter = pylax.Splitter(tabPagePicture, 10, 10, -20, -20)
splitter.position = 380

image = pylax.Image(splitter.box1, 20, 50, -20, -24, dynaset=ds, column="Picture", label = pylax.Label(splitter.box1, 20, 20, 70, 24, "Picture"))
entryDescription = pylax.MarkDownEntry(splitter.box2, 20, 50, -20, -20, dynaset=ds, column="Description", dataType=str, label = pylax.Label(splitter.box2, 20, 20, 70, 20, "Description"))

dsDetail.buttonEdit = pylax.Button(tabPageMain, -156, -36, 24, 24)
dsDetail.buttonNew = pylax.Button(tabPageMain, -126, -36, 24, 24)
dsDetail.buttonDelete = pylax.Button(tabPageMain, -96, -36, 24, 24)
dsDetail.buttonUndo = pylax.Button(tabPageMain, -66, -36, 24, 24)
dsDetail.buttonSave = pylax.Button(tabPageMain, -36, -36, 24, 24)

r = ds.execute({"Name": entrySearch.data})
