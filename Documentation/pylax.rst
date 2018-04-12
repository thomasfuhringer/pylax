:mod:`pylax` --- Database front end
===================================

.. module:: pylax
   :synopsis: A database front end for SQLite using embedded Python as scripting language

.. sectionauthor:: Thomas Führinger <thomasfuhringer@gmail.com>
.. moduleauthor:: Thomas Führinger <thomasfuhringer@gmail.com>

**Binaries:** http://www.pylax.org

**Source code:** https://github.com/thomasfuhringer/pylax

--------------

Pylax is a database front end for SQLite, PostgreSQL and MySQL
that uses embedded Python as scripting language.

To use Pylax you download the Linux binaries, extract them to a directory
and execute *start.sh*.

Hello World
-----------

In keeping with good tradition we shall run a very minimum application
which is called `Hello World`. It is available here:
https://github.com/thomasfuhringer/pylax/tree/master/Apps/Hello.pxa

For Pylax to start a user app it requires two files:
a SQLite database with a file name ending in `.px` and a Python
script file named `main.py` in the same directory.
A larger app might have several Python files but one needs to be
named `main.py`.

Let us look what `Hello World` does::

    import pylax

    form = pylax.Form(None, 20, 10, 680, 480, "Hello World")
    label = pylax.Label(form, 30, 30, -20, -20, "Welcome to Pylax!")
    label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center


At first it imports :mod:`pylax`. This is the connecting point of the
script with the host application.::

    form = pylax.Form(None, 20, 10, 680, 480, "Hello World")

creates a :class:`Form` object with no parent, coordinates
`20, 10, 680, 480` and caption "Hello World".
A :class:`Form` is a subclass of :class:`Widget` and as such can have
a parent, within which it is displayed. In the case of a Form,
however, this does not make sense, so it is None.
Also the coordinates are not used for a :class:`Form` :class:`Widget`
as long as MDI is not available.::

    label = pylax.Label(form, 30, 30, -20, -20, "Welcome to Pylax!")

*label* is an instance of :class:`Label` :class:`Widget` and a child widget of
*form*. It is displayed within the Form at position `30, 30` from top left and
its bottom right corner is `20, 20` from the bottom right of the parent widget.
If the letter two parameters were positive they would decribe the width and hight
of the :class:`Widget` object.::

    label.alignHoriz, label.alignVert = pylax.Align.center, pylax.Align.center

To center the Label's caption "Welcome to Pylax!" within the Widget's
dimensions we assign it a center alignment horizontally and vertically.


Managing Data
-------------

Obviously a database application without data is not all that interesting. So let's
look at an example that actually pulls data and allows manipulating it.

We use a simple table with this definition::

    CREATE TABLE Item (
        ItemID			INTEGER	PRIMARY KEY,
        Name			TEXT    UNIQUE,
        Description		TEXT,
        Picture			BLOB,
        Price   		REAL);


and a very basic script to browse and edit it::

    import pylax

    ds = pylax.Dynaset("Item", "SELECT ItemID, Name, Description, Picture, Price FROM Item;")
    ds.autoColumn = ds.add_column("ItemID", int, format="{:,}", key=True)
    ds.add_column("Name")
    ds.add_column("Description", str)
    ds.add_column("Picture", bytes)
    ds.add_column("Price", float)

    form = pylax.Form(None, 20, 10, 680, 480, "Test Form", ds)

    labelFormCaption = pylax.Label(form, 1, 1, 40, 20, dynaset=ds, column="Name", visible=False)
    labelFormCaption.captionClient = form # passes any assigment to property 'data' on to property 'caption' of the captionClient

    ds.buttonEdit = pylax.Button(form, -360, -40, 60, 20, "Edit")
    ds.buttonNew = pylax.Button(form, -290, -40, 60, 20, "New")
    ds.buttonDelete = pylax.Button(form, -220, -40, 60, 20, "Delete")
    ds.buttonUndo = pylax.Button(form, -150, -40, 60, 20, "Undo")
    ds.buttonSave = pylax.Button(form, -80, -40, 60, 20, "Save")

    selectionTable = pylax.Table(form, 20, 50, -320, -50, dynaset=ds, label = pylax.Label(form, 20, 20, 90, 20, "Select:"))
    selectionTable.add_column("Name", 70, "Name")
    selectionTable.add_column("Description", 100, "Description")
    selectionTable.showRowIndicator = True

    entryID = pylax.Entry(form, -200, 60, 40, 20, dynaset=ds, column="ItemID", dataType=int, label = pylax.Label(form, -300, 62, 70, 20, "ID"))
    entryID.editFormat="{:,}"
    entryID.alignHoriz = pylax.Align.left
    entryName = pylax.Entry(form, -200, 90, -110, 20, dynaset=ds, column="Name", dataType=str, label = pylax.Label(form, -300, 92, 70, 20, "Name"))
    entryPrice = pylax.Entry(form, -200, 120, -110, 20, dynaset=ds, column="Price", dataType=float, label = pylax.Label(form, -300, 122, 70, 20, "Price"))
    entryPrice.format="{0:,.2f}"

    r = ds.execute()


