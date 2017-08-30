# hinterland.py  | Hinterland communication server
# © 2017 by Thomas Führinger, made available under the terms of the GPL
# github.com/thomasfuhringer/pylax/hinterland
# For encrypted communication the library PyNaCl needs to be installed.

import pickle, socket, struct, threading, time, sys, traceback
import hashlib, uuid, base64, sqlite3, importlib.util
from enum import Enum

nacl_spec = importlib.util.find_spec("nacl")
nacl_available = nacl_spec is not None
if nacl_available:
    import nacl, nacl.utils
    from nacl.public import PrivateKey, PublicKey, Box

version_info = (0, 1, 0)
default_port = 1550

class Msg:
    Type, Connect, Disconnect, Error, Shutdown, \
    Success, Invalid, Decline, NotFound, NotAuthorized, \
    Key, SignOn, LogIn, Logout, ChangePassword, \
    Get, Data, \
    SearchPerson, GetPerson, DeletePerson, \
    SetPage, GetPageChildren, SearchPage, GetPage, DeletePage, \
    SetOrg, GetOrgChildren, SearchOrg, GetOrg, DeleteOrg, \
    SetRole, GetRoles, DeleteRole, \
    SendMessage, GetMessageList, GetMessage, MessageRead = range(37)

class LogLevel:
    Debug, Info, Warning, Error, Critical = (10, 20, 30, 40, 50)

def send(socket, data):
    length = len(data)
    socket.send(length.to_bytes(4, byteorder="big"))
    socket.sendall(data)

def receive(socket):
    length_raw = socket.recv(4)
    if len(length_raw) != 4:
        return None
    length = struct.unpack("!I", length_raw)[0]
    data = bytearray(length)
    view = memoryview(data)
    next_offset = 0
    while length - next_offset > 0:
        recv_size = socket.recv_into(view[next_offset:], length - next_offset)
        if recv_size == 0:
            return None
        next_offset += recv_size
    return data

def generate_salt():
    return uuid.uuid4().bytes

def hash(data):
    return hashlib.sha512(data).digest()

def generate_key_pair():
    if nacl_available:
        private_key = PrivateKey.generate()
        return private_key, private_key.public_key
    else:
        return None, None

def fingerprint(key):
    return hashlib.md5(pickle.dumps(key)).hexdigest()
    #return base64.b64encode(hashlib.sha256(pickle.dumps(key)).digest())

def export_key(key, file_name="Hinterland private.key"):
    with open(file_name, "wb") as key_file:
        pickle.dump(key, key_file)

def import_key(file_name="Hinterland private.key"):
    with  open(file_name, "rb") as key_file:
        key = pickle.load(key_file)
    return key


