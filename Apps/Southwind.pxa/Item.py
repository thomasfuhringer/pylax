import pylax

def launch():

    form = pylax.Form(None, 20, 30, 680, 500, "Item")
    #form.minWidth, form.minHeight = 640, 480
    
    ds = pylax.DataSet("Item", "SELECT ItemID, Name, Description, Picture, Parent FROM Item;")
    ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True)
    ds.add_column("Name")
    ds.add_column("Description", str)
    ds.add_column("Picture", bytes)
    ds.add_column("Parent", int)
    
    selectionTable = pylax.Table(form, 20, 70, -240, -55, dataSet=ds, label = pylax.Label(form, 20, 50, 90, 20, "Select:"))
    selectionTable.add_column("Name", 70, "Name")
    selectionTable.add_column("Description", 100, "Description")
    
    imageViewPicture = pylax.ImageView(form, -130, 20, 90, 80, dataSet=ds, column="Picture", dataType=bytes)
    #form.buttonCancel = pylax.Button(form, -60, -40, 50, 20, "Cancel")
    
    r = ds.execute()