Here we use a :class:`Dynaset` to select data from the database and hold
it in an internal table.

...

--------------

.. _pylax-module-contents:

Module Functions and Constants
==============================

.. function:: append_menu_item(menuItem)

    Adds a :class:`MenuItem` to the 'Apps' menu of Pylax.


.. function:: message(message[, title])

    Shows a message box displaying the string *message*,
    using the string *title* as window title.


.. function:: status_message(message)

    Displays string *message* in the status bar.


.. data:: version_info

    The version number as a tuple of integers.


.. data:: copyright

    Copyright notice.



Enumerations
------------

.. data:: Align

    :class:`Enum` for alignment of text in widgets, possible values:
    *left, right, center, top, bottom, block*



.. _pylax-classes:

Classes
=======

.. _pylax-class-dynaset:

Dynaset
-------

.. class:: Dynaset(table[, query, parent, connecion])

    A dynaset manages the data traffic between the database and :class:`Widget` objects.
    It relates to a table in the database and holds a subset of the data in it.
    The data can be manipulated by :class:`Widget` objects and written back to the database after that.
    For display purposes it can also hold data from related tables in columns which
    are not written back to the database.
    *table* is the name of the primary table in the database. *query* is
    the SQL string used to pull data.
    If a *parent* :class:`Dynaset` is given it will be used to synchronize a master-detail
    relationship.
    *connecion* can be a :class:`sqlite3.Connection` or :class:`psycopg2.extensions.connection`
    or :class:`mysql.connector.connection.MySQLConnection`
    or a `Hinterland` session to be used instead of the default connection
    (which is to the SQLite database opened with the App file).


    *Attributes and methods*


    .. attribute:: parent

        Master :class:`Dynaset`

    .. attribute:: autoColumn

        Can point to one of the Dynaset's :data:`DynasetColumn` objects to indicate
        that the value in this column will be generated by the database on insert.

    .. attribute:: lastRowID

        Row ID generated by the database for :attr:`autoColumn` at last insert.

    .. attribute:: row

        A Dynaset has a row pointer. Through this attribute it is possible to
        get or set the current index number of the current row.

        *-1* means no row is selected.


    .. attribute:: rows

        Row count. -1 if still not executed.

    .. attribute:: query

        Query string used to pull data.

    .. attribute:: autoExecute

        If :const:`True` :meth:`execute` will be triggered if parent row has changed.

    .. attribute:: buttonNew

        A :class:`Button` assigned here will enable the user to insert a new row.
        It will be enabled and disabled as appropriate according to the state of the Dynaset.

    .. attribute:: buttonEdit

        A :class:`Button` assigned here will enable the user to set the Dynaset and all
        child Dynasets into Edit mode.
        It will be enabled and disabled as appropriate according to the state of the Dynaset.

    .. attribute:: buttonUndo

        A :class:`Button` assigned here will enable the user to revert changes of the current row.
        It will be enabled and disabled as appropriate according to the state of the current row.

    .. attribute:: buttonSave

        A :class:`Button` assigned here will enable the user to save changes in the
        Dynaset and all child Dynasets.
        It will be enabled and disabled as appropriate according to the state of the
        Dynaset and children.

    .. attribute:: buttonDelete

        A :class:`Button` assigned here will enable the user to mark the current row
        for deletion.
        It will be enabled and disabled as appropriate according to the state of the Dynaset.

    .. attribute:: buttonOK

        A :class:`Button` assigned here will be enabled if a row is selected.
        It is for use in record selector dialogs.

    .. attribute:: frozen

        If :const:`True` the Dynaset is in the frozen state.
        This means that some child :class:`Dynaset` is in the process of being edited or has been
        changed. For that reason the user will not be allowed to change the current row.
        Bound Widgets serving as row selectors (currently only :class:`Table`) are disabled.

    .. attribute:: on_parent_changed

        A callback assigned here will be triggered every time the selected record in the
        parent Dataset has changed.

    .. attribute:: on_changed

        A callback assigned here will be triggered every time the parent Dataset has changed.
        The signature of the callback is *on_changed(self, row, column)*.


    .. attribute:: validate

        A callback assigned here will be called before save.
        If it does not return :const:`True` saving will be discontinued.


    .. attribute:: whoCols

        If :const:`True` the table has columns `ModDate` and `ModUser` and these will
        be populated automatically.

    .. attribute:: connection

        :class:`sqlite3.Connection` used to access the database.


    .. method:: add_column(name [, type, key, format, default, defaultFunction, parent])

        Constructs a :data:`DynasetColumn` and adds it to the Dynaset.


    .. method:: get_column(name)

        Returns the column with the given name.

    .. method:: execute([parameters, query])

        Run the query and load the result set into the internal table.

        *parameters* is a Python dict of paramenters with values that will be substituted
        in the query.

        *query* will be set as the Dynaset's new query before running, if given.

    .. method:: get_row([number])

        Returns the :data:`DynasetRow` with the given row number or,
        if no number provided, the current row.

    .. method:: get_data(column[, row])

        Returns the data value for a given column and row.
        *column* can be a str with the name of the column or a :data:`DynasetColumn`
        If *row* is not provided, the current row will be used.

    .. method:: set_data(column, data[, row])

        Sets the data value for a given column and row.

        *column* can be a str with the name of the column or a :data:`DynasetColumn`
        object.

        If *row* is not provided, the current row will be used.

    .. method:: get_row_data([row])

        Returns the :data:`DynasetRow` at the given row.
        If *row* is not provided, the current row will be used.

    .. method:: get_column_data_sum(column)

        Adds all the values in the given column and returns it as int or float
        (depending on the data type associated with the column).
        *column* can be a str with the name of the column or a :data:`DynasetColumn`

    .. method:: clear()

        Deletes all the rows of data.

    .. method:: save()

        Writes all changes to the corresponding database table.


    *Named tuples*

    A :class:`Dynaset` stores the data retrieved from the database in a table
    the elements of which can be accessed as :class:`~collections.namedtuple`
    objects.


