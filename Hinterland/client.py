# example conversation with Hinterland server
# Thomas Führinger, 2017-05-02

import socket, pickle, hinterland as hl

host = "localhost"
#host = "45.76.133.182"

password = "x"

session = hl.Client(host)#, encrypted=True)
if(session.status_message):
    print("Connection problem:", session.status_message)
    quit()

if session.key:
    print("Secure connection established.")
    print("Public key fingerprints:")
    print("    Server:    ", hl.fingerprint(session.key))
    print("    Client:    ", hl.fingerprint(session.public_key))
else:
    print("Connection established")
print("Server ID: '" + session.server_id + "'")
#session.export_key()

if session.exchange({hl.Msg.Type: hl.Msg.GetPage}):
    print("Home page:", session.msg["Data"]["Text"])
else:
    print("GetPage Failed:", session.status_message)
    quit()

if session.exchange({hl.Msg.Type: hl.Msg.GetPerson, "Handle": "me"}):
    #print("Person 'me' exists:", session.msg["Data"]["LastName"])
    if not session.log_in("me", password=password):
        print("Log in Failed:", session.status_message)
        quit()
    print("Logged in 'me'")
elif session.msg[hl.Msg.Type] == hl.Msg.NotFound:
    if session.sign_on("me", "Some", "One", password, "what@ever.org", "Some One"):
        print("Person 'me' signed on. User ID", session.msg["ID"])
    else:
        print("Sign On failed:", session.status_message)
        quit()
else:
    print("GetPerson Failed:", session.status_message)
    quit()
"""
if session.exchange({hl.Msg.Type: hl.Msg.GetPage,  "Person": 7}):
    print("Page Person 7", session.msg["Data"]["Text"])
else:
    print("GetPage Failed:", session.status_message)

if session.exchange({hl.Msg.Type: hl.Msg.GetPage, "KeyWord": "X"}):
    print("Page KW x", session.msg["Data"]["Text"])
else:
    print("GetPage Failed:", session.status_message)
"""
if session.exchange({hl.Msg.Type: hl.Msg.GetPageChildren, "ID": 99}):
    print("PageChildren 99")
    for row in session.msg["Data"]:
        print(row[0], row[1],row[2])
elif session.msg[hl.Msg.Type] == hl.Msg.NotFound:
    print("PageChildren not found")
else:
    print("GetPageChildren Failed:", session.status_message)

if session.exchange({hl.Msg.Type: hl.Msg.GetPerson, "ID": 7}):
    print("Person 7", session.msg["Data"]["LastName"])
else:
    print("GetPerson Failed:", session.status_message)

"""
if not session.exchange({hl.Msg.Type: hl.Msg.LogIn, "ID": 7, "Password": hl.hash(password.encode("utf-8")), "UpdateKey": True}):
    print("Log in Failed:", session.status_message)
print("Logged in")
"""

if session.exchange({hl.Msg.Type: hl.Msg.SearchPerson, "Name": "TFU"}):
    print("Persons found", session.msg["Data"])
else:
    print("SearchPerson Failed:", session.status_message)

if session.exchange({hl.Msg.Type: hl.Msg.SendMessage, "Message": {"Heading": "Hi", "Text": "How are you?", "Addressees": ({"Person": 7, "Org": 1},)}}):
    print("Message sent ID", session.msg["ID"])
else:
    print("SendMessage Failed:", session.status_message)
    quit()
"""
if not session.download_person(101):
    print("download_person Failed:", session.status_message)
else:
    if session.send_message({"Heading": "Hi", "Text": "This is secret", "Addressees": ({ "Person": 101},), "Encrypted": True}):
        print("Secret Message ID", session.msg["ID"])
    else:
        print("Sending message problem:", session.status_message)
if session.exchange({hl.Msg.Type: hl.Msg.GetMessageList}):
    print("Messages received", session.msg["Data"])
else:
    print("GetMessageList Failed:", session.status_message)

if session.exchange({hl.Msg.Type: hl.Msg.GetMessage, "ID": 1}):
    print("Message received", session.msg["Message"])
else:
    print("GetMessage Failed:", session.status_message)

if session.exchange({hl.Msg.Type: hl.Msg.MessageRead, "ID": 1}):
    print("Message read", 1)
else:
    print("MessageRead Failed:", session.status_message)

message = session.get_message(3)
if message is False:
    print("get_message Decl", session.status_message)
else:
    print("Message Text", message["Text"])

if not session.exchange({hl.Msg.Type: hl.Msg.SetPage, "KeyWord": "Newer", "Text": "One", "Person": 7, "Org": 1}):
    print("SetPage Failed:", session.status_message)
    quit()
else:
    print(session.msg["PageID"])
    if session.exchange({hl.Msg.Type: hl.Msg.SetPage, "KeyWord": "Newer Child", "Text": "One+", "Person": 7, "Org": 1, "Parent": session.msg["PageID"]}):
        print(session.msg["PageID"])
    else:
        print("SetPage Failed:", session.status_message)

"""
if session.exchange({hl.Msg.Type: hl.Msg.Get, "Entity": "Item"}):
    print(session.msg["Data"])
else:
    print("Get Entity Failed:", session.status_message)


session.send({hl.Msg.Type: hl.Msg.Disconnect})
print("Done")
