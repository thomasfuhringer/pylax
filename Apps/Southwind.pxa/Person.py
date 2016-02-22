import pylax

entrySearch = None
ds = None

def launch():
    global ds, entrySearch
    def nameColumn__default():
        pylax.message("Default", "Gratuliere")
        return "Hallo"

    ds = pylax.DataSet("Person", "SELECT PersonID, FirstName, LastName, Picture FROM Person ORDER BY LastName DESC LIMIT 100;")
    ds.autoColumn = ds.add_column("PersonID", int, format="{:,}", key=True)
    ds.add_column("FirstName", str, defaultFunction = nameColumn__default)
    ds.add_column("LastName", str, False, default = "Geschafft")
    ds.add_column("Picture", bytes)

    # GUI
    form = pylax.Form(None, pylax.Widget.defaultCoordinate, 10, 680, 450, "Person", ds)
    form.minWidth, form.minHeight = 640, 480

    def entryFirstName__verify(widget, data):
        pylax.message("Nicht Geklückt! 品", "Gratuliere")
        return False
    labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dataSet=ds, column="LastName", visible=False)
    labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

    entrySearch = pylax.Entry(form, 70, 20, 160, 20, label = pylax.Label(form, 20, 20, 40, 20, "Search"))
    ds.buttonSearch = pylax.Button(form, 240, 20, 60, 20, "Search")
    ds.buttonSearch.on_click = buttonSearch__on_click
    ds.buttonSearch.defaultEnter = True

    selectionTable = pylax.Table(form, 20, 70, -240, -55, dataSet=ds, label = pylax.Label(form, 20, 50, 90, 20, "Select:"))
    selectionTable.add_column("First Name", 70, "FirstName") # title, width, dataset
    selectionTable.add_column("Last Name", 150, "LastName") # title, width, dataset
    selectionTable.add_column("ID", 130, "PersonID") # title, width, dataset

    imagePicture = pylax.ImageView(form, -130, 20, 90, 80, dataSet=ds, column="Picture", dataType=bytes)

    entryID = pylax.Entry(form, -130, 120, 120, 20, dataSet=ds, column="PersonID", dataType=int, label = pylax.Label(form, -230, 120, 90, 20, "ID"))
    entryID.editFormat="{:,}"
    entryID.alignHoriz = pylax.Align.left

    entryFirstName = pylax.Entry(form, -130, 150, 120, 20, dataSet=ds, column="FirstName", dataType=str, label = pylax.Label(form, -230, 150, 90, 20, "First Name"))
    entryFirstName.verify = entryFirstName__verify
    entryLastName = pylax.Entry(form, -130, 180, 120, 20, dataSet=ds, column="LastName", dataType=str, label = pylax.Label(form, -230, 180, 90, 20, "Last Name"))
    comboBox = pylax.ComboBox(form, -130, 220, 120, 20, dataSet=ds, column="PersonID", label = pylax.Label(form, -230, 220, 90, 20, "PersonID"))
    comboBox.append("Zero", 0)
    comboBox.append("One", 1)
    comboBox.append("Two", 2)
    entryLastName.readOnly = True

    ds.buttonNew = pylax.Button(form, -370, -40, 60, 20, "New")
    ds.buttonEdit = pylax.Button(form, -300, -40, 60, 20, "Edit")
    ds.buttonUndo = pylax.Button(form, -230, -40, 60, 20, "Undo")
    ds.buttonSave = pylax.Button(form, -160, -40, 60, 20, "Save")
    ds.buttonDelete = pylax.Button(form, -90, -40, 60, 20, "Delete")

    #r = ds.execute()
    #ds.row = 0
    #print("b",form.bottom)


def buttonSearch__on_click(self):
    #pylax.message(str(ds.lastUpdateSQL))
    #pylax.message("Geklückt! 品", "Gratuliere")
    if entrySearch.data == None:
        r = ds.execute(query = "SELECT PersonID, FirstName, LastName, Picture FROM Person;")
    else:
        #ds.query = "SELECT PersonID, FirstName, LastName, Picture FROM Person WHERE LastName LIKE :LastName;"
        r = ds.execute({"LastName": entrySearch.data}, "SELECT PersonID, FirstName, LastName, Picture FROM Person WHERE LastName LIKE :LastName;")




