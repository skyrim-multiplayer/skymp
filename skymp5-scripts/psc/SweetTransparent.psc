Scriptname SweetTransparent extends ObjectReference  
objectreference property Most1 auto
objectreference property Most2 auto
objectreference property Most3 auto
objectreference property Most4 auto
objectreference property Most1N auto
objectreference property Most2N auto
objectreference property Most3N auto
objectreference property Most4N auto

event OnActivate(ObjectReference akActionRef)
if !IsActivationBlocked()
game.getplayer().SetAlpha(0,9)

Most1.disable()

Most2.disable()

Most3.disable()

Most4.disable()


Most1N.enable()

Most2N.enable()

Most3N.enable()

Most4N.enable()

blockactivation()
self.SetPosition(173397.7656, -97021.6875, 10850.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10800.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10750.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10700.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10650.0000)
utility.wait(30)
utility.wait(15)
self.SetPosition(173397.7656, -97021.6875, 10700.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10750.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10800.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10850.0000)
utility.wait(1)
self.SetPosition(173397.7656, -97021.6875, 10900.0000)

Most1N.disable()

Most2N.disable()

Most3N.disable()

Most4N.disable()




Most1.enable()

Most2.enable()

Most3.enable()

Most4.enable()

game.getplayer().SetAlpha(1,0)
blockactivation(false)
endif
endevent

