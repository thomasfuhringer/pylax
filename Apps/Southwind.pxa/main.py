import pylax
import Person
import Order
import Item
import LegalEntity

def itemMenuItem__on_click():
    Item.launch()
    
def personMenuItem__on_click():
    Person.launch()
    
def orderMenuItem__on_click():
    Order.launch()   
    
def legalEntityMenuItem__on_click():
    LegalEntity.launch()   

itemMenuItem = pylax.MenuItem("_Item", itemMenuItem__on_click)
pylax.append_menu_item(itemMenuItem)
personMenuItem = pylax.MenuItem("_Person", personMenuItem__on_click)
pylax.append_menu_item(personMenuItem)
pylax.append_menu_item(pylax.MenuItem("_Order", orderMenuItem__on_click))
pylax.append_menu_item(pylax.MenuItem("_Legal Entity", legalEntityMenuItem__on_click))

legalEntityMenuItem__on_click()
#orderMenuItem__on_click()
