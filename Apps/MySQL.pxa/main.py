# Pylax MySQL example form, 2018 by Thomas Führinger

import site, sys
sitepackages = site.getsitepackages()
sys.path.extend(sitepackages) # to find mysql on Debian

import pylax, mysql.connector

cnx = mysql.connector.connect(user='pylax', database='pylax', password='x')

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0

ds = pylax.Dynaset("Item", "SELECT ItemID, Name FROM Item WHERE Name LIKE %(Name)s ORDER BY Name DESC LIMIT 100;", connection=cnx)
ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True)
ds.add_column("Name")

form = pylax.Form(None, 20, 10, 680, 480, "Item from MySQL", ds)

ds.buttonEdit = pylax.Button(form, -360, -40, 60, 20, "Edit")
ds.buttonNew = pylax.Button(form, -290, -40, 60, 20, "New")
ds.buttonDelete = pylax.Button(form, -220, -40, 60, 20, "Delete")
ds.buttonUndo = pylax.Button(form, -150, -40, 60, 20, "Undo")
ds.buttonSave = pylax.Button(form, -80, -40, 60, 20, "Save")


entrySearch = pylax.Entry(form, 70, 20, -420, 20, label = pylax.Label(form, 20, 22, 40, 20, "Search"))
entrySearch.data = "%"
buttonSearch = pylax.Button(form, -410, 20, 20, 20, "⏎")
buttonSearch.on_click = buttonSearch__on_click

selectionTable = pylax.Table(form, 20, 80, -360, -130, dynaset=ds, label = pylax.Label(form, 20, 60, 90, 20, "Select"))
selectionTable.add_column("Name", 70, "Name")
selectionTable.showRowIndicator = True

entryID = pylax.Entry(form, -200, 60, 40, 20, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(form, -300, 60, 70, 20, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(form, -200, 90, -110, 20, dynaset=ds, column="Name", dataType=str, label = pylax.Label(form, -300, 90, 70, 20, "Name"))

def on_load():      # this function gets called after the script has been loaded
    r = ds.execute({"Name": "%"})
