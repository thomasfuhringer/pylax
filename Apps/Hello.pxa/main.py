# Pylax example program, 2017 by Thomas Führinger
import pylax

form = pylax.Form(None, 20, 10, 680, 480, "Hello World")
label = pylax.Label(form, 12, 12, -12, -12, "Welcome to Pylax!")
label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center