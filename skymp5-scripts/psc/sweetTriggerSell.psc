Scriptname sweetTriggerSell extends ObjectReference  
MiscObject Property gold  Auto  
Book Property dogovor  Auto 
Message Property ST Auto
Message Property K Auto
Message Property C Auto
Message Property S Auto
objectreference property marker auto
int cena
int kolvo
int pribil
int edinici
int tovar
State Sell
Event OnTriggerEnter(ObjectReference akActionRef)
akActionRef.blockactivation()
if game.getplayer().getitemcount(dogovor) >=1
Int iButton = C.Show()
if (IButton == 0)
gotostate ("")
elseif (Ibutton == 1)
cena = cena + 2
elseif (IButton == 2)
akActionRef.blockactivation(False)
elseif akActionRef.blockactivation()
debug.notification("Dont touch")
endif
endif
endevent
endstate






