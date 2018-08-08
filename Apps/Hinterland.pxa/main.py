# Pylax hinterland example form, 2018 by Thomas Führinger

import pylax, sys, hinterland as hl

#host = "localhost"
host = "45.76.133.182"

session = hl.Client(host)
if(session.status_message):
    raise RuntimeError("Connection problem: " + session.status_message)
"""
if not session.log_in("me", password="?"):
    raise RuntimeError("Log in Failed: " + session.status_message)
"""
def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0 
        
def buttonLoadForm__on_click(self):
    if session.exchange({hl.Msg.Type: hl.Msg.Get, "Entity": "Form", "Parameters": {"Name": "A%"}}):
        exec(session.msg["Code"])
    else:
        print("Get Entity Failed:", session.status_message)        
        

ds = pylax.Dynaset("Item", connection=session)
ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True)
ds.add_column("Name")
ds.add_column("Description", str)

form = pylax.Form(None, 20, 10, 680, 480, "Item from Hinterland", ds)

ds.buttonEdit = pylax.Button(form, -328, -36, 60, 24, "Edit")
ds.buttonNew = pylax.Button(form, -264, -36, 60, 24, "New")
ds.buttonDelete = pylax.Button(form, -200, -36, 60, 24, "Delete")
ds.buttonUndo = pylax.Button(form, -136, -36, 60, 24, "Undo")
ds.buttonSave = pylax.Button(form, -72, -36, 60, 24, "Save")

entrySearch = pylax.Entry(form, 62, 12, -389, 24, label = pylax.Label(form, 12, 12, 40, 24, "Search"))
entrySearch.data = "%"
buttonSearch = pylax.Button(form, -379, 12, 24, 24, "⏎")
buttonSearch.on_click = buttonSearch__on_click
buttonSearch.defaultEnter = True

selectionTable = pylax.Table(form, 12, 88, -344, -170, dynaset=ds, label = pylax.Label(form, 12, 60, 90, 24, "Select"))
selectionTable.add_column("Name", 70, "Name")
selectionTable.add_column("Description", 100, "Description")
selectionTable.showRowIndicator = True

entryID = pylax.Entry(form, -200, 60, 40, 24, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(form, -300, 60, 70, 24, "ID"))
entryID.editFormat="{:,}"
entryID.alignHoriz = pylax.Align.left
entryName = pylax.Entry(form, -200, 90, -110, 24, dynaset=ds, column="Name", dataType=str, label = pylax.Label(form, -300, 90, 70, 24, "Name"))

buttonLoadForm = pylax.Button(form, 20, -40, 50, 24, "Load Form")
buttonLoadForm.on_click = buttonLoadForm__on_click

#r = ds.execute({"Name": "%"})

