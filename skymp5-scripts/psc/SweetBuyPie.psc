Scriptname SweetBuyPie extends TopicInfo  
int random
Potion property pie auto
Event OnStoryHello(Location akLocation, ObjectReference akActor1, ObjectReference akActor2)
random = Utility.RandomInt(1 ,2)
 Utility.RandomInt(1 ,2)
if random == 1
game.getplayer().Additem(pie)
endif
endevent

