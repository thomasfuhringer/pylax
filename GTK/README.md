# Pylax on GTK
GTK version of Pylax, a database front end for SQLite using embedded Python as scripting language

Code examples

### Hello World

![Hello World screen shot](Screenshot%20Hello.png)

```python
import pylax

form = pylax.Form(None, 20, 10, 680, 480, "Hello World")
label = pylax.Label(form, 30, 30, -20, -20, "Welcome to Pylax!")
label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center
```

### Example Form

![example form screen shot](Screenshot%20Test.png)

```python
import pylax

ds = pylax.Dynaset("Item", "SELECT ItemID, Name, Description, Picture, Price FROM Item;")
ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True)
ds.add_column("Name")
ds.add_column("Description", str)
ds.add_column("Picture", bytes)
ds.add_column("Price", float)

form = pylax.Form(None, 20, 10, 680, 480, "Test Form", ds)

labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dynaset=ds, column="Name", visible=False)
labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

ds.buttonEdit = pylax.Button(form, -360, -40, 60, 20, "Edit")
ds.buttonNew = pylax.Button(form, -290, -40, 60, 20, "New")
ds.buttonDelete = pylax.Button(form, -220, -40, 60, 20, "Delete")
ds.buttonUndo = pylax.Button(form, -150, -40, 60, 20, "Undo")
ds.buttonSave = pylax.Button(form, -80, -40, 60, 20, "Save")

selectionTable = pylax.Table(form, 20, 50, -320, -50, dynaset=ds, label = pylax.Label(form, 20, 20, 90, 20, "Select:"))
selectionTable.add_column("Name", 70, "Name")
selectionTable.add_column("Description", 100, "Description")
selectionTable.showRowIndicator = True

entryID = pylax.Entry(form, -200, 60, 40, 20, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(form, -300, 62, 70, 20, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(form, -200, 90, -110, 20, dynaset=ds, column="Name", dataType=str, label = pylax.Label(form, -300, 92, 70, 20, "Name"))
entryPrice = pylax.Entry(form, -200, 120, -110, 20, dynaset=ds, column="Price", dataType=float, label = pylax.Label(form, -300, 122, 70, 20, "Price"))
entryPrice.format="{0:,.2f}"

r = ds.execute()
```

### Master - Detail Form

![Item form screen shot](Screenshot%20Item.png)

