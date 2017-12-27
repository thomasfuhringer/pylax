# Pylax example program, 2017 by Thomas Führinger
import pylax

form = pylax.Form(None, 20, 10, 680, 480, "Hello World")
label = pylax.Label(form, 30, 30, -20, -20, "Welcome to Pylax!")
label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center