Scriptname SweetDamahe extends ObjectReference  
Int InTrigger = 0
EffectShader Property eff  Auto 

Event OnTriggerEnter(ObjectReference akTriggerRef)  
if akTriggerRef == Game.GetPlayer()
 InTrigger += 1
endif
while InTrigger == 1
Game.GetPlayer().DamageActorValue("health", 7.0)
eff.Play(game.getplayer())
utility.wait(1)
eff.Stop(game.getplayer())
endwhile
endevent

Event OnTriggerLeave(ObjectReference akTriggerRef)
	if (InTrigger > 0)
		if akTriggerRef == Game.GetPlayer()
 InTrigger -= 1
		endif
	endif
EndEvent