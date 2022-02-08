Scriptname SweetDamage extends ObjectReference
Actor[] refList 
Actor[] refListToDelete
EffectShader Property eff  Auto 
Float Property Damage = 10.0 Auto
Bool Property Enabled = False Auto


Event OnInit()
	refList = new Actor[32]
	refListToDelete = new Actor[32]
	if(Enabled)
		debug.SPLog(none, "SweetDamage Initialized with force " + Damage)
	endif
	RegisterForSingleUpdate(0.5)
EndEvent

Event OnTriggerEnter(ObjectReference akTriggerRef)
	Actor AK = akTriggerRef as Actor
	if(AK == Game.GetPlayer() && refList.Find(AK) < 0)
		Int nIndex = refList.Find(none)
		refList[nIndex]=AK
		eff.Play(AK)
		if(Enabled)
			debug.SPLog(AK, "Entered Trigger; Position " + nIndex)
		endif
	endif
EndEvent

Event OnUpdate()
	Int nIndex = 0
	while nIndex < refList.Length
		Actor AK = refList[nIndex]
		if(AK != none)
			AK.DamageActorValue("health", Damage)
			if(Enabled)
				debug.SPLog(AK, "Still Trigger")
			endif
			if(AK.IsDead())
				Int i = refListToDelete.Find(none)
				refListToDelete[i]=AK
				if(Enabled)
					debug.SPLog(AK, "Died after Trigger")
				endif
			endif
		endif
		nIndex += 1
	endwhile
	nIndex = 0
	while nIndex < refListToDelete.Length
		Actor AK = refListToDelete[nIndex]
		if(AK != none)
			eff.Stop(AK)
			Int i = refList.Find(AK)
			refListToDelete[nIndex] = none;
			if(i>=0)
				refList[i] = none
			endif
		endif
		nIndex += 1
	endwhile
	RegisterForSingleUpdate(0.5)
EndEvent
 
Event OnTriggerLeave(ObjectReference akTriggerRef)
	Actor AK = akTriggerRef as Actor
	if(AK == Game.GetPlayer() && refList.Find(AK) >= 0 && refListToDelete.Find(AK) < 0)
		Int nIndex = refListToDelete.Find(none)
		refListToDelete[nIndex]=AK
		if(Enabled)
			debug.SPLog(AK, "Leaving Trigger; Position in removal " + nIndex)
		endif
	endif
EndEvent
