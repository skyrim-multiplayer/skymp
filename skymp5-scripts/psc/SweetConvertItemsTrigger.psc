Scriptname SweetConvertItemsTrigger extends ObjectReference  
Message property Mes auto
FormList property armorform auto 
ObjectReference property Convert auto 
Miscobject property gold auto
int count
event OnActivate(ObjectReference akActionRef)
Int iButton = Mes.Show() 
if (IButton == 1)
count = Convert.getitemcount(armorform)
Convert.removeitem(armorform, count)
Convert.additem(gold, count)
count = 0
elseif (IButton == 1)&&convert.getitemcount(gold) >= 1
debug.notification("Gold dnont be can converted")
endif
endevent



