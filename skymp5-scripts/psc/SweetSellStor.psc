Scriptname SweetSellStor extends ObjectReference  
Book Property dogovor  Auto 
objectreference property marker auto
Event OnItemAdded(Form akBaseItem, int aiItemCount, ObjectReference akItemReference, ObjectReference akSourceContainer)
if game.getplayer().getitemcount(dogovor)>=1
marker.placeatme(akBaseItem)
endif
endevent




