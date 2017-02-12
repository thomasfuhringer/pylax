import pylax

entrySearch = None
ds = None
selectorEntrySearch = None

def launch():
    global ds, entrySearch
        
    ds = pylax.Dynaset("LE", "SELECT le.LeID, le.Name, le.Description, le.Logo, le.Parent, p.Name AS ParentName FROM LE LEFT JOIN LE AS p ON le.Parent=p.LeID WHERE le.Name LIKE :Name OR le.LeID = :Name ORDER BY le.Name DESC LIMIT 100;")
    ds.autoColumn = ds.add_column("LeID", int, format="{:,}", key=True)
    ds.add_column("Name")
    ds.add_column("Description")
    ds.add_column("Logo", bytes)
    ds.add_column("Parent", int)
    ds.add_column("ParentName")
    ds.before_save = ds__before_save

    # GUI
    form = pylax.Form(None, pylax.Widget.defaultCoordinate, 10, 640, 480, "Legal Entity", ds)
    form.minWidth, form.minHeight = 480, 360
    labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dynaset=ds, column="Name", visible=False)
    labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

    entrySearch = pylax.Entry(form, 70, 20, 160, 20, label = pylax.Label(form, 20, 20, 40, 20, "Search")) 
    entrySearch.data = "%"
    entrySearch.focus_in()
    #entrySearch.verify= entryS__verify
    #print(form.focus)
    ds.buttonSearch = pylax.Button(form, 240, 20, 60, 20, "Sea&rch")
    ds.buttonSearch.on_click = buttonSearch__on_click
    ds.buttonSearch.defaultEnter = True

    selectionTable = pylax.Table(form, 20, 70, -240, -55, dynaset=ds, label = pylax.Label(form, 20, 50, 90, 20, "Select:"))
    selectionTable.add_column("ID", 60, "LeID") # title, width, dataset
    selectionTable.add_column("Name", 130, "Name") 
    selectionTable.add_column("Parent", 130, "ParentName", widget=pylax.Entry(None)) 
    #print(selectionTable.columns[2].widget)

    imagePicture = pylax.ImageView(form, -140, 20, 90, 80, dynaset=ds, column="Logo", dataType=bytes)

    entryID = pylax.Entry(form, -160, 120, 60, 20, dynaset=ds, column="LeID", dataType=int, label = pylax.Label(form, -230, 120, 60, 20, "ID"))
    entryID.editFormat="{:,}"
    entryID.alignHoriz = pylax.Align.left

    entryName = pylax.Entry(form, -160, 150, 140, 20, dynaset=ds, column="Name", label = pylax.Label(form, -230, 150, 60, 20, "Name"))
    entryParent = pylax.Entry(form, -160, 180, 140, 20, dynaset=ds, column="ParentName", label = pylax.Label(form, -230, 180, 60, 20, "Parent"))
    #entryParent.alignHoriz = pylax.Align.left
    #entryParentName = pylax.Entry(form, -160, 210, 100, 20, dynaset=ds, column="ParentName")
    entryParent.verify = entryParent__verify
    entryParent.on_click_button = entryParent__on_click_button
    
    ds.buttonNew = pylax.Button(form, -360, -40, 60, 20, "&New")
    ds.buttonEdit = pylax.Button(form, -290, -40, 60, 20, "&Edit")
    ds.buttonUndo = pylax.Button(form, -220, -40, 60, 20, "&Undo")
    ds.buttonSave = pylax.Button(form, -150, -40, 60, 20, "&Save")
    ds.buttonDelete = pylax.Button(form, -80, -40, 60, 20, "&Delete")

    r = ds.execute({"Name": entrySearch.data})
    #ds.row = 0
    #print("b",form.bottom)

def buttonSearch__on_click(self):
    r = ds.execute({"Name": entrySearch.data})
    if r > 0:
        ds.row = 0
    print(ds.lastUpdateSQL)
        
def entryParent__verify(self):
    r=selector.dynaset.execute({"Name": self.inputString})
    if r == 1:
        print(selector.dynaset.get_row_data(0)["LeID"])
        selector.dynaset.row = 0
        self.input = selector.data["Name"]
        self.dynaset.set_data("Parent", selector.data["LeID"])
        return True
    if selector.run():
        self.data = selector.data["Name"]
        self.dynaset.set_data("Parent", selector.data["LeID"])
        return True
    else:
        pylax.message("Legal Entity '" + self.inputString + "' does not exist!", "Invalid Input")
        return False
        
def entryParent__on_click_button(self):
    if selector.run():
        self.data = selector.data["Name"]
        self.dynaset.set_data("Parent", selector.data["LeID"])
        
def ds__before_save(self):
    #pylax.message(ds.lastUpdateSQL)
    print("BS")
    return True
        

def create_selector_LegalEntity():
    global selectorEntrySearch
    ds = pylax.Dynaset("LE", "SELECT LeID, Name, Description FROM LE WHERE Name LIKE :Name OR LeID = :Name ORDER BY Name DESC LIMIT 100;")
    ds.autoColumn = ds.add_column("LeID", int, key=True)
    ds.add_column("Name")
    ds.add_column("Description")
    selector = pylax.Window(None,  30, 20, 480, 280, "Select Legal Entity", ds)

    selectorEntrySearch= pylax.Entry(selector, 20, 20, -90, 20)
    selectorEntrySearch.data = "%"
    ds.buttonSearch = pylax.Button(selector, -80, 20, 60, 20, "Sea&rch")
    ds.buttonSearch.on_click = selectorSearchButton__on_click;

    table = pylax.Table(selector, 20, 48, -20, -60, dynaset=ds)
    table.add_column("Name", 150, "Name")
    table.add_column("ID", 30, "LeID")
    table.showRowIndicator = False
    selector.buttonOK = ds.buttonOK = pylax.Button(selector, -130, -40, 50, 20, "OK")
    selector.buttonCancel = pylax.Button(selector, -70, -40, 50, 20, "Cancel")
    r=ds.execute({"Name": "%"})
    return selector

def selectorSearchButton__on_click(self):
    ds = self.window.dynaset
    r = ds.execute({"Name": selectorEntrySearch.data})
    if r > 0:
        ds.row = 0
		
def entryS__verify(self):
    pylax.message("Ver")
    return True
        
selector = create_selector_LegalEntity()