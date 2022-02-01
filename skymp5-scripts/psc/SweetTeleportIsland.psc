Scriptname SweetTeleportIsland extends ObjectReference  
Objectreference property Texit auto
miscobject property rememberitem auto
Event OnTriggerEnter(ObjectReference akActionRef)
if game.getplayer().getitemcount(rememberitem) >= 1
akActionRef.moveto(Texit)
endif
endevent