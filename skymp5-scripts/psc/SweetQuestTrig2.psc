Scriptname SweetQuestTrig2 extends ObjectReference  
message property mess0 auto
Event OnTriggerEnter(ObjectReference akActionRef)
if game.getplayer().getitemcount(game.getform(0x0844D565)) < 1
game.getplayer().additem(game.getform(0x0844D565),1, True)
int Ibutton = mess0.show() 
endif
endevent
