Scriptname SweetWindTrig extends ObjectReference 
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
Int InTrigger = 0
int count1 = 0
int activ = 0
int count = 0
int count2 = 0
EffectShader Property eff  Auto 
ObjectReference property cr auto
 

Event OnTriggerEnter(ObjectReference akTriggerRef)  
if akTriggerRef == Game.GetPlayer()
 InTrigger += 1
gotostate("spawn")
endif
endevent


state spawn
event onbeginstate()
if  InTrigger == 1
count2 = 0
count1 = 0
count = 0

utility.wait(5)
 debug.notification("it's getting cold outside")
utility.wait(10)
 debug.notification("it's getting frieezing cold")
utility.wait(5)
endif

if activ < 1 

objectreference w1 = xm1.placeatme(game.getform(0x00101CF3))
w1.GetHeadingAngle(xm1)

objectreference w2 = xm2.placeatme(game.getform(0x00101CF3))
w2.GetHeadingAngle(xm2)


objectreference w3 = xm3.placeatme(game.getform(0x00101CF3))
w3.GetHeadingAngle(xm3)


objectreference w4 = xm4.placeatme(game.getform(0x00101CF3))
w4.GetHeadingAngle(xm4)


objectreference w5 = xm5.placeatme(game.getform(0x00101CF3))
w5.GetHeadingAngle(xm5)

objectreference w6 = xm6.placeatme(game.getform(0x00101CF3))
w6.GetHeadingAngle(xm6)


objectreference w7 = xm7.placeatme(game.getform(0x00101CF3))
w7.GetHeadingAngle(xm7)


objectreference w8 = xm8.placeatme(game.getform(0x00101CF3))
w8.GetHeadingAngle(xm8)


objectreference w9 = xm9.placeatme(game.getform(0x00101CF3))
w9.GetHeadingAngle(xm9)

objectreference w10 = xm10.placeatme(game.getform(0x00101CF3))
w10.GetHeadingAngle(xm10)


objectreference w11 = xm11.placeatme(game.getform(0x00101CF3))
w11.GetHeadingAngle(xm11)


objectreference w12 = xm12.placeatme(game.getform(0x00101CF3))
w12.GetHeadingAngle(xm12)


objectreference w13 = xm13.placeatme(game.getform(0x00101CF3))
w13.GetHeadingAngle(xm13)


objectreference w14 = xm14.placeatme(game.getform(0x00101CF3))
w14.GetHeadingAngle(xm14)


objectreference w15 = xm15.placeatme(game.getform(0x00101CF3))
w15.GetHeadingAngle(xm15)


objectreference w16 = xm16.placeatme(game.getform(0x00101CF3))
w16.GetHeadingAngle(xm16)


objectreference w17 = xm17.placeatme(game.getform(0x00101CF3))
w17.GetHeadingAngle(xm17)


objectreference w18 = xm18.placeatme(game.getform(0x00101CF3))
w18.GetHeadingAngle(xm18)


objectreference w19 = xm19.placeatme(game.getform(0x00101CF3))
w19.GetHeadingAngle(xm19)


objectreference w20 = xm20.placeatme(game.getform(0x00101CF3))
w20.GetHeadingAngle(xm20)


objectreference w21 = xm21.placeatme(game.getform(0x00101CF3))
w21.GetHeadingAngle(xm21)

activ = activ + 1 
count1 = count1 + 100
gotostate("froz")
elseif activ >= 1
gotostate("spawn2")
endif
endevent
endstate

state spawn2
event onbeginstate()
count1 = count1 + 100
count2 = 0
if count2 < 200
while count2 < 200
objectreference r
r = Game.FindRandomReferenceOfAnyTypeInListFromRef(listwind, cr, 10000.0)
r.enable()
count2 = count2 + 1
endwhile
endif
if count2 >=200
gotostate("froz")
endif
endevent
endstate



state froz
event onbeginstate()
if InTrigger == 1
Game.GetPlayer().SetActorValue("SpeedMult", 50.0)
Game.GetPlayer().additem(game.getform(0x063B8DF5), 1, false)
eff.Play(game.getplayer())
utility.wait(40)
eff.Stop(game.getplayer())
Game.GetPlayer().removeitem(game.getform(0x063B8DF5), 200, false)
Game.GetPlayer().SetActorValue("SpeedMult", 100.0)
gotostate("dis")
endif
endevent
endstate

state dis
event onbeginstate()
while count < 200
objectreference r 
r = Game.FindRandomReferenceOfAnyTypeInListFromRef(listwind, cr, 10000.0)
r.disable()
ObjectReference refr
  refr = Game.FindClosestReferenceOfAnyTypeInListFromRef(list, cr, 10000.0)
refr.disable()
count = count + 1
endwhile
if count >= 200
gotostate("activate12")
endif
EndEvent
endstate


State activate12
event onbeginstate()
ObjectReference refr
  refr = Game.FindClosestReferenceOfAnyTypeInListFromRef(list, cr, 1000.0)
refr.enable()
count2 = 0
gotostate("spawn")
endevent
endstate


Event OnTriggerLeave(ObjectReference akTriggerRef)  
if akTriggerRef == Game.GetPlayer()
 InTrigger = 0
game.getplayer().removeitem(game.getform(0x063B8DF5), 10, false)
endif
endevent

