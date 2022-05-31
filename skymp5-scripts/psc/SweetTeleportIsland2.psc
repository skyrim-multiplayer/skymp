Scriptname SweetTeleportIsland2 extends ObjectReference
Objectreference property Texit auto
Event OnTriggerEnter(ObjectReference akActionRef)
Form rememberitem = game.getform(0x0840ad63)
if game.getplayer().getitemcount(rememberitem) >= 1
akActionRef.moveto(Texit)
endif
endevent