.. data:: DynasetColumn

    A :class:`~collections.namedtuple` with these elements:

	*name*:  Name of column in query

	*index*: Position in query

	*type*: Data type

	*key*: True if column is part of the primary key, False if non-key database column, None if not part of the database table

	*default*: Default value

	*get_default*: Function providing default value

	*format*: Default display format

	*parent*: Coresponding column in parent Dynaset


.. data:: DynasetRow

    A :class:`~collections.namedtuple` with these elements:

	*data*: Tuple of data pulled, in the order as queried

	*dataOld*: Tuple of data before modification, or None if the row is clean

	*new*: True if the row is still not in database

	*delete*: True it the row is to be removed from the database



.. _pylax-class-widget:

Widget
------

.. class:: Widget(parent[, left, top, right, bottom, caption, dynaset, column, dataType, format, label, visible])

   Widget is the base class from which all data aware widgets that
   can be displayed on a :class:`Window` are derived.

   The construtor parameters are also available as attributes:

   .. attribute:: parent

      The parent :class:`Widget` within which it is displayed, in many cases
      a :class:`Form`.

   .. attribute:: caption

      Only used with select subclasses (:class:`Window`, :class:`Form`, :class:`Entry`).

   .. attribute:: dynaset

      The :class:`Dynaset` object the widget is bound to.

   .. attribute:: dataColumn

      The :class:`DynasetColumn` object of the :class:`Dynaset` object
      the widget is bound to.

   .. attribute:: dataType

      The Python data type the widget can hold.

   .. attribute:: format

      The Python format string in the :meth:`str.format()` syntax that is used
      to render the data.

   .. attribute:: editFormat

      The Python format string in the :meth:`str.format()` syntax that is used
      to render the data when in edit mode.

   .. attribute:: window

      The :class:`Window` the Widget is on.

   .. attribute:: left

      Distance from left edge of parent, if negative from right.

   .. attribute:: top

      Distance from top edge of parent, if negative from bottom

   .. attribute:: right

      Width or, if zero or negative, distance of right edge from right edge
      of parent

   .. attribute:: bottom

      Height or, if zero or negative, distance of bottom edge from from bottom edge
      of parent

   .. attribute:: visible

      By default :const:`True`

   .. attribute:: label

      A Widget can have a :class:`Label` Widget attached to it.
      This way certain operations on the Widget will have an effect
      on both Widgets (currently unused).

   .. attribute:: data

      The data value currently held


