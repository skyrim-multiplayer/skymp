Scriptname SweetWindTrig extends ObjectReference  
Int InTrigger = 0
int count = 0
EffectShader Property eff  Auto 

Event OnTriggerEnter(ObjectReference akTriggerRef)  
if akTriggerRef == Game.GetPlayer()
 InTrigger += 1
endif
while InTrigger == 1&&count == 1
Game.GetPlayer().additem(game.getform(0x063B8DF5))
Game.GetPlayer().SetActorValue("SpeedMult", 50.0)
Game.GetPlayer().additem(game.getform(0x063B8DF5))
eff.Play(game.getplayer())
utility.wait(1)
eff.Stop(game.getplayer())
endwhile
endevent

Event OnTriggerLeave(ObjectReference akTriggerRef)
	if (InTrigger > 0)
		if akTriggerRef == Game.GetPlayer()
 InTrigger -= 1
Game.GetPlayer().removeitem(game.getform(0x063B8DF5))
Game.GetPlayer().removeitem(game.getform(0x063B8DF5))
Game.GetPlayer().SetActorValue("SpeedMult", 100.0)
		endif
	endif
EndEvent


Event onactivatie(ObjectReference akActionRef)
count = count + 1
if count == 2
count = 0
endif
endevent