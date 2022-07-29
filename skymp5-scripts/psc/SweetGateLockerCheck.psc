Scriptname SweetGateLockerCheck extends ObjectReference  

int activation

Event OnInit()

activation = 0

endevent

Event OnActivate(ObjectReference akActionRef)

		if (Self.IsActivationBlocked() == true)&&(activation == 0)
			debug.notification("Gate is blocked")
			activation = 1
			
		elseif (Self.IsActivationBlocked() == true)&&(activation == 1)
			debug.notification("Gate is blocked")
			Debug.SendAnimationEvent(Game.GetPlayer(),"IdleMT_DoorBang")
			activation = 0

		elseif (Self.IsActivationBlocked() == false)
			activation = 0		

		endif
endevent
