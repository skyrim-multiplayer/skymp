Scriptname SweetDamage extends ObjectReference
Actor[] refList 
Actor[] refListToDelete
EffectShader Property eff  Auto 
Float Property Damage = 10.0 Auto
Bool Property Enabled = False Auto

Function MakeDamage(Actor AK)
	if(!AK.IsDead())
		eff.Play(AK)
		AK.DamageActorValue("health", Damage)
		if(Enabled)
			SweetPie.SPLog(self, AK, "Damaged")
		endif
	endif
	if(AK.IsDead())
		Int i = refListToDelete.Find(none)
		refListToDelete[i]=AK
		if(Enabled)
			SweetPie.SPLog(self, AK, "Died")
		endif
	endif
EndFunction

Event OnInit()
	refList = new Actor[32]
	refListToDelete = new Actor[32]
	if(Enabled)
		SweetPie.SPLog(self, none, "SweetDamage Initialized with force " + Damage)
	endif
	RegisterForSingleUpdate(0.5)
EndEvent

Event OnTriggerEnter(ObjectReference akTriggerRef)
	Actor AK = akTriggerRef as Actor
	if(AK == Game.GetPlayer() && !AK.IsDead() && refList.Find(AK) < 0)
		if(Enabled)
			SweetPie.SPLog(self, AK, "Entered Trigger; Position " + nIndex)
		endif
		Int nIndex = refList.Find(none)
		refList[nIndex]=AK
		MakeDamage(AK)
	endif
	if(Enabled)
		SweetPie.SPDumpActorArray(self, "Entered", refList)
		SweetPie.SPDumpActorArray(self, "Left", refListToDelete )
	endif
EndEvent

Event OnUpdate()
	Int nIndex = 0
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
	nIndex = 0
	while nIndex < refList.Length
		Actor AK = refList[nIndex]
		if(AK != none)
			 MakeDamage(AK)
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
			SweetPie.SPLog(self, AK, "Leaving Trigger; Position in removal " + nIndex)
		endif
	endif
	if(Enabled)
		SweetPie.SPDumpActorArray(self, "Entered", refList)
		SweetPie.SPDumpActorArray(self, "Left", refListToDelete )
	endif
EndEvent