class Session(threading.Thread):
    def __init__(self, socket, address, server):
        threading.Thread.__init__(self)
        self.socket = socket
        self.address = address
        self.server = server
        self.token = None
        self.key = None            # public key of client
        self.up = True
        self.user = None

    def send(self, data):
        bytes = pickle.dumps(data)
        if self.key and nacl_available:
            box = Box(self.server.private_key, self.key)
            encrypted = box.encrypt(bytes, nacl.utils.random(Box.NONCE_SIZE))
            send(self.socket, pickle.dumps(encrypted))
        else:
            send(self.socket, bytes)

    def receive(self):
        data = receive(self.socket)
        if data is None:
            return None
        if self.key and nacl_available:
            box = Box(self.server.private_key, self.key)
            decrypted = box.decrypt(pickle.loads(data))
            return pickle.loads(decrypted)
        else:
            return pickle.loads(data)

    def run(self):
        hi = self.socket.recv(4)
        if len(hi) != 4 or hi[:2] != bytes([0x06, 0x0E]): # magic number
            print("Invalid connection request from address: " + self.address[0] + ", port: " + str(self.address[1]), file=sys.stderr)
            self.socket.close()
            return
        self.socket.send(bytes([0x06, 0x0E, version_info[0], version_info[1]]))

        self.cnx = sqlite3.connect(self.server.database, detect_types=sqlite3.PARSE_DECLTYPES) #, detect_types=sqlite3.PARSE_COLNAMES)
        self.cnx.row_factory = sqlite3.Row
        self.cursor = self.cnx.cursor()
        self.log(LogLevel.Info, "Connection| address: " + self.address[0] + "| port: " + str(self.address[1]))

        while self.up:
            try:
                self.msg = self.receive()
                if self.msg is None:
                    self.up = False
                    self.log(LogLevel.Info, "Connection broke")
                elif Msg.Type in self.msg:
                    if self.msg[Msg.Type] == Msg.Connect:
                        self.connect()
                    elif self.msg[Msg.Type] == Msg.Disconnect:
                        self.user = None
                        self.up = False
                        #self.send({Msg.Type: Msg.Success})

                    elif self.msg[Msg.Type] == Msg.Key:
                        self.exchange_keys()
                    elif self.msg[Msg.Type] == Msg.SignOn:
                        self.sign_on()
                    elif self.msg[Msg.Type] == Msg.LogIn:
                        self.log_in()
                    elif self.msg[Msg.Type] == Msg.Logout:
                        self.logout()
                    elif self.msg[Msg.Type] == Msg.ChangePassword:
                        self.change_password()

                    elif self.msg[Msg.Type] == Msg.SetPage:
                        self.set_page()
                    elif self.msg[Msg.Type] == Msg.SearchPage:
                        self.search_page()
                    elif self.msg[Msg.Type] == Msg.GetPage:
                        self.get_page()
                    elif self.msg[Msg.Type] == Msg.GetPageChildren:
                        self.get_page_children()
                    elif self.msg[Msg.Type] == Msg.DeletePage:
                        self.delete_page()

                    elif self.msg[Msg.Type] == Msg.SearchPerson:
                        self.search_person()
                    elif self.msg[Msg.Type] == Msg.GetPerson:
                        self.get_person()

                    elif self.msg[Msg.Type] == Msg.SearchOrg:
                        self.search_org()
                    elif self.msg[Msg.Type] == Msg.GetOrg:
                        self.get_org()
                    elif self.msg[Msg.Type] == Msg.GetOrgChildren:
                        self.get_org_children()

                    elif self.msg[Msg.Type] == Msg.SendMessage:
                        self.send_message()
                    elif self.msg[Msg.Type] == Msg.GetMessageList:
                        self.get_message_list()
                    elif self.msg[Msg.Type] == Msg.GetMessage:
                        self.get_message()
                    elif self.msg[Msg.Type] == Msg.MessageRead:
                        self.message_read()

                    elif self.msg[Msg.Type] == Msg.Get:
                        self.get()

                    elif self.msg[Msg.Type] == Msg.Shutdown:
                        self.shutdown()
                    else:
                        self.send({Msg.Type: Msg.Invalid, "Text": "No valid message type"})
                        self.log(LogLevel.Info, "No valid message type")
                else:
                    self.send({Msg.Type: Msg.Invalid, "Text": "No message type"})
                    self.log(LogLevel.Info, "No message type")
            except Exception as e:
                self.up = False
                self.log(LogLevel.Error, str(e))
                print("Unexpected error:", e, file=sys.stderr)
                traceback.print_tb(sys.exc_info()[2])

        self.socket.close()
        self.log(LogLevel.Info, "Connection closed ")
        self.cnx.close()

    def connect(self):
        if "Token" in self.msg:
            self.token = self.msg["Token"]
        if "PublicKey" in self.msg:
            self.send({Msg.Type: Msg.Success, "ID": self.server.token, "PublicKey": self.server.public_key})
            self.key = self.msg["PublicKey"]
        else:
            self.send({Msg.Type: Msg.Success, "ID": self.server.token})
        self.log(LogLevel.Info, "client: " + self.token +  ("|encrypted" if self.key else ""))

    def shutdown(self):
        self.log(LogLevel.Info, "Shutdown")
        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized, "Text": "Not logged in"})
            return

        self.cursor.execute("SELECT Person FROM Role WHERE Person=? AND Administrator=1 AND Org IS NULL;", (self.user,))
        row = self.cursor.fetchone()
        if row is None:
            self.send({Msg.Type: Msg.NotAuthorized, "Text": "Site administrator role required"})
            return

        self.server.up = False
        self.send({Msg.Type: Msg.Success})
        try:
            for session in server.sessions:
                session.up = False
                #ctypes.pythonapi.PyThreadState_SetAsyncExc(session.ident, None)
        except Exception as e:
            return

    def exchange_keys(self):
        if "PublicKey" in self.msg:
            self.send({Msg.Type: Msg.Success, "PublicKey": self.server.public_key})
            self.key = self.msg["PublicKey"]
            if self.user is not None:
                self.cursor.execute("UPDATE Person SET PublicKey=?, ModDate=CURRENT_TIMESTAMP WHERE PersonID=?;", (pickle.dumps(self.key), self.user))
                self.cnx.commit()
            self.log(LogLevel.Info, "Key sent")
        else:
            self.send({Msg.Type: Msg.Invalid, "Text": "No public key provided"})
            self.log(LogLevel.Info, "Key request invalid. No public key provided.")

    def sign_on(self):
        if "Password" in self.msg:
            salt = generate_salt()
            password = self.hash(self.msg["Password"] + salt)
        else:
            salt = None
            password = None

        handle = self.msg["Handle"]
        if len(handle) == 0 or \
           len(list(filter(str.isdigit, handle))) == len(handle) or \
           any(c in "@$%&§#~?^°|<>\'""" for c in handle):
            self.send({Msg.Type: Msg.Decline, "Text": "Please choose a different handle!"})
            self.log(LogLevel.Info, "Failed signon attempt. Handle invalid: " + handle)
            return

        self.cursor.execute("SELECT PersonID FROM Person WHERE Handle=?;", (handle,))
        result = self.cursor.fetchone()
        if result is not None:
            self.send({Msg.Type: Msg.Decline, "Text": "User exists."})
            self.log(LogLevel.Info, "Failed signon attempt. User exists: " + handle)
            return
        try:
            self.cursor.execute("INSERT INTO Person (FirstName, LastName, Handle, EMail, Password, PasswordSalt, PublicKey) VALUES (?,?,?,?,?,?,?) ;",
                (self.msg["FirstName"] if "FirstName" in self.msg else None,
                self.msg["LastName"] if "LastName" in self.msg else None,
                handle,
                self.msg["EMail"] if "EMail" in self.msg else None,
                password, salt, pickle.dumps(self.key)))
            self.user = self.cursor.lastrowid
            self.cursor.execute("INSERT INTO Message (Person, Text) VALUES (0, 'Welcome to Hinterland" + (", " + self.msg["FirstName"] if "FirstName" in self.msg else "") + "!')");
            self.cursor.execute("INSERT INTO Addressee (Message, Person) VALUES (?,?)", (self.cursor.lastrowid, self.user));
            self.cnx.commit()
            self.send({Msg.Type: Msg.Success, "ID": self.user})
            self.log(LogLevel.Info, "Signon")
        except sqlite3.Error as error:
            self.cnx.rollback()
            self.send({Msg.Type: Msg.Error, "Text": "Database error " + str(error)})
            self.log(LogLevel.Error, "Database error " + str(error))

    def log_in(self):
        if self.user is not None:
            self.send({Msg.Type: Msg.Decline, "Text": "A user is logged in already."})
            return
        else:
            if "ID" in self.msg:
                self.cursor.execute("SELECT PersonID, Handle, FirstName, LastName, Password, PasswordSalt, PublicKey FROM Person WHERE PersonID=?;", (self.msg["ID"],))
                r = self.cursor.fetchone()
                if r is None:
                    self.send({Msg.Type: Msg.Decline, "Text": "Invalid user ID"})
                    return
                else:
                    self.user = self.msg["ID"]
            elif "Handle" in self.msg:
                self.cursor.execute("SELECT PersonID, Handle, FirstName, LastName, Password, PasswordSalt, PublicKey FROM Person WHERE Handle=?;", (self.msg["Handle"],))
                r = self.cursor.fetchone()
                if r is None:
                    self.send({Msg.Type: Msg.Decline, "Text": "Invalid user name"})
                    return
                else:
                    self.user = r["PersonID"]
            else:
                self.send({Msg.Type: Msg.Invalid, "Text": "No user name or ID given."})

            assert self.user is not None
            if r["Password"]:
                if "Password" in self.msg:
                    password = self.hash(self.msg["Password"] + r["PasswordSalt"])
                else:
                    password = None
                if password != r["Password"]:
                    self.user = None
                    self.send({Msg.Type: Msg.Decline, "Text": "Invalid password"})
                    return
            else:
                if "Password" in self.msg:
                    self.user = None
                    self.send({Msg.Type: Msg.Decline, "Text": "Invalid password"})
                    return

            if self.key and (r["PublicKey"] is None or self.key != pickle.loads(r["PublicKey"])):
                self.send({Msg.Type: Msg.Success, "ID": self.user, "FirstName": r["FirstName"], "LastName": r["LastName"], "Handle": r["Handle"], "Warning": "Different public encryption key on file for this user."})
            else:
                if "Ecrypted" in self.msg:
                    if not nacl_available:
                        self.send({Msg.Type: Msg.Decline, "Text": "Encryption not available."})
                    elif r["PublicKey"]:
                        self.send({Msg.Type: Msg.Success, "ID": self.user, "FirstName": r["FirstName"], "LastName": r["LastName"], "Handle": r["Handle"], "PublicKey": self.server.public_key})
                        self.key = r["PublicKey"]
                    else:
                        self.send({Msg.Type: Msg.Decline, "Text": "No public key on file."})
                        return
                else:
                    self.send({Msg.Type: Msg.Success, "ID": self.user, "FirstName": r["FirstName"], "LastName": r["LastName"], "Handle": r["Handle"]})
            self.log(LogLevel.Info, "LogIn")

    def logout(self):
        if self.user is None:
            self.send({Msg.Type: Msg.Decline, "Text": "No user logged in"})
        else:
            self.user = None
            self.send({Msg.Type: Msg.Success})
            self.log(LogLevel.Info, "Logout")

    def change_password(self):
        self.log(LogLevel.Info, "ChangePassword")
        if self.user is None:
            self.send({Msg.Type: Msg.Decline, "Text": "No user logged in"})
            return

        if "Password" not in self.msg:
            self.send({Msg.Type: Msg.Decline, "Text": "No password provided"})
            return

        user = self.user
        if "User" in self.msg and self.msg["User"] != self.user:
            user = self.msg["User"]
            self.cursor.execute("SELECT Person FROM Role WHERE Person=? AND Administrator=1 AND Org IS NULL;", (self.user,))
            row = self.cursor.fetchone()
            if row is None:
                self.send({Msg.Type: Msg.NotAuthorized, "Text": "Site administrator role required"})
                return

        if self.msg["Password"] is None:
            self.cursor.execute("UPDATE Person SET Password=NULL, PasswordSalt=NULL WHERE PersonID=?;", (user,))
        else:
            salt = generate_salt()
            password = self.hash(self.msg["Password"] + salt)
            self.cursor.execute("UPDATE Person SET Password=?, PasswordSalt=? WHERE PersonID=?;", (password, salt, user))
        self.cnx.commit()
        self.send({Msg.Type: Msg.Success})

    def get(self):
        self.log(LogLevel.Info, "Get|" + self.msg["Entity"])
        if self.msg["Entity"] == "HinterlandVersion":
            self.send({Msg.Type: Msg.Success, "Data": version_info})
        else:
            self.send({Msg.Type: Msg.Decline, "Text": "Invalid entity"})

    def search_person(self):
        self.log(LogLevel.Info, "SearchPerson")
        if "Name" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "Name not given"})
            return
        else:
            term = "%" + self.msg["Name"] + "%"
            self.cursor.execute("SELECT PersonID, Handle, FirstName, MiddleName, LastName, Transcription FROM Person WHERE FirstName LIKE ? OR LastName LIKE ? OR Transcription LIKE ? OR Handle LIKE ? LIMIT 100;", (term, term, term, term))
            rows = self.cursor.fetchall()
            if rows:
                self.send({Msg.Type: Msg.Success, "Data": tuple((tuple(row) for row in rows))})
            else:
                self.send({Msg.Type: Msg.NotFound})

    def get_person(self):
        self.log(LogLevel.Info, "GetPerson")
        if "ID" in self.msg:
            self.cursor.execute("SELECT PersonID, Handle, FirstName, MiddleName, LastName, Transcription, PublicKey FROM Person WHERE PersonID=?;", (self.msg["ID"],))
        elif "Handle" in self.msg:
            self.cursor.execute("SELECT PersonID, Handle, FirstName, MiddleName, LastName, Transcription, PublicKey FROM Person WHERE Handle=?;", (self.msg["Handle"],))
        else:
            self.send({Msg.Type: Msg.Invalid, "Text": "Name or Handle required"})
            return

        row = self.cursor.fetchone()
        if row:
            self.send({Msg.Type: Msg.Success, "Data": {"ID": row["PersonID"], "FirstName": row["FirstName"], "MiddleName": row["MiddleName"], "LastName": row["LastName"], "Transcription": row["Transcription"], "PublicKey": row["PublicKey"], "Handle": row["Handle"]}})
        else:
            self.send({Msg.Type: Msg.NotFound})

    def set_page(self):
        self.log(LogLevel.Info, "SetPage")
        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized})
            return

        person = None if "Person" not in self.msg else self.msg["Person"]
        org = None if "Org" not in self.msg else self.msg["Org"]
        key_word = None if "KeyWord" not in self.msg else self.msg["KeyWord"]
        heading = None if "Heading" not in self.msg else self.msg["Heading"]
        text = None if "Text" not in self.msg else self.msg["Text"]
        data = None if "Data" not in self.msg else self.msg["Data"]
        language = None if "Language" not in self.msg else self.msg["Language"]
        if "Parent" in self.msg:
            parent = self.msg["Parent"]
            self.cursor.execute("SELECT PageID FROM Page WHERE PageID=?;", (parent, ))
            row = self.cursor.fetchone()
            if row is None:
                self.send({Msg.Type: Msg.Invalid, "Text": "Invalid parent"})
                return
        else:
            parent = None

        if text is None and data is None:
            self.send({Msg.Type: Msg.Invalid, "Text": "No message text or data"})
            return

        ok = False
        if self.user == person:
            if org is None:
                ok = True
            else:
                role = self.get_role(self.user, org)
                if role[2] == 1:
                    ok = True
        elif self.is_administrator(self.user, org):
            ok = True

        if not ok:
            self.send({Msg.Type: Msg.NotAuthorized})
            return

        id = None
        if "ID" in self.msg:
            id = self.msg["ID"]
        else:
            self.cursor.execute("SELECT PageID FROM Page WHERE Person" + ("=" if person else " IS ") + "? AND Org" + ("=" if org else " IS ") + "? AND Parent" + ("=" if parent else " IS ") + "? AND KeyWord" + ("=" if key_word else " IS ") + "? AND Language" + ("=" if language else " IS ") + "?;", (person, org, parent, key_word, language))
            row = self.cursor.fetchone()
            if row:
                id = row["PageID"]

        if id is not None:
            self.cursor.execute("UPDATE Page SET KeyWord=?, Heading=?, Text=?, Data=?, Parent=?, Language=?, Person=?, Org=?, ModUser=?, ModDate=CURRENT_TIMESTAMP WHERE PageID=?;", (key_word, heading, text, data, parent, language, person, org, self.user, id))
        else:
            self.cursor.execute("INSERT INTO Page (KeyWord, Heading, Text, Data, Parent, Language, Person, Org, ModUser) VALUES (?,?,?,?,?,?,?,?,?) ;", (key_word, heading, text, data, parent, language, person, org, self.user))
            id = self.cursor.lastrowid

        self.cnx.commit()
        self.send({Msg.Type: Msg.Success, "PageID": id})

    def search_page(self):
        if "Text" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "No search term given"})
            return
        self.log(LogLevel.Info, "SearchPage| ID: " + str(self.msg["Text"]))
        term = "%" + self.msg["Text"] + "%"
        self.cursor.execute("SELECT PageID, KeyWord, Heading, substr(Text,1,80), Org, Org.Name AS OrgName, Person, substr(Person.FirstName,1,1)||'. '|| Person.LastName AS PersonName, Page.Language, Page.ModDate FROM Page LEFT OUTER JOIN Person ON Page.Person=Person.PersonID LEFT OUTER JOIN Org ON Page.Org=Org.OrgID WHERE KeyWord LIKE ? OR Heading LIKE ? OR Text LIKE ? LIMIT 100;", (term, term, term))
        rows = self.cursor.fetchall()
        if rows:
            self.send({Msg.Type: Msg.Success, "Data": tuple((tuple(row) for row in rows))})
        else:
            self.send({Msg.Type: Msg.NotFound})

    def get_page_children(self):
        self.log(LogLevel.Info, "GetPageChildren")
        parent = None if "ID" not in self.msg else self.msg["ID"]
        if parent is not None:
            self.cursor.execute("SELECT PageID, KeyWord, Heading, substr(Text,1,80), Org, Org.Name AS OrgName, Person, substr(Person.FirstName,1,1)||'. '|| Person.LastName AS PersonName, Page.Language, Page.ModDate FROM Page LEFT OUTER JOIN Person ON Page.Person=Person.PersonID LEFT OUTER JOIN Org ON Page.Org=Org.OrgID WHERE Page.Parent" + ("=" if parent else " IS ") + "? LIMIT 100;", (parent,))
        else:
            org = None if "Org" not in self.msg else self.msg["Org"]
            person = None if "Person" not in self.msg else self.msg["Person"]
            language = None if "Language" not in self.msg else self.msg["Language"]
            self.cursor.execute("SELECT PageID, KeyWord, Heading, substr(Text,1,80), Org, Org.Name AS OrgName, Person, substr(Person.FirstName,1,1)||'. '|| Person.LastName AS PersonName, Page.Language, Page.ModDate FROM Page LEFT OUTER JOIN Person ON Page.Person=Person.PersonID LEFT OUTER JOIN Org ON Page.Org=Org.OrgID WHERE Page.Person" + ("=" if person else " IS ") + "? AND Page.Org" + ("=" if org else " IS ") + "? AND Page.Language" + ("=" if language else " IS ") + "? AND Page.Parent IS NULL LIMIT 100;", (person, org, language,))
        rows = self.cursor.fetchall()
        if rows:
            self.send({Msg.Type: Msg.Success, "Data": tuple((tuple(row) for row in rows))})
        else:
            self.send({Msg.Type: Msg.NotFound})

    def get_page(self):
        self.log(LogLevel.Info, "GetPage")
        if "ID" in self.msg:
            self.cursor.execute("SELECT PageID, KeyWord, Heading, Text, Data, Org, Org.Name AS OrgName, Org.Parent AS OrgParent, Person, substr(Person.FirstName,1,1)||'. '|| Person.LastName AS PersonName, Person.Handle, Page.Parent, Page.Language, Page.ModDate FROM Page LEFT OUTER JOIN Person ON Page.Person=Person.PersonID LEFT OUTER JOIN Org ON Page.Org=Org.OrgID WHERE PageID=?;", (self.msg["ID"],))
        else:
            if "Person" in self.msg:
                person = self.msg["Person"]
            else:
                person = None
            if "Org" in self.msg:
                org = self.msg["Org"]
            else:
                org = None
            if "KeyWord" in self.msg:
                key_word = self.msg["KeyWord"]
            else:
                key_word = None
            if "Parent" in self.msg:
                parent = self.msg["Parent"]
            else:
                parent = None
            if "Language" in self.msg:
                language = self.msg["Language"]
            else:
                language = None

            self.cursor.execute("SELECT PageID, KeyWord, Heading, Text, Data, Org, Org.Name AS OrgName, Org.Parent AS OrgParent, Person, substr(Person.FirstName,1,1)||'. '|| Person.LastName AS PersonName, Person.Handle, Page.Parent, Page.Language, Page.ModDate FROM Page LEFT OUTER JOIN Person ON Page.Person=Person.PersonID LEFT OUTER JOIN Org ON Page.Org=Org.OrgID WHERE Person" + ("=" if person else " IS ") + "? AND Org" + ("=" if org else " IS ") + "? AND Page.Parent" + ("=" if parent else " IS ") + "? AND KeyWord" + ("=" if key_word else " IS ") + "? AND Page.Language" + ("=" if language else " IS ") + "?;", (person, org, parent, key_word, language))

        row = self.cursor.fetchone()
        if row:
            # pylax.org|Hinterland Center♦TFu•Main Page•Sub Page
            if row["Org"] is not None:
                org_parent = row["OrgParent"]
                org_name = row["OrgName"]
                path = ""
                while org_name is not None:
                    path = "|" + org_name + path
                    self.cursor.execute("SELECT Parent, Name FROM Org WHERE OrgID=?;", (org_parent, ))
                    r = self.cursor.fetchone()
                    if r:
                        org_parent = r["Parent"]
                        org_name = r["Name"]
                    else:
                        org_name = None
            else:
                path = ""
            if row["Person"] is not None:
                path = path + "♦" + row["Handle"]
            kw_path = ""
            if row["KeyWord"] is not None:
                page = row["Parent"]
                kw = row["KeyWord"]
                while kw is not None:
                    kw_path = "•" + kw + kw_path
                    self.cursor.execute("SELECT Parent, KeyWord FROM Page WHERE PageID=?;", (page, ))
                    r = self.cursor.fetchone()
                    if r:
                        page = r["Parent"]
                        kw = r["KeyWord"]
                    else:
                        kw = None
            else:
                kw_path = "•"
            path = path + kw_path

            self.send({Msg.Type: Msg.Success, "Data": {"ID": row["PageID"], "KeyWord": row["KeyWord"], "Heading": row["Heading"], "Text": row["Text"], "Data": row["Data"], "Parent": row["Parent"], "Person": row["Person"], "PersonName": row["PersonName"], "Org": row["Org"], "OrgName": row["OrgName"], "Address": path, "Date": row["ModDate"]}})
            self.cursor.execute("UPDATE Page SET Views=Views+1, LastView=CURRENT_TIMESTAMP WHERE PageID=?;", (row["PageID"], ))
            self.cnx.commit()
        else:
            self.send({Msg.Type: Msg.NotFound})

    def delete_page(self):
        if "ID" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "No ID given"})
            return
        self.log(LogLevel.Info, "DeletePage| ID: " + str(self.msg["ID"]))

        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized})
            return

        if not self.is_administrator(self.user, None):
            self.send({Msg.Type: Msg.NotAuthorized})
            return
            # todo: more sophisticated authorization

        if self.delete_page_tree(self.msg["ID"]):
            self.send({Msg.Type: Msg.Success})
            self.cnx.commit()
        else:
            self.cnx.rollback()
            self.send({Msg.Type: Msg.NotFound})

    def delete_page_tree(self, ID):
        self.cursor.execute("SELECT PageID FROM Page WHERE Parent=?;", (ID, ))
        for row in self.cursor:
            self.delete_page_tree(row["PageID"])
        self.cursor.execute("DELETE FROM Page WHERE PageID=?;", (ID, ))
        return True if self.cursor.rowcount else False

    def search_org(self):
        if "Name" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "Name not given"})
            return
        self.log(LogLevel.Info, "SearchOrg| ID: " + str(self.msg["Name"]))
        term = "%" + self.msg["Name"] + "%"
        self.cursor.execute("SELECT OrgID, Name, Description, Parent FROM Org WHERE Name LIKE ? OR Description LIKE ?;", (term, term))
        rows = self.cursor.fetchall()
        if rows:
            self.send({Msg.Type: Msg.Success, "Data": tuple((tuple(row) for row in rows))})
        else:
            self.send({Msg.Type: Msg.NotFound})

    def get_org(self):
        if "ID" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "No ID given"})
            return
        self.log(LogLevel.Info, "GetOrg| ID: " + str(self.msg["ID"]))
        self.cursor.execute("SELECT OrgID, Name, Description, HostName, HostPort, PublicKey, Parent FROM Org WHERE OrgID=?;", (self.msg["ID"],))
        row = self.cursor.fetchone()
        if row:
            self.send({Msg.Type: Msg.Success, "Data": {"ID": row["OrgID"], "Name": row["Name"], "HostName": row["HostName"], "HostPort": row["HostPort"], "Parent": row["Parent"], "PublicKey": (pickle.loads(row["PublicKey"]) if row["PublicKey"] else None)}})
            self.cnx.commit()
        else:
            self.send({Msg.Type: Msg.NotFound})

    def get_org_children(self):
        self.log(LogLevel.Info, "GetOrgChildren")
        parent = None if "ID" not in self.msg else self.msg["ID"]
        self.cursor.execute("SELECT OrgID, Name, ModDate FROM Org WHERE Parent" + ("=" if parent else " IS ") + "? LIMIT 100;", (parent,))
        rows = self.cursor.fetchall()
        if rows:
            self.send({Msg.Type: Msg.Success, "Data": tuple((tuple(row) for row in rows))})
        else:
            self.send({Msg.Type: Msg.NotFound})

    def send_message(self):
        self.log(LogLevel.Info, "SendMessage")
        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized})
            return

        if "Message" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "No message given"})
            return

        message = self.msg["Message"]
        heading = None if "Heading" not in message else message["Heading"]
        text = None if "Text" not in message else message["Text"]
        data = None if "Data" not in message else message["Data"]
        if (text is None and data is None) or "Addressees" not in message:
            self.send({Msg.Type: Msg.Invalid, "Text": "Incomplete"})
            return

        if "Org" in message:
            org = message["Org"]
            if self.get_role(self.user, org) is None:
                self.send({Msg.Type: Msg.Invalid, "Text": "Sender has no role in Organization"})
                return
        else:
            org = None

        if "Encrypted" in message and message["Encrypted"] is True:
            encrypted = 1
        else:
            encrypted = 0

        self.cursor.execute("INSERT INTO Message (Person, Org, Heading, Text, Data, Encrypted) VALUES (?,?,?,?,?,?) ;", (self.user, org, heading, text, data, encrypted))
        message_id = self.cursor.lastrowid

        failed = False
        for addressee in message["Addressees"]:
            if "Person" in addressee:
                person = addressee["Person"]
                self.cursor.execute("SELECT PersonID FROM Person WHERE PersonID=?;", (person, ))
                row = self.cursor.fetchone()
                if row is None:
                    failed = True
                    break
            else:
                person = None

            if "Org" in addressee:
                org = addressee["Org"]
                self.cursor.execute("SELECT OrgID FROM Org WHERE OrgID=?;", (org, ))
                row = self.cursor.fetchone()
                if row is None:
                    failed = True
                    break
                if person is not None and self.get_role(person, org) is None:
                    failed = True
                    break
            else:
                org = None
            self.cursor.execute("INSERT INTO Addressee (Message, Person, Org) VALUES (?,?,?);", (message_id, person, org))

        if failed:
            self.cnx.rollback()
            self.send({Msg.Type: Msg.Invalid, "Text": "Org of addressee invalid"})
        else:
            self.cnx.commit()
            self.send({Msg.Type: Msg.Success, "ID": message_id})

    def get_message_list(self):
        self.log(LogLevel.Info, "GetMessageList")
        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized})
        else:
            old = True if ("Old" not in self.msg or self.msg["Old"] is not True) else True
            from_date = None if "From" not in self.msg else self.msg["From"]
            to_date = None if "To" not in self.msg else self.msg["To"]
            self.cursor.execute("SELECT MessageID, Heading, substr(Text,1,80), Message.Person, substr(Person.FirstName,1,1)||'. '|| Person.LastName AS PersonName, Message.Org, Org.Name AS OrgName, Encrypted, Message.ModDate FROM Message INNER JOIN Person ON Message.Person = Person.PersonID INNER JOIN Addressee ON Message.MessageID = Addressee.Message LEFT OUTER JOIN Org ON Message.Org = Org.OrgID WHERE Addressee.Person=?" + ("" if old else " AND Delivered IS NULL") + " AND (? IS NULL OR Message.ModDate >= ?) AND  (? IS NULL OR Message.ModDate <= ?) LIMIT 100;", (self.user, to_date, to_date, from_date, from_date))
            rows = self.cursor.fetchall()
            if rows:
                self.send({Msg.Type: Msg.Success, "Data": tuple((tuple(row) for row in rows))})
            else:
                self.send({Msg.Type: Msg.NotFound})

    def get_message(self):
        self.log(LogLevel.Info, "GetMessage")
        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized})
            return

        if "ID" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "ID not given"})
            return

        self.cursor.execute("SELECT Person FROM Addressee WHERE Person=? AND Message=?;", (self.user, self.msg["ID"]))
        row = self.cursor.fetchone()
        if row is None:
            self.send({Msg.Type: Msg.NotFound})
            return

        self.cursor.execute("SELECT Heading, Text, Data, Person, Org, Encrypted FROM Message WHERE MessageID=?;", (self.msg["ID"], ))
        row = self.cursor.fetchone()
        message = {"Sender": row["Person"]}
        if row["Heading"]:
            message["Heading"] = row["Heading"]
        if row["Text"]:
            message["Text"] = row["Text"]
        if row["Data"]:
            message["Data"] = row["Data"]
        if row["Encrypted"]:
            message["Encrypted"] = True

        self.cursor.execute("SELECT Person, Org, Delivered, Read FROM Addressee WHERE Message=?;", (self.msg["ID"],))
        rows = self.cursor.fetchall()
        addressees = []
        for row in rows:
            addressees.append({"Person": row["Person"], "Org": row["Org"], "Delivered": row["Delivered"], "Read": row["Read"]})
        message["Addressees"] = addressees

        self.send({Msg.Type: Msg.Success, "Message": message})
        self.cursor.execute("UPDATE Addressee SET Delivered=CURRENT_TIMESTAMP WHERE Person=? AND Message=?;", (self.user, self.msg["ID"]))
        self.cnx.commit()

    def message_read(self):
        if "ID" not in self.msg:
            self.send({Msg.Type: Msg.Invalid, "Text": "Message ID not given"})
            return
        self.log(LogLevel.Info, "MessageRead| message: " + str(self.msg["ID"]))

        if self.user is None:
            self.send({Msg.Type: Msg.NotAuthorized})
            return

        self.cursor.execute("SELECT Person FROM Addressee WHERE Person=? AND Message=?;", (self.user, self.msg["ID"]))
        row = self.cursor.fetchone()
        if row is None:
            self.send({Msg.Type: Msg.NotFound})
            return

        self.cursor.execute("UPDATE Addressee SET Read=CURRENT_TIMESTAMP WHERE Person=? AND Message=?;", (self.user, self.msg["ID"]))
        self.cnx.commit()
        self.send({Msg.Type: Msg.Success})

    def get_role(self, person, org):
        self.cursor.execute("SELECT Type, Title, Administrator FROM Role WHERE Person=? AND Org=?;", (person, org))
        row = self.cursor.fetchone()
        return row

    def is_administrator(self, user, org): # run up the org hierarchy to make Administrator privilege inheritable
        self.cursor.execute("WITH RECURSIVE ParentOrg(id) AS ("
            "SELECT ? "
            "UNION ALL "
            "SELECT Org.Parent FROM Org, ParentOrg WHERE Org.OrgID=ParentOrg.id AND Org.Parent IS NOT NULL) "
            "SELECT Person FROM Role WHERE Person=? AND Administrator=1 AND (Role.Org IS NULL OR Role.Org IN ParentOrg);", (org, user))
        row = self.cursor.fetchone()
        return (row is not None)

    def log(self, level, message, user=None):
        #print(message, file=sys.stderr)
        if level >= self.server.log_level:
            self.cursor.execute("INSERT INTO Log (Session, Level, Message, User) VALUES (?,?,?,?)", (id(self), level, message, user if user else self.user));
            self.cnx.commit()

    def hash(self, string):    # to allow inheritance
        return hash(string)