.. _pylax-class-window:

Window
------

.. class:: Window

    A GUI window which serves as a canvas to hold :class:`Widget` objects.
    Itself it is derived from :class:`Widget`.
    It serves also as the base class for :class:`Form`.

    *Attributes and methods*


    .. attribute:: parent

        The parent widget within which it is displayed, in many cases
        a :class:`Form`.

    .. attribute:: caption

        Assigning a str here makes it appear as the Form's title.


    .. attribute:: nameInCaption

        If :const:`True` the name of the Form is prepended if a caption is assigned
        to :attr:`caption`.

    .. attribute:: before_close

        A callable assigned here is called right before the Window is closed,
        typically to perform cleanup tasks.

    .. attribute:: focus

        The :class:`Widget` that currently holds the keyboard focus

    .. attribute:: visible

        By default :const:`True`.

    .. attribute:: readOnly

        If not :const:`True`, the data will not be editable.

    .. attribute:: validate

        A callback assigned here will be called if the content was changed and
        the user tries to navigate to another widget.
        If it does not return :const:`True` change of focus will be undone.

    .. attribute:: position

        A tuple with the *(x, y)* position of the widget on the screen.

    .. attribute:: size

        A tuple with the *(width, height)* of the widget on the screen.

    .. attribute:: minWidth

        The minimum with of the Window.

    .. attribute:: minHeight

        The minimum height of the Window.


    .. method:: close()

        Close and destroy


    .. method:: wait_for_close()

        This blocks until the window is closed (by some other callback).
        For using the Window as a dialog.



.. _pylax-class-form:

Form
----

.. class:: Form

    A Form is the primary container to display Pylax widgets. A Pylax app
    will usually open at least one Form. Currently it is implemented as a tab
    in the client area. The aspiration for the future is to allow an MDI mode.
    Derived from :class:`Window`.

    *Attributes and methods*

    .. attribute:: caption

        Assigning a str here makes it appear as the Form's title.

    .. attribute:: nameInCaption

        If :const:`True` the name of the Form is prepended if a caption is assigned
        to :attr:`caption`.


.. _pylax-class-label:

Label
-----

.. class:: Label

    Shows boilerplate text on the :class:`Window`.

    .. attribute:: captionClient

        If a :class:`Widget` is assigned here, any value assingned to
        :attr:`data` will be passed on to :attr:`caption` of
        that widget.

        Typically the :class:`Form` will be assigned here and the Label
        bound to a name column so that name will appear as the caption of the
        :class:`Form`.



.. _pylax-class-entry:

Entry
-----

