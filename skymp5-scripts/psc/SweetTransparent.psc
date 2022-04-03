Scriptname SweetTransparent extends ObjectReference  
Int[] property Bridges Auto
Float property DefaultZ Auto
Float property Timeout = 30.0 Auto
Bool property DebugEnabled = False Auto
ObjectReference[] BridgeRefs
Actor PlayerRef = none

Function ChangeZ(Float value)
	Int nIndex = 0
	while nIndex < Bridges.Length
		ObjectReference oRef = BridgeRefs[nIndex]
		if(oRef != none)
			Float nX = oRef.GetPositionX()
			Float nY = oRef.GetPositionY()
			if(DebugEnabled)
				Float nZ = oRef.GetPositionZ()
				SweetPie.SPLog(self, oRef, "Platform: X " + nX + "; Y " + nY + "; Z " + nZ)
				;debug.notification("Platform: X " + nX + "; Y " + nY + "; Z " + nZ)
			endif
			oRef.SetPosition(nX, nY, DefaultZ + value)
		endif
		nIndex += 1
	endwhile
EndFunction

Event OnInit()
	BridgeRefs = new ObjectReference[32]
	Int nIndex = 0
	while (nIndex < Bridges.Length && nIndex < BridgeRefs.Length)
		BridgeRefs[nIndex] = Game.GetForm(Bridges[nIndex]) as ObjectReference
		nIndex += 1
	endwhile
	if(DebugEnabled)
		Float nX = GetPositionX()
		Float nY = GetPositionY()
		Float nZ = GetPositionZ()
		SweetPie.SPLog(self, none, "Activator: X " + nX + "; Y " + nY + "; Z " + nZ)
		;debug.notification("Activator: X " + nX + "; Y " + nY + "; Z " + nZ)
	endif
	ChangeZ(0.0)
EndEvent

Event OnActivate(ObjectReference akActionRef)
	if(!IsActivationBlocked())
		PlayerRef = game.getplayer()
		BlockActivation()
		if(PlayerRef != none)
			PlayerRef.SetAlpha(0.2)
		endif
		ChangeZ(318.0)
		RegisterForSingleUpdate(Timeout)
	endif
EndEvent

Event OnUpdate()
	ChangeZ(0.0)
	blockactivation(false)
	if(PlayerRef != none)
		PlayerRef.SetAlpha(1.0)
		PlayerRef = none
	endif
EndEvent
