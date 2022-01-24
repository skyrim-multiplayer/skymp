Scriptname sweetHealthActivator extends ObjectReference  
EffectShader Property eff  Auto  
Event OnActivate(ObjectReference akActionRef)
if !IsActivationBlocked()
Game.GetPlayer().RestoreActorValue("health", 100.0)
eff.Play(game.getplayer())
blockactivation()
utility.wait(2)
eff.Stop(game.getplayer())
utility.wait(15)
blockactivation(False)
endif
endevent
