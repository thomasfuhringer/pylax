# Pylax example program, 2015 by Thomas Führinger
import pylax

form = pylax.Form(None, 20, 10, 680, 480, "Hello World")


image = pylax.Image("Example.bmp")
#image = pylax.Image("Lybniz.ico")
imageView = pylax.ImageView(form, 30, 20, -20, -10)
imageView.data = image

"""

# -- Entry -----------------------
       

w = pylax.Window(form, 30, 20, 120, 210, "Hello Win ῳ")
w.minWidth, w.minHeight = 640, 480
editName = pylax.Entry(w, 30, 20, 90, 20)
editName.focus_in()
w.buttonOK = pylax.Button(w, -160, -40, 50, 20, "OK")
print(form.alignHoriz)
w.run()


def say_hello(self):
    pylax.message("Hello " + self.input, "Yeah!")
    
editName = pylax.Entry(form, 30, 20, 90, 20)
editName.on_click_button=say_hello
editName2 = pylax.Entry(form, 130, 20, 90, 20, dataType=int)
editName3 = pylax.Entry(form, 230, 20, -30, 20)
editName3.on_click_button=say_hello
editName3.data = "Bla"
editName2.data = 7
buttonOK = pylax.Button(form, -160, -40, 50, 20, "OK")
buttonOK.defaultEnter = True;



image = pylax.Image(form, 30, 20, -20, -10)

#print(pylax.ask("Feeling good?", "Poll",no=True, cancel=True))

def sayHelloMenuItem__on_click():
    print(pylax.ask("Feeling good?", "Poll",no=True, cancel=True))

w = pylax.Window(None, 20, 30, 320, 240, "Hello Win")
menu = pylax.Menu("Menu")
sMenu = pylax.Menu("Sub")
sayHelloMenuItem = pylax.MenuItem("Say Hello", sayHelloMenuItem__on_click)
sMenu.append(sayHelloMenuItem)
menu.append(sMenu)
w.menu = menu

w.buttonOK = pylax.Button(w, -160, -40, 50, 20, "OK")
w.buttonOK.defaultEnter = True;
##w.visible = True
w.buttonCancel = pylax.Button(w, -60, -40, 50, 20, "Cancel")

print(w.run())
del w


selector = pylax.Selector(form,  30, 20, 480, 280, "Select")
r = selector.show()
print(r)

# -- Image -----------------------

image = pylax.Image(form, 30, 20, -20, -10)
#print(image.format)
#print(image.parent)
#image.fill = True
#image.load("TFuhrgr.bmp")


w = pylax.Window(form, 30, 20, 120, 210, "Hello Win")
w.show()

w = pylax.Window(None, 20, 30, 680, 480, "Hello Win")
w.show(True, True)
print("back")

def form__before_close(self):
    print("Good bye!", self)    
    
form.before_close = form__before_close

# -- MenuItem -----------------------

def sayHelloMenuItem__on_click():
    print("Hello!")

sayHelloMenuItem = pylax.MenuItem("Say Hello", sayHelloMenuItem__on_click)
pylax.add_menu_item(sayHelloMenuItem)

# -- Canvas -----------------------

def canvas__on_paint(canvas):
    canvas.point(-3, -13)
    canvas.fillColor = (200, 220, 167)
    canvas.rectangle(3, 4, 140,155,30)
    canvas.rectangle(230, 240, 140,155)
    canvas.penColor = (0, 0, 250)
    canvas.move_to(100, 120)
    canvas.line_to(-30, -40)
    canvas.textColor = (250, 0, 0)
    canvas.text(10, 12, "Bla bla")
    
canvas = pylax.Canvas(form, 30, 70, -10, -10)
canvas.frame = True
print(canvas.position)
print(canvas.size)
canvas.on_paint = canvas__on_paint
canvas.repaint()


pylax.show_status_message("Hello 世界!")

# --- Box -------------------------

b = pylax.Box(form, 20, 20, -20, -20, "Welcome to B!")
b.frame= True
label = pylax.Label(b, 30, 30, -20, -20, "Wel&come to Pylax!")
label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center
label.frame= True

# --- Splitter ---------------------

splitter = pylax.Splitter(form, 20, 20, -20, -20, 120)
label = pylax.Label(splitter.box1, 20, 20, -20, -20, "Welcome to Pylax!")
label2 = pylax.Label(splitter.box2, 20, 20, -20, -20, "again")

tab = pylax.Tab(form, 20, 20, -20, -20, "Überschrift")
tabPage = pylax.TabPage(tab, 20, 20, -20, -20, "Hü")
tabPage2 = pylax.TabPage(tab, 20, 20, -20, -20, "Löffel")

label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center


label.captionClient = form
form.nameInCaption = False
label.data = "Bla bla"

from threading import Timer, Lock

class RepeatedTimer(object):    
    def __init__(self, interval, function, *args, **kwargs):
        self._lock = Lock()
        self._timer = None
        self.function = function
        self.interval = interval
        self.args = args
        self.kwargs = kwargs
        self._stopped = True
        if kwargs.pop('autostart', True):
            self.start()

    def start(self, from_run=False):
        self._lock.acquire()
        if from_run or self._stopped:
            self._stopped = False
            self._timer = Timer(self.interval, self._run)
            self._timer.start()
            self._lock.release()

    def _run(self):
        self.start(from_run=True)
        self.function(*self.args, **self.kwargs)

    def stop(self):
        self._lock.acquire()
        self._stopped = True
        self._timer.cancel()
        self._lock.release()
        

def show_time(data):
    print("Fired")
    pylax.status_message(data) 
    
rt = RepeatedTimer(1, show_time, "T World") 


# before_close -------------------------

def before_close():
    pylax.message("Bye!","Application")
    pylax.delete_timer(t)
    return True
	
def before_close_window(self):
    pylax.message("Bye!","Form")
    #return True

pylax.set_before_close(before_close)

dc = pylax.Widget.defaultCoordinate
form = pylax.Form(None, dc, dc, 680, 480, "Hello World")
form.before_close = before_close_window


# - Timer -------------------------
import time
def show_time():
    pylax.status_message(time.ctime())    

form = pylax.Form(None, 20, 10, 680, 480, "Hello World")
t = form.create_timer(show_time, 1000, 5000)


# - Image -------------------------

image = pylax.Image("Example.bmp")
#image = pylax.Image("Lybniz.ico")
imageView = pylax.ImageView(form, 30, 20, -20, -10)
imageView.data = image

# - Tab ---------------------------

tab = pylax.Tab(form, 20, 20, -20, -20, "Überschrift")
tabPage = pylax.TabPage(tab, 0, 0, 0, 0, "Hü")
tabPage2 = pylax.TabPage(tab, 20, 30, -20, -20, "Löffel")
label = pylax.Label(tabPage, 20, 20, -20, -20, "Welcome to Pylax!")

# - Splitter ----------------------

splitter = pylax.Splitter(form, 10, 10, -10, -10)
label = pylax.Label(splitter.box2, 20, 20, -20, -20, "Hi!")
#splitter.vertical = False
#splitter.spacing = 30
splitter.position = -200


"""