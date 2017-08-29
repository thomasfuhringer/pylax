# example of Hinterland server app
# Thomas Führinger, 2017-02-02

import hinterland as hl

host = "localhost"
#host = "45.76.133.182"

class MySession(hl.Session):

    def get(self):
        if self.msg["Entity"] == "Item":
            self.cursor.execute("SELECT ItemID, Name FROM Item;")
            row = self.cursor.fetchone()
            if row:
                self.send({hl.Msg.Type: hl.Msg.Success, "Data": {"ID": row["ItemID"], "Name": row["Name"]}})
            else:
                self.send({hl.Msg.Type: hl.Msg.NotFound})
        else:
            super().get()

server = hl.Server(host)
server.log_level = 0
#server.export_key()
server.run(MySession)
