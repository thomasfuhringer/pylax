import pylax

entrySearch = None
comboBoxSearch = None
ds = None

def launch():
    global ds, entrySearch, comboBoxSearch

    ds = pylax.Dynaset("Person", "SELECT PersonID, FirstName, LastName, Picture FROM Person ORDER BY LastName DESC LIMIT 100;")
    ds.autoColumn = ds.add_column("PersonID", int, format="{:,}", key=True)
    ds.add_column("LastName", str, False)
    ds.add_column("FirstName", str, False, default = "One")
    ds.add_column("Picture", bytes)

    # GUI
    form = pylax.Form(None, 10, 10, 680, 450, "Person", ds)
    form.minWidth, form.minHeight = 640, 480

    labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dynaset=ds, column="LastName", visible=False)
    labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

    comboBoxSearch = pylax.ComboBox(form, 90, 20, 50, 20, label = pylax.Label(form, 20, 20, 70, 20, "Search by"))
    comboBoxSearch.append("ID")
    comboBoxSearch.append("Name")
    comboBoxSearch.data = "Name"

    entrySearch = pylax.Entry(form, 160, 20, -410, 20)
    buttonSearch = pylax.Button(form, -400, 20, 20, 20, "⏎")
    buttonSearch.on_click = buttonSearch__on_click
    buttonSearch.defaultEnter = True

    selectionTable = pylax.Table(form, 20, 90, -360, -55, dynaset=ds, label = pylax.Label(form, 20, 70, 90, 20, "Select:"))
    selectionTable.add_column("First Name", 70, "FirstName")
    selectionTable.add_column("Last Name", 150, "LastName")

    imagePicture = pylax.Image(form, -260, 10, 150, 150, dynaset=ds, column="Picture")

    entryID = pylax.Entry(form, -260, 180, 60, 20, dynaset=ds, column="PersonID", dataType=int, label = pylax.Label(form, -340, 180, 70, 20, "ID"))
    entryID.editFormat="{:,}"
    entryID.alignHoriz = pylax.Align.left
    entryID.readOnly = True


    entryFirstName = pylax.Entry(form, -260, 210, 60, 20, dynaset=ds, column="FirstName", dataType=str, label = pylax.Label(form, -340, 210, 70, 20, "First Name"))
    entryLastName = pylax.Entry(form, -260, 240, 60, 20, dynaset=ds, column="LastName", dataType=str, label = pylax.Label(form, -340, 240, 70, 20, "Last Name"))
    #comboBox = pylax.ComboBox(form, -260, 270, 60, 20, dynaset=ds, column="PersonID", label = pylax.Label(form, -340, 270, 70, 20, "PersonID"))
    #comboBox.append("Zero", 0)
    #comboBox.append("One", 1)
    #comboBox.append("Two", 2)

    ds.buttonEdit = pylax.Button(form, -360, -40, 60, 20, "Edit")
    ds.buttonNew = pylax.Button(form, -290, -40, 60, 20, "New")
    ds.buttonDelete = pylax.Button(form, -220, -40, 60, 20, "Delete")
    ds.buttonUndo = pylax.Button(form, -150, -40, 60, 20, "Undo")
    ds.buttonSave = pylax.Button(form, -80, -40, 60, 20, "Save")

    #r = ds.execute()
    #ds.row = 0


def buttonSearch__on_click(self):
    if entrySearch.data == None:
        r = ds.execute(query = "SELECT PersonID, FirstName, LastName, Picture FROM Person ORDER BY LastName DESC LIMIT 100;")
    else:
        if comboBoxSearch.data == "ID":
            r = ds.execute({"PersonID": entrySearch.data}, "SELECT PersonID, FirstName, LastName, Picture FROM Person WHERE PersonID = :PersonID;")
        else:
            r = ds.execute({"LastName": entrySearch.data}, "SELECT PersonID, FirstName, LastName, Picture FROM Person WHERE LastName LIKE :LastName ORDER BY LastName DESC LIMIT 100;")
