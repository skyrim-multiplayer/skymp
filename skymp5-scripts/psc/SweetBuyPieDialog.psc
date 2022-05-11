Scriptname SweetBuyPieDialog extends ObjectReference  
int random
Potion property pie1 auto
Potion property pie2 auto
Potion property pie3 auto
Message property m1 auto
Message property m2 auto
Message property m3 auto
Event OnActivate(ObjectReference akActionRef)
Int iButton = m1.Show()
if (IButton == 0)&&game.getplayer().getitemcount(game.getform(0x0000000F)) >= 30
gotostate("havemoney")

elseif (IButton == 0)&&game.getplayer().getitemcount(game.getform(0x0000000F)) < 30
gotostate("notmoney")
endif
endevent

State havemoney
event onbeginstate()
random = Utility.RandomInt(1 ,4)
int ibutton = m2.show()
if (ibutton == 0 )
 Utility.RandomInt(1 ,4)
endif
if random == 1
game.getplayer().removeitem(game.getform(0x0000000F), 30)
game.getplayer().additem(game.getform(0x00064B43), 1)

elseif random == 2
game.getplayer().removeitem(game.getform(0x0000000F), 30)
game.getplayer().additem(pie1, 1)

elseif random == 3
game.getplayer().removeitem(game.getform(0x0000000F), 30)
game.getplayer().additem(pie2, 1)

elseif random == 4
game.getplayer().removeitem(game.getform(0x0000000F), 30)
game.getplayer().additem(pie3, 1)

endif
endevent
endstate

State notmoney
event onbeginstate()
Int Ibutton = m3.show()
endevent
endstate
