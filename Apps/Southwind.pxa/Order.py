import pylax
import datetime

#ds = None
    
def launch():
    ds = pylax.DataSet("SalesOrder", "SELECT OrderID, Customer, LE.Name AS CustomerName, OrderDate FROM SalesOrder JOIN le ON LeID=Customer;")
    ds.autoColumn = ds.add_column("OrderID", int, key=True)
    ds.add_column("Customer", int)
    ds.add_column("CustomerName", str, None)
    dt = ds.add_column("OrderDate", datetime.datetime, False, format="{:%Y-%m-%d}", default = datetime.datetime.now())

    dsOrderLine = pylax.DataSet("OrderLine", "SELECT OrderLine.OrderID AS OrderID, Item, Quantity, Price, Quantity * Price AS Amount, OrderLine.rowid AS RowID FROM OrderLine JOIN SalesOrder ON SalesOrder.OrderID=OrderLine.OrderID WHERE OrderLine.OrderID=:OrderID;", parent=ds)
    dsOrderLine.add_column("RowID", int, key=True)
    dsOrderLine.add_column("OrderID", int, parent = "OrderID")
    dsOrderLine.add_column("Item", int)
    dsOrderLine.add_column("Quantity", float)
    dsOrderLine.add_column("Price", float)
    dsOrderLine.add_column("Amount", key=None)
    #dsOrderLine.on_parent_changed=dsOrderLine__on_parent_changed
    #print(dsOrderLine.parent)
    #dsOrderLine.autoExecute = False

    form = pylax.Form(None, 20, 30, 640, 480, "Order", ds)
    form.icon = pylax.Image("Order.ico")
    #form.minWidth, form.minHeight = 640, 400
    form.before_close=dsOrderLine__on_parent_changed

    tableSelect = pylax.Table(form, 20, 44, -20, 100, dataSet=ds)
    tableSelect.add_column("Customer", 150, "CustomerName") # title, width, DataSet column name
    tableSelect.add_column("ID", 30, "OrderID")
    tableSelect.add_column("Date", 90, "OrderDate", format="{:%y-%m-%d}")


    entrySearch = pylax.Entry(form, 90, 20, 120, 20, "Search Name") #, label = pylax.Label(form, 20, 20, 70, 20, "Search"))
    #print(entrySearch.dataType)
    ds.buttonSearch = pylax.Button(form, 220, 20, 60, 20, "Search")
    ds.buttonSearch.on_click = searchButton__on_click;

    entryID = pylax.Entry(form, 100, 160, 120, 20, dataSet=ds, column="OrderID", dataType=int, label = pylax.Label(form, 20, 160, 70, 20, "ID"))
    entryDate = pylax.Entry(form, 100, 190, 120, 20, dataSet=ds, column="OrderDate", format="{:%Y-%m-%d %H:%M}", label = pylax.Label(form, 20, 190, 70, 20, "Date"))
    entryDate.editFormat = "{:%Y-%m-%d %H:%M:%S}"
    entryCustomer = pylax.Entry(form, 330, 160, 120, 20, dataSet=ds, column="CustomerName", format="{:%Y-%m-%d %H:%M}", label = pylax.Label(form, 240, 160, 90, 20, "Customer"))
    entryCustomer.verify = entryCustomer__verify

    tableOrderLine = pylax.Table(form, 20, 220, -20, -80, dataSet=dsOrderLine)
    tableOrderLine.add_column("Item ID", 50, "Item")
    tableOrderLine.add_column("Quantity", 50, "Quantity")

    entryQuantity = pylax.Entry(form, 120, -70, 120, 20, dataSet=dsOrderLine, column="Quantity", label = pylax.Label(form, 20, -70, 90, 20, "Quantity"))

    #print(entrySearch.alignHoriz)

    ds.buttonNew = pylax.Button(form, -370, -40, 60, 20, "New")
    ds.buttonEdit = pylax.Button(form, -300, -40, 60, 20, "Edit")
    ds.buttonUndo = pylax.Button(form, -230, -40, 60, 20, "Undo")
    ds.buttonSave = pylax.Button(form, -160, -40, 60, 20, "Save")
    ds.buttonDelete = pylax.Button(form, -90, -40, 60, 20, "Delete")

    labelFormCaption = pylax.Label(form, 2, 2, 20, 60, "FormCaption", dataSet=ds, column="CustomerName", visible = False)
    labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient
    r = ds.execute()
    #ds.row = 0
    #print(form.data)
    #r2 = dsOrderLine.execute((1,))
    #dsOrderLine.row = 0

def dsOrderLine__on_parent_changed(data):
    pylax.status_message("Update detail!")


def entryCustomer__verify(widget, data):
    rows = selector.dataSet.execute({"Name": data}, "SELECT LeID, Name FROM LE WHERE Name = :Name;")
    if rows == 1:
        selector.dataSet.row = 0
    else:
        pass#selector.entrySearch.data = data + "%"
        if not selector.show():
            return False
    return {"Customer": selector.dataSet.get_data("LeID"), "CustomerName": selector.dataSet.get_data("Name")}

def searchButton__on_click(self):
    pylax.status_message("search...")
    r = self.window.ds.execute()


def create_selector_Customer():
    ds = pylax.DataSet("LE", "SELECT LeID, Name FROM LE;")
    ds.autoColumn = ds.add_column("LeID", int, key=True)
    ds.add_column("Name")
    selector = pylax.Window(None,  30, 20, 480, 280, "Select Customer", ds)

    entrySearch= pylax.Entry(selector, 90, 20, 120, 20, "Search Name")
    ds.buttonSearch = pylax.Button(selector, 220, 20, 60, 20, "Search")
    ds.buttonSearch.on_click = selectorSearchButton__on_click;

    table = pylax.Table(selector, 20, 44, -20, 100, dataSet=ds)
    table.add_column("Name", 150, "Name")
    table.add_column("ID", 30, "LeID")
    ds.buttonOK = pylax.Button(selector, -120, -40, 50, 20, "OK")
    selector.buttonCancel = pylax.Button(selector, -60, -40, 50, 20, "Cancel")
    r=ds.execute()
    return selector

def selectorSearchButton__on_click(data):
    pylax.message("Search!")
    if entrySearch.data == None:
        r = ds.execute(query = "SELECT LeID, Name FROM LE;")
    else:
        r = ds.execute({"Name": entrySearch.data}, "SELECT LeID, Name FROM LE WHERE Name LIKE :Name;")
        
selector = create_selector_Customer()