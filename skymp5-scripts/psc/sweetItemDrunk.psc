Scriptname sweetItemDrunk extends ObjectReference 
miscobject property me auto 
Event onload()
debug.notification("1")
while (game.getplayer().getitemcount(me) >=5)
debug.notification("w")
Debug.SendAnimationEvent(Game.GetPlayer(),"idledrunkstart")
utility.wait(120)
game.getplayer().removeitem(me, 1)
endwhile
endevent
