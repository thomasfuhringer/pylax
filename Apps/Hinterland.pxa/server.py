# example of Hinterland server app
# Thomas Führinger, 2018-08-03

import sqlite3, hinterland as hl

#host = "localhost"
host = "45.76.133.182"

class MySession(hl.Session):

    def get(self):
        if self.msg["Entity"] == "Item":
            self.cursor.execute("SELECT ItemID, Name, Description FROM Item WHERE Name LIKE :Name LIMIT 100;", self.msg["Parameters"])
            rows = self.cursor.fetchall()
            if rows:
                self.send({hl.Msg.Type: hl.Msg.Success, "Columns": ("ItemID", "Name", "Description"), "Data": tuple((tuple(row) for row in rows))})
            else:
                self.send({hl.Msg.Type: hl.Msg.NotFound})
            
        elif self.msg["Entity"] == "Form":

            with open("HostedForm.py", "r") as myfile:
                program = myfile.read()
                #code = compile(program, "<string>", "exec")
                self.send({hl.Msg.Type: hl.Msg.Success, "Code": program})
            
        else:
            super().get()
            
    def set(self):
        if self.msg["Entity"] == "Item":
            try:
                if "Parameters" in self.msg:
                    self.cursor.execute("UPDATE Item SET Name=:Name, Description=:Description  WHERE ItemID = :ItemID;", {**self.msg["Parameters"], **self.msg["Data"]})
                    #self.cnx.commit()
                    self.send({hl.Msg.Type: hl.Msg.Success})
                else:
                    self.cursor.execute("INSERT INTO Item (Name, Description) VALUES (:Name, :Description);", self.msg["Data"])
                    id = self.cursor.lastrowid
                    #self.cnx.commit()
                    self.send({hl.Msg.Type: hl.Msg.Success, "ItemID": id})
                    
            except sqlite3.Error as error:
                self.cnx.rollback()
                self.send({hl.Msg.Type: hl.Msg.Error, "Text": "Database error " + str(error)})
        else:
            super().set()
            
    def delete(self):
        if self.msg["Entity"] == "Item":
            try:
                self.cursor.execute("DELETE FROM Item WHERE ItemID = :ItemID;", self.msg["Parameters"])
                #self.cnx.commit()
                self.send({hl.Msg.Type: hl.Msg.Success})
            except sqlite3.Error as error:
                self.cnx.rollback()
                self.send({hl.Msg.Type: hl.Msg.Error, "Text": "Database error " + str(error)})
        else:
            super().delete()

server = hl.Server(host)
server.log_level = 0
#server.export_key()
server.run(MySession)
