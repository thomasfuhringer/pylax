# Pylax PostgreSQL example form, 2018 by Thomas Führinger

import site, sys
sitepackages = site.getsitepackages()
sys.path.extend(sitepackages) # to find psycopg2 on Debian

import pylax, psycopg2

cnx = psycopg2.connect("host='localhost' dbname='pylax' user='commoner' password='x'")

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0

ds = pylax.Dynaset("Item", "SELECT itemid, name FROM item WHERE name LIKE %(Name)s ORDER BY name DESC LIMIT 100;", connection=cnx)
ds.autoColumn = ds.add_column("itemid", int, format="{:,}", key=True)
ds.add_column("name")

form = pylax.Form(None, 20, 10, 680, 480, "Item from PostgreSQL", ds)

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
selectionTable.add_column("Name", 70, "name")
selectionTable.showRowIndicator = True

entryID = pylax.Entry(form, -200, 60, 40, 20, dynaset=ds, column="itemid", dataType=int, label = pylax.Label(form, -300, 60, 70, 20, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(form, -200, 90, -110, 20, dynaset=ds, column="name", dataType=str, label = pylax.Label(form, -300, 90, 70, 20, "Name"))

#r = ds.execute({"Name": "%"})

