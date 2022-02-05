Scriptname SweetDamage extends ObjectReference
Actor[] refList 
Actor[] refListToDelete
EffectShader Property eff  Auto 
Float Property Damage = 4.0 Auto
Bool Property Enabled = false Auto


Event OnInit()
	refList = new Actor[128]
	refListToDelete = new Actor[128]
	debug.SPLog(none, "SweetDamage Initialized with force " + Damage)
	RegisterForSingleUpdate(0.2)
EndEvent

Event OnTriggerEnter(ObjectReference akTriggerRef)
debug.SPLog(akTriggerRef, "Entered Trigger; Size "+ refList.Length)
	Actor AK = akTriggerRef as Actor
	if (AK == Game.GetPlayer() && refList.Find(AK) < 0)
		Int nIndex = refList.Find(none)
		refList[nIndex]=AK
		eff.Play(AK)
		if(Enabled)
			debug.SPLog(akTriggerRef, "Entered Trigger; Size "+ refList.Length)
		endif
	endif
EndEvent

Event OnUpdate()
	Int nIndex = refList.Length
	while nIndex 
		nIndex -= 1
		Actor AK = refList[nIndex]
		if(AK != none)
			AK.DamageActorValue("health", Damage)
			if(Enabled)
				debug.SPLog(AK, "Still Trigger")
			endif
			if (AK.IsDead())
				Int i = refListToDelete.Find(none)
				refListToDelete[i]=AK
				if(Enabled)
					debug.SPLog(AK, "Died after Trigger")
				endif
			endif
		endif
	endwhile
	nIndex = refListToDelete.Length
	while nIndex
		nIndex -= 1
		Actor AK = refListToDelete[nIndex]
		if(AK != none)
			eff.Stop(AK)
			Int i = refList.Find(AK)
			refListToDelete[nIndex] = none;
			refList[i] = none
		endif
	endwhile
	RegisterForSingleUpdate(0.2)
EndEvent
 
Event OnTriggerLeave(ObjectReference akTriggerRef)
	Actor AK = akTriggerRef as Actor
	if (AK == Game.GetPlayer() && refList.Find(AK) >= 0)
		Int nIndex = refListToDelete.Find(none)
		refListToDelete[nIndex]=AK
		debug.SPLog(akTriggerRef, "Leaving Trigger; Size " + refListToDelete.Length)
	endif
EndEvent

