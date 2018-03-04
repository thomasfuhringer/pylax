# example of Hinterland server app
# Thomas Führinger, 2018-03-03

import hinterland as hl

host = "localhost"
#host = "45.76.133.182"

class MySession(hl.Session):

    def get(self):
        if self.msg["Entity"] == "Item":
            self.cursor.execute("SELECT ItemID, Name, Description FROM Item WHERE Name LIKE :Name LIMIT 100;", self.msg["Parameters"])
            rows = self.cursor.fetchall()
            if rows:
                self.send({hl.Msg.Type: hl.Msg.Success, "Columns": ("ItemID", "Name", "Description"), "Data": tuple((tuple(row) for row in rows))})
            else:
                self.send({hl.Msg.Type: hl.Msg.NotFound})
        else:
            super().get()

    # todo: def set(self):

server = hl.Server(host)
server.log_level = 0
#server.export_key()
server.run(MySession)
