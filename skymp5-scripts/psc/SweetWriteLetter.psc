Scriptname SweetWriteLetter extends ObjectReference  
message property LetterMess auto 
Event OnActivate(ObjectReference akActionRef)
Form rememberItem = game.getform(0x0840ad63)
int Ibutton = LetterMess.show()
if (Ibutton == 0)&&game.getplayer().getitemcount(rememberItem) < 1
game.getplayer().additem(rememberItem, 1, false)
endif
endevent