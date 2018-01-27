import pylax
import datetime

#ds = None

def launch():
    print("HI")
    ds = pylax.Dynaset("SalesOrder", "SELECT OrderID, Customer, LE.Name AS CustomerName, OrderDate FROM SalesOrder JOIN le ON LeID=Customer WHERE LE.Name LIKE :Name ORDER BY OrderDate DESC LIMIT 100;")
    ds.autoColumn = ds.add_column("OrderID", int, key=True)
    ds.add_column("Customer", int)
    ds.add_column("CustomerName", str, None)
    dt = ds.add_column("OrderDate", datetime.datetime, False, format="{:%Y-%m-%d}", default = datetime.datetime.now())
    print(ds)

    dsOrderLine = pylax.Dynaset("OrderLine", "SELECT OrderLine.OrderID AS OrderID, Item, Quantity, Price, Quantity * Price AS Amount, OrderLine.rowid AS RowID FROM OrderLine JOIN SalesOrder ON SalesOrder.OrderID=OrderLine.OrderID WHERE OrderLine.OrderID=:OrderID LIMIT 10000;", parent=ds)
    dsOrderLine.add_column("RowID", int, key=True)
    dsOrderLine.add_column("OrderID", int, parent = "OrderID")
    dsOrderLine.add_column("Item", int)
    dsOrderLine.add_column("Quantity", float)
    dsOrderLine.add_column("Price", float)
    dsOrderLine.add_column("Amount", key=None)
    #dsOrderLine.autoExecute = False
    print(dsOrderLine)

    form = pylax.Form(None, 20, 30, 640, 480, "Order", ds)
    #form.icon = pylax.Image("Order.ico")
    #form.minWidth, form.minHeight = 640, 400
    form.before_close=Form__before_close
    labelFormCaption = pylax.Label(form, 2, 2, 20, 60, "FormCaption", dynaset=ds, column="CustomerName", visible = False)
    labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

    tableSelect = pylax.Table(form, 20, 44, -20, 100, dynaset=ds)
    tableSelect.add_column("Customer", 150, "CustomerName")
    tableSelect.add_column("ID", 30, "OrderID")
    tableSelect.add_column("Date", 90, "OrderDate", format="{:%y-%m-%d}")


    entrySearch = pylax.Entry(form, 90, 20, 120, 20, "Search Name") #, label = pylax.Label(form, 20, 20, 70, 20, "Search"))
    #print(entrySearch.dataType)
    ds.buttonSearch = pylax.Button(form, 220, 20, 20, 20, "⏎")
    ds.buttonSearch.on_click = buttonSearch__on_click;

    entryID = pylax.Entry(form, 100, 160, 120, 20, dynaset=ds, column="OrderID", dataType=int, label = pylax.Label(form, 20, 160, 70, 20, "ID"))
    entryDate = pylax.Entry(form, 100, 190, 120, 20, dynaset=ds, column="OrderDate", format="{:%Y-%m-%d %H:%M}", label = pylax.Label(form, 20, 190, 70, 20, "Date"))
    entryDate.editFormat = "{:%Y-%m-%d %H:%M:%S}"
    entryCustomer = pylax.Entry(form, 330, 160, 120, 20, dynaset=ds, column="CustomerName", format="{:%Y-%m-%d %H:%M}", label = pylax.Label(form, 240, 160, 90, 20, "Customer"))
    entryCustomer.validate = entryCustomer__validate

    tableOrderLine = pylax.Table(form, 20, 220, -20, -80, dynaset=dsOrderLine)
    tableOrderLine.add_column("Item ID", 50, "Item")
    tableOrderLine.add_column("Quantity", 50, "Quantity")

    entryQuantity = pylax.Entry(form, 120, -70, 120, 20, dynaset=dsOrderLine, column="Quantity", label = pylax.Label(form, 20, -70, 90, 20, "Quantity"))

    ds.buttonEdit = pylax.Button(form, -360, -40, 60, 20, "Edit")
    ds.buttonNew = pylax.Button(form, -290, -40, 60, 20, "New")
    ds.buttonDelete = pylax.Button(form, -220, -40, 60, 20, "Delete")
    ds.buttonUndo = pylax.Button(form, -150, -40, 60, 20, "Undo")
    ds.buttonSave = pylax.Button(form, -80, -40, 60, 20, "Save")

    r = ds.execute({"Name": entrySearch.data})
    #ds.row = 0
    #print(form.data)
    #r2 = dsOrderLine.execute((1,))
    #dsOrderLine.row = 0

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0

def Form__before_close(data):
    print("Closing!")

def entryCustomer__validate(widget, data):

    def buttonSearch__on_click(data):
        if entrySearch.data == None:
            r = ds.execute(query = "SELECT LeID, Name FROM LE LIMIT 100;")
        else:
            r = ds.execute({"Name": entrySearch.data}, "SELECT LeID, Name FROM LE WHERE Name LIKE :Name LIMIT 100;")

    def buttonOK__on_click(data):
        widget.dynaset.set_data("Customer", ds.get_row_data()["LeID"])
        widget.dynaset.set_data("CustomerName", ds.get_row_data()["Name"])
        window.close()

    ds = pylax.Dynaset("LE", "SELECT LeID, Name FROM LE WHERE Name = :Name LIMIT 100;")
    ds.autoColumn = ds.add_column("LeID", int, key=True)
    ds.add_column("Name")
    rows = ds.execute({"Name": data})
    if rows == 1:
        ds.row = 0
        widget.dynaset.set_data("Customer", ds.get_row_data()["LeID"])
        widget.dynaset.set_data("CustomerName", ds.get_row_data()["Name"])
        return True
    else:
        window = pylax.Window(None, 0, 0, 320, 220, "Select Customer", ds)

        entrySearch= pylax.Entry(window, 20, 20, -70, 20, "Search Name")
        entrySearch.data = data + "%"
        ds.buttonSearch = pylax.Button(window, -60, 20, 20, 20, "⏎")
        ds.buttonSearch.on_click = buttonSearch__on_click;

        table = pylax.Table(window, 20, 60, -20, -70, dynaset=ds)
        table.add_column("Name", 150, "Name")
        table.add_column("ID", 30, "LeID")
        ds.buttonOK = pylax.Button(window, -60, -50, 40, 20, "OK")
        ds.buttonOK.on_click = buttonOK__on_click;
        r = ds.execute({"Name": entrySearch.data}, "SELECT LeID, Name FROM LE WHERE Name LIKE :Name;")
        return False