class Server(object):
    """
    A JSON application server intended for serving up data for Pylax front end.
    """
    backlog = 5

    def __init__(self, host, port=default_port, database="Hinterland.db"):
        self.sessions = []
        self.socket = socket.socket()
        self.socket.bind((host, port))
        self.socket.listen(Server.backlog)
        self.database = database
        self.up = True
        self.log_level = 40

        self.private_key = None
        self.public_key = None

        self.cnx = sqlite3.connect(database, detect_types=sqlite3.PARSE_DECLTYPES)
        self.cnx.row_factory = sqlite3.Row
        self.cursor = self.cnx.cursor()
        self.cursor.execute("SELECT Name FROM Enum WHERE Type=0 AND Item=1;")
        row = self.cursor.fetchone()
        if row is None:
            raise Exception("Not a valid database")

        self.token = row["Name"]
        if self.token is None:
            self.token = uuid.uuid4().hex
            self.cursor.execute("UPDATE Enum SET Name=?, ModUser=0, ModDate=CURRENT_TIMESTAMP WHERE Type=0 AND Item=1;", (self.token,))
            self.cnx.commit()

        if nacl_available:
            self.cursor.execute("SELECT Name FROM Enum WHERE Type=2 AND Item=0;")
            row = self.cursor.fetchone()
            self.private_key =  None if row["Name"] is None  else pickle.loads(row["Name"])
            if self.private_key is None:
                self.generate_key_pair()
            else:
                self.cursor.execute("SELECT Name FROM Enum WHERE Type=2 AND Item=1;")
                row = self.cursor.fetchone()
                if row is None:
                    raise Exception("Not a valid database")
                self.public_key =  pickle.loads(row["Name"])

    def run(self, session_class):
        print("Hinterland running.", file=sys.stderr)
        if nacl_available:
            print("Encryption available, public key fingerprint:", fingerprint(self.public_key))
        while self.up:
            (clientsocket, address) = self.socket.accept()
            session = session_class(clientsocket, address, self)
            self.sessions += [session]
            session.start()
            #print("Hinterland session "+ str(len(self.sessions) - 1) + ", ID " + str(session.ident) + " starting.", file=sys.stderr)
            time.sleep(1)

        try:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
        except Exception as e:
            pass
        self.cnx.commit()
        self.cnx.close()
        print("Hinterland halted.", file=sys.stderr)

    def generate_key_pair(self):
        if nacl_available:
            self.private_key, self.public_key = generate_key_pair()
            self.save_key_pair()

    def save_key_pair(self):
            self.cursor.execute("UPDATE Enum SET Name=?, ModUser=0, ModDate=CURRENT_TIMESTAMP WHERE Type=2 AND Item=0;", (pickle.dumps(self.private_key),))
            self.cursor.execute("UPDATE Enum SET Name=?, ModUser=0, ModDate=CURRENT_TIMESTAMP WHERE Type=2 AND Item=1;", (pickle.dumps(self.public_key),))
            self.cnx.commit()

    def export_key(self, file_name=None):
        if file_name is None:
            file_name = self.token + " private.key"
        export_key(self.private_key, file_name)

    def import_key(self, file_name=None):
        if file_name is None:
            file_name = self.token + " private.key"
        self.private_key = import_key(file_name)
        self.public_key = self.private_key.public_key
        self.save_key_pair()


