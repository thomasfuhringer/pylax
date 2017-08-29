# stop Hinterland server
# Thomas Führinger, 2017-05-02

import hinterland as hl

host = "localhost"
#host = "45.76.133.182"
password = "x"

session = hl.Client(host)#, encrypted=True)
if(session.status_message):
    print("Connection problem:", session.status_message)
    quit()

if not session.log_in("admin"):#, password):
    print("Log in Failed:", session.status_message)
    quit()

if session.exchange({hl.Msg.Type: hl.Msg.Shutdown}):
    session.send({hl.Msg.Type: hl.Msg.Disconnect})
    print("Server down")
else:
    print("Shutdown Failed:", session.status_message)
