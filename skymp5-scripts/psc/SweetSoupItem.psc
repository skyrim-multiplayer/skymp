Scriptname SweetSoupItem extends ObjectReference  
Event oninit()
debug.notification("w")
utility.wait(35)
game.getplayer().removeitem(Game.GetForm(0x8277674), 1, true)
endevent
