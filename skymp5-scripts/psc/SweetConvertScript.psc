Scriptname SweetConvertScript extends ObjectReference  
Message property Mes auto
FormList property armorform auto 
ObjectReference property Convert auto 
miscobject property gold auto
int trash

event OnItemAdded(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akSourceContainer)
if armorform.Find(akBaseItem) <= 0
debug.notification("Incorrect item. You can convert only armors, weapons or clothes")
trash = getitemcount(akBaseItem)
game.getplayer().additem(akBaseItem, trash)
removeitem(akBaseItem, trash)
endif
endevent