class Client(object):

    def __init__(self, host, port=default_port, database="Backyard.db", encrypted=False, hide_token=False):
        self.key = None             # public key of server
        self.private_key = None
        self.public_key = None
        self.socket = socket.socket()
        self.user = None
        self.msg = None
        self.status_message = None
        self.database = database
        self.site = None
        self.encrypted = False
        self.cnx = sqlite3.connect(database, detect_types=sqlite3.PARSE_DECLTYPES)
        self.cnx.row_factory = sqlite3.Row
        self.cursor = self.cnx.cursor()
        self.cursor.execute("SELECT Name FROM Enum WHERE Type=0 AND Item=1;")
        row = self.cursor.fetchone()
        if row is None:
            raise Exception("Not a valid database")

        if hide_token:
            self.token = "00000000000000000000000000000000"
        else:
            self.token = row["Name"]
        if self.token is None:
            self.generate_token()

        if nacl_available:
            self.cursor.execute("SELECT Name FROM Enum WHERE Type=2 AND Item=0;") # public key
            row = self.cursor.fetchone()
            if row is None:
                raise Exception("Not a valid database")
            self.private_key = None if row["Name"] is None else pickle.loads(row["Name"])
            if self.private_key is None:
                self.generate_key_pair()
            else:
                self.cursor.execute("SELECT Name FROM Enum WHERE Type=2 AND Item=1;") # private key
                row = self.cursor.fetchone()
                if row is None:
                    raise Exception("Not a valid database")
                self.public_key =  pickle.loads(row["Name"])
        elif encrypted:
            self.status_message = "PyNaCl library not installed. Encrypted communication not possible."
            return

        try:
            self.socket.connect((host, port))
            self.socket.send(bytes([0x06, 0x0E, version_info[0], version_info[1]]))
        except (ConnectionRefusedError) as e:
            self.status_message = "Hinterland server not up."
            return

        hi = self.socket.recv(4)
        if len(hi) != 4 or hi[:2] != bytes([0x06, 0x0E]): # magic number
            self.status_message = "No Hinterland server"
            return

        if encrypted:
            self.send({Msg.Type: Msg.Connect, "Token": self.token, "HL Version": version_info, "PublicKey": self.public_key})
        else:
            self.send({Msg.Type: Msg.Connect, "Token": self.token, "HL Version": version_info})

        self.msg = self.receive()
        if self.msg[Msg.Type] == Msg.Success:
            self.server_id = self.msg["ID"]
            self.status_message = None
            if encrypted and "PublicKey" in self.msg:
                self.key = self.msg["PublicKey"]
                self.encrypted = True
            self.set_site(self.server_id, self.key)
        else:
            self.status_message = "Could not connect to server."

    def __del__(self):
        self.close()

    def generate_token(self):
        self.token = uuid.uuid4().hex
        self.cursor.execute("UPDATE Enum SET Name=?, ModUser=0, ModDate=CURRENT_TIMESTAMP WHERE Type=0 AND Item=1;", (self.token,))
        self.cnx.commit()

    def generate_key_pair(self):
        if nacl_available:
            self.private_key, self.public_key = generate_key_pair()
            self.save_key_pair()

    def save_key_pair(self):
        self.cursor.execute("UPDATE Enum SET Name=?, ModUser=0, ModDate=CURRENT_TIMESTAMP WHERE Type=2 AND Item=0;", (pickle.dumps(self.private_key),))
        self.cursor.execute("UPDATE Enum SET Name=?, ModUser=0, ModDate=CURRENT_TIMESTAMP WHERE Type=2 AND Item=1;", (pickle.dumps(self.public_key),))
        self.cnx.commit()

    def send(self, data):
        bytes = pickle.dumps(data)
        if self.encrypted:
            box = Box(self.private_key, self.key)
            encrypted = box.encrypt(bytes, nacl.utils.random(Box.NONCE_SIZE))
            send(self.socket, pickle.dumps(encrypted))
        else:
            send(self.socket, bytes)

    def receive(self):
        data = receive(self.socket)
        if self.encrypted:
            box = Box(self.private_key, self.key)
            decrypted = box.decrypt(pickle.loads(data))
            return pickle.loads(decrypted)
        else:
            return pickle.loads(data)

    def exchange(self, data):
        self.send(data)
        self.msg = self.receive()
        if  self.msg[Msg.Type] == Msg.Success:
            return True
        else:
            if "Text" in self.msg:
                self.status_message = self.msg["Text"]
            elif self.msg[Msg.Type] == Msg.NotFound:
                self.status_message = "Nothing found"
            elif self.msg[Msg.Type] == Msg.NotAuthorized:
                self.status_message = "Not authorized"
            else:
                self.status_message = None
            return False

    def set_site(self, site_name, key):
        self.cursor.execute("SELECT SiteID FROM Site WHERE Name=?;", (site_name,))
        site = self.cursor.fetchone()
        if site is None:
            self.cursor.execute("INSERT INTO Site (Name, PublicKey) VALUES (?,?);", (site_name, pickle.dumps(key)))
            self.site = self.cursor.lastrowid
        else:
            self.site = site["SiteID"]
            self.cursor.execute("UPDATE Site SET LastConnection=CURRENT_TIMESTAMP WHERE SiteID=?;", (self.site, ))
        self.cnx.commit()
        return True

    def save_person(self, data):
        self.cursor.execute("SELECT PersonID, Handle, FirstName, LastName PublicKey FROM Person WHERE Site=? AND PersonID=?;", (self.site, data["ID"],))
        r = self.cursor.fetchone()
        if r is None:
            self.cursor.execute("INSERT INTO Person (Site, PersonID, FirstName, MiddleName, LastName, Transcription, Handle, PublicKey) VALUES (?,?,?,?,?,?,?,?) ;", (self.site, data["ID"], data["FirstName"], data["MiddleName"], data["LastName"], data["Transcription"], data["Handle"], data["PublicKey"]))
        else:
            self.cursor.execute("UPDATE Person SET FirstName=?, MiddleName=?, LastName=?, Transcription=?, Handle=?, PublicKey=?, ModDate=CURRENT_TIMESTAMP WHERE Site=? AND PersonID=?;", (data["FirstName"], data["MiddleName"], data["LastName"], data["Transcription"], data["Handle"], data["PublicKey"], self.site, data["ID"]))
        self.cnx.commit()
        return True

    def download_person(self, id):
        if self.exchange({Msg.Type: Msg.GetPerson, "ID": id}):
            return self.save_person(self.msg["Data"])
        else:
            return False

    def get_persons_key(self, site, person):
        self.cursor.execute("SELECT PublicKey FROM Person WHERE Site=? AND PersonID=?;", (site, person))
        row = self.cursor.fetchone()
        if row is None:
            return None
        else:
            return pickle.loads(row["PublicKey"])

    def sign_on(self, handle, first_name, last_name, password=None, e_mail=None, transcription = None):
        msg={Msg.Type: Msg.SignOn, "Handle": handle, "FirstName": first_name, "LastName": last_name}
        if e_mail is not None:
            msg["EMail"] = e_mail
        if transcription is not None:
            msg["Transcription"] = transcription
        if password is not None:
            password = hash(password.encode("utf-8"))
            msg["Password"] = password
        """
        if nacl_available:
            private_key, public_key = generate_key_pair()
            if self.exchange({Msg.Type: Msg.Key, "PublicKey": public_key}):
                if "PublicKey" in self.msg:
                    self.key = self.msg["PublicKey"]
                self.private_key, self.public_key = private_key, public_key
            else:
                return False
        """
        self.cursor.execute("SELECT PersonID FROM Person WHERE Site=? AND Handle=?;", (self.site, handle))
        exists = self.cursor.fetchone()
        if exists is not None:
            session.status_message = "User '" + handle + "' already signed on."
            return False

        if self.exchange(msg):
            self.user = self.msg["ID"]
            public_key = pickle.dumps(self.public_key) if self.public_key is not None else None
            private_key = pickle.dumps(self.private_key) if self.private_key is not None else None
            self.cursor.execute("INSERT INTO Person (Site, PersonID, FirstName, LastName, Transcription, Handle, EMail, Password, PublicKey, PrivateKey) VALUES (?,?,?,?,?,?,?,?,?,?) ;", (self.site, self.user, first_name, last_name, transcription, handle, e_mail, password, public_key, private_key))
            self.cursor.execute("UPDATE Site SET Me=?, PrivateKey=?, ModUser=?, ModDate=CURRENT_TIMESTAMP WHERE SiteID=?;", (self.user, pickle.dumps(self.private_key) if self.private_key is not None else None, self.user, self.site))
            self.cnx.commit()
            return True
        else:
            return False

    def get_me(self):
        self.cursor.execute("SELECT Me FROM Site WHERE SiteID=?;", (self.site, ))
        site = self.cursor.fetchone()
        return None if site is None else site["Me"]

    def log_in(self, user=None, password=None):
        msg={Msg.Type: Msg.LogIn}

        if password is not None:
            password = hash(password.encode("utf-8"))

        if user is None:
            me = self.get_me()
            if me is None:
                self.status_message = "No user ID provided."
                return False
            else:
                msg["ID"] = me
                if password is  None:
                    self.cursor.execute("SELECT Password FROM Person WHERE Site=? AND PersonID=?;", (self.site, me))
                    row = self.cursor.fetchone()
                    password = None if row is None or row["Password"] is None else row["Password"]
        else:
            if type(user) == int:
                msg["ID"] = user
            elif len(list(filter(str.isdigit, user))) == len(user):
                msg["ID"] = int(user)
            else:
                msg["Handle"] = user

        if password is not None:
            msg["Password"] = password

        if self.exchange(msg):
            self.user = self.msg["ID"]
            return True
        else:
            self.status_message = self.msg["Text"]
            return False

    def change_key(self):
        if not nacl_available:
            session.status_message = "PyNaCl not installed"
            return False

        private_key, public_key = generate_key_pair()
        if self.exchange({Msg.Type: Msg.Key, "PublicKey": public_key}):
            if "PublicKey" in self.msg:
                self.key = self.msg["PublicKey"]
            self.private_key, self.public_key = private_key, public_key
            self.save_key_pair()
        else:
            return False
        if self.user is not None:
            self.cursor.execute("UPDATE Person SET PrivateKey=?, PublicKey=?, ModUser=?, ModDate=CURRENT_TIMESTAMP WHERE Site=?, PersonID=?;", (pickle.dumps(self.private_key), pickle.dumps(self.public_key), self.user, self.site, self.user))
            self.cursor.execute("UPDATE Site SET PublicKey=?, PrivateKey=?, ModUser=?, ModDate=CURRENT_TIMESTAMP WHERE SiteID=?;", (self.key, pickle.dumps(self.private_key), elf.site))
            self.cnx.commit()
        return True

    def send_message(self, message):
        if "Heading" in message:
            heading = message["Heading"]
        else:
            heading = None
        if "Text" in message:
            text = message["Text"]
        else:
            text = None
        if "Data" in message:
            data = message["Data"]
        else:
            data = None
        if "Encrypted" in message:
            encrypted = message["Encrypted"]
        else:
            encrypted = None

        if encrypted is True:
            if not nacl_available:
                self.status_message = "Encryption not available, PyNaCl not installed"
                return False

            if len(message["Addressees"]) != 1:
                self.status_message = "An encrypted message needs to have exactly one addressee."
                return False

            if "Person" not in message["Addressees"][0]:
                self.status_message = "No Person in addressees"
                return False

            key = self.get_persons_key(self.site, message["Addressees"][0]["Person"])
            if key is None:
                self.status_message = "No public key for the addressee"
                return False

            box = Box(self.private_key, key)
            if heading:
                message["Heading"] = box.encrypt(pickle.dumps(heading), nacl.utils.random(Box.NONCE_SIZE))
            if text:
                message["Text"] = box.encrypt(pickle.dumps(text), nacl.utils.random(Box.NONCE_SIZE))
            if data:
                message["Data"] = box.encrypt(pickle.dumps(data), nacl.utils.random(Box.NONCE_SIZE))

        return self.exchange({Msg.Type: Msg.SendMessage, "Message": message})


    def get_message(self, id):
        if not self.exchange({Msg.Type: Msg.GetMessage, "ID": id}):
            return False

        message = self.msg["Message"]
        if "Encrypted" in message and message["Encrypted"] is True:
            if not nacl_available:
                self.status_message = "Message decryption not possible, PyNaCl not installed"
                return False
            key = self.get_persons_key(self.site, message["Sender"])
            if key is None:
                self.status_message = "No public key for the sender"
                return False
            box = Box(self.private_key, key)
            if "Heading" in message:
                message["Heading"] = pickle.loads(box.decrypt(message["Heading"]))
            if "Text" in message:
                message["Text"] = pickle.loads(box.decrypt(message["Text"]))
            if "Data" in message:
                message["Data"] = pickle.loads(box.decrypt(message["Data"]))

        #self.cursor.execute("INSERT INTO Message (Person, Org, Heading, Text, Data, Encrypted) VALUES (?,?,?,?,?,?) ;", (message["Sender"], org, heading, text, data, encrypted))
        message_id = self.cursor.lastrowid

        return message

    def close(self):
        self.socket.close()

    def export_key(self, file_name=None):
        if file_name is None:
            file_name = self.token + " private.key"
        export_key(self.private_key, file_name)

    def import_key(self, file_name=None):
        if file_name is None:
            file_name = self.token + " private.key"
        self.private_key = import_key(file_name)
        self.public_key = self.private_key.public_key
        self.save_key_pair()