.. class:: Entry

    Single line data entry

    *Attributes and methods*

    .. attribute:: inputString

        The currently entered input as a raw string.

    .. attribute:: inputData

        The currently entered input converted to the Widget's data type
        (not yet written to the :class:`Dynaset`,
        for validation purposes.

    .. attribute:: alignHoriz

        Horizontal alignment. By default pylax.Align.left for data type str and
        pylax.Align.right for numeric data types

    .. attribute:: on_click_button

        If a callable is assigned here a Find icon (magnifying glass) will
        be displayed in the Entry and the callable is called when the icon is
        clicked.


.. _pylax-class-combobox:

ComboBox
--------

.. class:: ComboBox

    ComboBox which allows selection from a drop down list of data values.

    .. attribute:: noneSelectable

        If set :const:`True` it is possible to make no selection.


    .. method:: append(value[, key])

        Appends o tuple of (*value*, *key*) to the list of available items.
        *key* is displayed in the widget, *value* returned as
        :attr:`Data`
        If *key* is not given, *value* will be assumed.



.. _pylax-class-mark-down-entry:

MarkDownEntry
-----

.. class:: MarkDownEntry

    Multiple line text entry which supports MarkDown formatting

    *Attributes and methods*

    .. attribute:: inputString

        The currently entered input as a raw string.


.. _pylax-class-button:

Button
------

.. class:: Button

    Push button to trigger a callback.

    .. attribute:: on_click

        If a callable is assigned here it is called when the button is
        clicked.


.. _pylax-class-table:

Table
-----

.. class:: Table

    Displays a :class:`Dynaset` object's data in a table and allows the user to select
    a row.

    .. attribute:: columns

        :class:`List` of the Table's :class:`TableColumn` objects

    .. attribute:: showRowIndicator

        If set :const:`True` a column on the left will be displayed that
        indicates the status of the data in the row.


    .. method:: add_column(caption, width[, data, type, editable, format, widget, autoSize])

        Appends a :class:`TableColumn`
        *data* must be a :class:`DynasetColumn` of the Table's :class:`Dynaset` object.



.. _pylax-class-table-column:

TableColumn
-----------

.. class:: TableColumn

    Column used in a :class:`Table`. Not a subclass of :class:`Widget`.

    .. attribute:: data

        Bound :data:`DynasetColumn` object.

    .. attribute:: type

        Data type, must be in line with the data type of the :data:`DynasetColumn`.

    .. attribute:: format

        Display format



.. _pylax-class-tab:

Tab
---

.. class:: Tab

    A tabbed notebook container.
    Holds :class:`TabPage` objects.


.. _pylax-class-tab-page:

TabPage
-------

.. class:: TabPage(parent)

    Page in a :class:`Tab` Widget.
    *parent* in contructor must be the :class:`Tab` object.


.. _pylax-class-box:

Box
---

.. class:: Box

    Container that can hold other Widgets.

    A :class:`Splitter` object holds two Box Widgets.


.. _pylax-class-splitter:

Splitter
--------

.. class:: Splitter

    A widget with two adjustable panes. Each one is a
    :class:`Box` object.

    .. attribute:: box1

        :class:`Box` object on the left or top.

    .. attribute:: box2

        :class:`Box` object on the right or bottom.

    .. attribute:: vertical

        If :const:`True` panes are arranged top/bottom.

    .. attribute:: box1Resize

        If :const:`True` Box 1 resizes with window

    .. attribute:: box2Resize

        If :const:`True` Box 2 resizes with window

    .. attribute:: position

        Position of the separator

    .. attribute:: spacing

        Width of the separator


.. _pylax-class-image:

Image
------

.. class:: Image

    Displays an image bitmap. Currently only JPG format is supported.
    In Edit mode a click on the widget brings up a file selector dialog
    which allows to select a JPG file to be imported.

    .. attribute:: scale

        If :const:`True` the bitmap will be scaled to fit the current
        size of the widget.


.. _pylax-class-canvas:

Canvas
--------

.. class:: Canvas

    A widget for very basic drawing.

    .. attribute:: on_paint

        Drawing operations should be done in a callback which is assigned here.

    .. attribute:: penColor

        A tuple of (R, G. B) values to be used for drawing.

    .. attribute:: fillColor

        A tuple of (R, G. B) values to be used for filling rectangles.

    .. attribute:: textColor

        A tuple of (R, G. B) values to be used for drawing text.


    .. method:: point(x, y)

        Draws a point at the given coordinates.


    .. method:: move_to(x, y)

        Sets x, y as the origin of the next line operation.


    .. method:: line_to(x, y)

        Draws a line to x, y.


    .. method:: rectangle(x, y, width, height[, radius])

        Draws a rectangle. If *radius* is provided, corners will be rounded.


    .. method:: text(x, y, string)

        Writes *string* at given position.


    .. method:: repaint()

        Triggers the paint process.



.. _pylax-class-menu:

Menu
--------

.. class:: Menu(caption)

    A submenu.

    *caption* must be a str to be displayed in the menu,

    .. method:: append(item)

        Add either a :class:`MenuItem` object or
        a :class:`Menu` object to serve as a submenu.



.. _pylax-class-menu-item:

MenuItem
--------

.. class:: MenuItem(caption, on_click)

    An item in a :class:`Menu` object.
    Can also be inserted into Pylax' `App` menu
    through :func:`append_menu_item`.

    *caption* must be a str to be displayed in the menu,

    *on_click* is the callback to be triggered on selection
