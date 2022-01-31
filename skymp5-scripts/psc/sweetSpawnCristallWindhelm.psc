Scriptname sweetSpawnCristallWindhelm extends ObjectReference  
objectreference property xm1 auto
objectreference property xm2 auto
objectreference property xm3 auto
objectreference property xm4 auto
objectreference property xm5 auto
objectreference property xm6 auto
objectreference property xm7 auto
objectreference property xm8 auto
objectreference property xm9 auto
objectreference property xm10 auto
objectreference property xm11 auto
objectreference property xm12 auto
objectreference property xm13 auto
objectreference property xm14 auto
objectreference property xm15 auto
objectreference property xm16 auto
objectreference property xm17 auto
objectreference property xm18 auto
objectreference property xm19 auto
objectreference property xm20 auto
objectreference property xm21 auto
formlist property list auto
formlist property listwind auto
formlist property xmark auto
objectreference property trig1 auto
objectreference property trig2 auto
objectreference property trig3 auto
objectreference property trig4 auto
objectreference property trig5 auto
objectreference property trig6 auto
objectreference property trig7 auto
objectreference property trig8 auto
objectreference property trig9 auto
objectreference property trig10 auto
objectreference property trig11 auto
objectreference property trig12 auto
objectreference property trig13 auto
objectreference property trig14 auto
objectreference property trig15 auto
objectreference property trig16 auto
objectreference property trig17 auto
objectreference property trig18 auto
objectreference property trig19 auto
objectreference property trig20 auto
objectreference property trig21 auto
objectreference property trig22 auto
objectreference property trig23 auto

int activ
int count
int count1
int count2




event oninit()
 ObjectReference refr
  refr = Game.FindClosestReferenceOfAnyTypeInListFromRef(list, self, 1000)
 if Game.FindClosestReferenceOfAnyTypeInListFromRef(list, self, 1000) == true
debug.notification("2")
self.placeatme(game.getform(0x4027C32))
gotostate("activate1")
elseif Game.FindClosestReferenceOfAnyTypeInListFromRef(list, self, 1000) == false
debug.notification("0")
gotostate("activate1")
endif
endevent


State activate1
Event OnActivate(ObjectReference akActionRef)
debug.notification(1)


objectreference w1 = xm1.placeatme(game.getform(0x00101CF3))
w1.GetHeadingAngle(xm1)
trig1.blockactivation()

objectreference w2 = xm2.placeatme(game.getform(0x00101CF3))
w2.GetHeadingAngle(xm2)
trig2.blockactivation()


objectreference w3 = xm3.placeatme(game.getform(0x00101CF3))
w3.GetHeadingAngle(xm3)
trig3.blockactivation()

objectreference w4 = xm4.placeatme(game.getform(0x00101CF3))
w4.GetHeadingAngle(xm4)
trig4.blockactivation()

objectreference w5 = xm5.placeatme(game.getform(0x00101CF3))
w5.GetHeadingAngle(xm5)
trig5.blockactivation()

objectreference w6 = xm6.placeatme(game.getform(0x00101CF3))
w6.GetHeadingAngle(xm6)
trig6.blockactivation()

objectreference w7 = xm7.placeatme(game.getform(0x00101CF3))
w7.GetHeadingAngle(xm7)
trig7.blockactivation()

objectreference w8 = xm8.placeatme(game.getform(0x00101CF3))
w8.GetHeadingAngle(xm8)
trig8.activate(self)

objectreference w9 = xm9.placeatme(game.getform(0x00101CF3))
w9.GetHeadingAngle(xm9)
trig9.activate(self)

objectreference w10 = xm10.placeatme(game.getform(0x00101CF3))
w10.GetHeadingAngle(xm10)
trig10.activate(self)

objectreference w11 = xm11.placeatme(game.getform(0x00101CF3))
w11.GetHeadingAngle(xm11)
trig11.activate(self)

objectreference w12 = xm12.placeatme(game.getform(0x00101CF3))
w12.GetHeadingAngle(xm12)
trig12.activate(self)

objectreference w13 = xm13.placeatme(game.getform(0x00101CF3))
w13.GetHeadingAngle(xm13)
trig13.activate(self)

objectreference w14 = xm14.placeatme(game.getform(0x00101CF3))
w14.GetHeadingAngle(xm14)
trig14.activate(self)

objectreference w15 = xm15.placeatme(game.getform(0x00101CF3))
w15.GetHeadingAngle(xm15)
trig15.activate(self)

objectreference w16 = xm16.placeatme(game.getform(0x00101CF3))
w16.GetHeadingAngle(xm16)
trig16.activate(self)

objectreference w17 = xm17.placeatme(game.getform(0x00101CF3))
w17.GetHeadingAngle(xm17)
trig17.activate(self)

objectreference w18 = xm18.placeatme(game.getform(0x00101CF3))
w18.GetHeadingAngle(xm18)
trig18.activate(self)

objectreference w19 = xm19.placeatme(game.getform(0x00101CF3))
w19.GetHeadingAngle(xm19)
trig19.activate(self)

objectreference w20 = xm20.placeatme(game.getform(0x00101CF3))
w20.GetHeadingAngle(xm20)
trig20.activate(self)

objectreference w21 = xm21.placeatme(game.getform(0x00101CF3))
w21.GetHeadingAngle(xm21)
trig21.activate(self)
trig22.activate(self)
trig23.activate(self)

utility.wait(15)
gotostate("activate11")
endevent
endstate

function wa()
  GotoState("w")
endfunction



State activate11
event onbeginstate()
debug.notification(2)
objectreference r 
ObjectReference refr
  refr = Game.FindClosestReferenceOfAnyTypeInListFromRef(list, self, 1000)
refr.disable()
while count < 200
r = Game.FindRandomReferenceOfAnyTypeInListFromRef(listwind, self, 10000.0)
r.disable()
count = count + 1
endwhile
if count >= 200

trig1.activate(self)

trig2.activate(self)

trig3.activate(self)

trig4.activate(self)

trig5.activate(self)

trig6.activate(self)

trig7.activate(self)

trig8.activate(self)

trig9.activate(self)

trig10.activate(self)

trig11.activate(self)

trig12.activate(self)

trig13.activate(self)

trig14.activate(self)

trig15.activate(self)

trig16.activate(self)

trig17.activate(self)

trig18.activate(self)

trig19.activate(self)

trig20.activate(self)

trig21.activate(self)
trig22.activate(self)
trig23.activate(self)


gotostate("activate12")
endif
endevent
endstate

State activate12
event onbeginstate()
debug.notification(3)
objectreference r 
ObjectReference refr
  refr = Game.FindClosestReferenceOfAnyTypeInListFromRef(list, self, 1000)
while count2 < 200
r = Game.FindRandomReferenceOfAnyTypeInListFromRef(listwind, self, 10000.0)
r.enable()
refr.enable()
count2 = count2 + 1
endwhile
if count2 >= 200
gotostate("activate")
endif
endevent
endstate


State activate
Event OnActivate(ObjectReference akActionRef)
debug.notification(4)
utility.wait(60)
count = 0
count2 = 0
gotostate("activate11")
endevent
endstate


