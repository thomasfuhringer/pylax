# Pylax version of Lybniz, © 2015 by Thomas Führinger
# demonstration of the Canvas widget

import pylax
from math import *

dc = pylax.Widget.defaultCoordinate

def newWindowMenuItem__on_click():
    form = pylax.Form(None, dc, dc, 640, 480, "Lybniz")
    form.icon = pylax.Image("Lybniz.ico")

    # GUI
    canvas = pylax.Canvas(form, 0, 110, 0, 0)
    canvas.frame = True

    entryY1 = pylax.Entry(form, 40, 10, -250, 20,  label = pylax.Label(form, 10, 10, 25, 20, "y1 ="))
    entryY1.label.textColor = (0, 0, 255)
    entryY1.data = "sin(x)"
    entryXMin = pylax.Entry(form, -200, 10, 70, 20, label = pylax.Label(form, -240, 10, 35, 20, "X min"), dataType=float)
    entryXMin.data = -5.0
    entryYMin = pylax.Entry(form, -80, 10, 70, 20, label = pylax.Label(form, -120, 10, 35, 20, "Y min"), dataType=float)
    entryYMin.data = -3.0
    entryY2 = pylax.Entry(form, 40, 35, -250, 20,  label = pylax.Label(form, 10, 35, 25, 20, "y2 ="))
    entryY2.label.textColor = (10, 255, 10)
    entryXMax = pylax.Entry(form, -200, 35, 70, 20, label = pylax.Label(form, -240, 35, 35, 20, "X max"), dataType=float)
    entryXMax.data = 5.0
    entryYMax = pylax.Entry(form, -80, 35, 70, 20, label = pylax.Label(form, -120, 35, 35, 20, "Y max"), dataType=float)
    entryYMax.data = 3.0
    entryY3 = pylax.Entry(form, 40, 60, -250, 20,  label = pylax.Label(form, 10, 60, 25, 20, "y3 ="))
    entryY3.label.textColor = (255, 0, 0)
    entryXScale = pylax.Entry(form, -200, 60, 70, 20, label = pylax.Label(form, -240, 60, 35, 20, "X scale"), dataType=float)
    entryXScale.data = 1.0
    entryYScale = pylax.Entry(form, -80, 60, 70, 20, label = pylax.Label(form, -120, 60, 35, 20, "Y scale"), dataType=float)
    entryYScale.data = 1.0

    executeButton = pylax.Button(form, -50, 85, 40, 20, "&Plot")
    executeButton.defaultEnter = True;

    def canvas__on_paint(sender):
        canvasWidth, canvasHeight = canvas.size
        xMin = entryXMin.data
        xMax = entryXMax.data
        yMin = entryYMin.data
        yMax = entryYMax.data

        def canvasX(x):
            "Calculate position on canvas to point on graph"
            return int((x - xMin) * canvasWidth/(xMax - xMin))

        def canvasY(y):
            return int((yMax - y) * canvasHeight/(yMax - yMin))

        def graphX(x):
            "Calculate position on graph from point on canvas"
            return x  * (xMax - xMin) / canvasWidth + xMin

        def GraphY(y):
            return yMax - (y * (yMax - yMin) / canvasHeight)

        # draw cross
        canvas.move_to(canvasX(0), 0)
        canvas.line_to(canvasX(0), canvasHeight + 1)
        canvas.move_to(0, canvasY(0))
        canvas.line_to(canvasWidth + 1, canvasY(0))

        # draw scaling x
        iv = int(entryXScale.data * canvasWidth/(xMax - xMin))
        os = canvasX(0) % iv
        for i in range(int(canvasWidth / iv + 1)):
            canvas.move_to(os + i * iv, canvasY(0) - 5)
            canvas.line_to(os + i * iv, canvasY(0) + 6)

        # draw scaling y
        iv = int(entryYScale.data * canvasHeight/(yMax - yMin))
        os = canvasY(0) % iv
        for i in range(int(canvasHeight / iv + 1)):
            canvas.move_to(canvasX(0) - 5, i * iv + os)
            canvas.line_to(canvasX(0) + 6, i * iv + os)

        # plot
        for e in ((entryY1.data, (0, 0, 250)), (entryY2.data, (0, 250, 0)), (entryY3.data, (250, 0, 0))):
            canvas.penColor = e[1]
            penDown = False

            if e[0] != None:
                for i in range(canvasWidth):
                    x = graphX(i + 1)
                    try:
                        y = eval(e[0])					
                        yC = canvasY(y)
                    except:
                        yC = -1

                    if yC < 0 or yC > canvasHeight:
                        penDown = False
                    else:
                        if penDown:
                            canvas.line_to(i, yC)
                        else:
                            canvas.move_to(i, yC)
                            penDown = True

    def executeButton__on_click(data):
        canvas.repaint()

    canvas.on_paint = canvas__on_paint
    executeButton.on_click = executeButton__on_click    

newWindowMenuItem = pylax.MenuItem("&New Window", newWindowMenuItem__on_click)
pylax.append_menu_item(newWindowMenuItem)
newWindowMenuItem__on_click()
