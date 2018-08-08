import pylax

def launch():

    form = pylax.Form(None, 20, 30, 680, 500, "Item")
    #form.minWidth, form.minHeight = 640, 480

    ds = pylax.Dynaset("Item", "SELECT ItemID, Name, Description, Picture, Parent FROM Item ORDER BY Name DESC LIMIT 100;")
    ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True)
    ds.add_column("Name")
    ds.add_column("Description", str)
    ds.add_column("Picture", bytes)
    ds.add_column("Parent", int)
    
    selectionTable = pylax.Table(form, 12, 88, -344, -170, dynaset=ds, label = pylax.Label(form, 12, 60, 90, 24, "Select"))
    selectionTable.add_column("Name", 70, "Name")
    selectionTable.add_column("Description", 100, "Description")
    selectionTable.showRowIndicator = True    

    imagePicture = pylax.Image(form, -330, 20, 320, 320, dynaset=ds, column="Picture")

    ds.buttonEdit = pylax.Button(form, -328, -36, 60, 24, "Edit")
    ds.buttonNew = pylax.Button(form, -264, -36, 60, 24, "New")
    ds.buttonDelete = pylax.Button(form, -200, -36, 60, 24, "Delete")
    ds.buttonUndo = pylax.Button(form, -136, -36, 60, 24, "Undo")
    ds.buttonSave = pylax.Button(form, -72, -36, 60, 24, "Save")

    r = ds.execute()

