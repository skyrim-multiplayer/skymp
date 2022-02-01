Scriptname SweetWriteLetter extends ObjectReference  
message property LetterMess auto 
miscobject property rememberitem auto
Event OnActivate(ObjectReference akActionRef)
int Ibutton = LetterMess.show()
if (Ibutton == 0)&&game.getplayer().getitemcount(rememberItem) < 1
game.getplayer().additem(rememberitem, 1, false)
endif
endevent