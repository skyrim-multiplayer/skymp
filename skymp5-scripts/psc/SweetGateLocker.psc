Scriptname SweetGateLocker extends ObjectReference  

Message property DoorLockingMenu auto

int property DoorMainI auto

int Button
ObjectReference DoorMain

Event Oninit()

	DoorMain = game.getform(DoorMainI) as ObjectReference

endevent

Event OnActivate(ObjectReference akActionRef)

	Button = DoorLockingMenu.show()

		if (Button == 0)
			debug.notification("Gate is blocked")
			DoorMain.blockactivation()

		elseif (Button == 1)
			debug.notification("Gate is unlocked")
			DoorMain.blockactivation(false)

		endif
endevent
