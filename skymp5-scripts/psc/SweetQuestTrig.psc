Scriptname SweetQuestTrig extends ObjectReference  
message property mess1 auto
message property mess2 auto
Event OnTriggerEnter(ObjectReference akActionRef)
if game.getplayer().getitemcount(game.getform(0x0844D564)) < 1
int Ibutton = mess1.show() 
if (Ibutton == 0)
game.getplayer().additem(game.getform(0x0844D564), 1, True)
gotostate("m2")
endif
endif
endevent

State m2
event onbeginstate()
int Ibutton = mess2.show() 
gotostate("")
endevent
endstate