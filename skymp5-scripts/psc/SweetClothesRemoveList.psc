Scriptname SweetClothesRemoveList extends ObjectReference  
FormList property armorform auto 
int trash
int property Clothes auto

Event OnActivate(ObjectReference akActionRef)

trash = game.getplayer().getitemcount(armorform)
game.getplayer().removeitem(armorform, trash, true)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleWarmArms")
utility.wait(1)
Debug.SendAnimationEvent(Game.GetPlayer(),"IdleStop")
Game.GetPlayer().AddItem(Game.GetForm(Clothes), 1)
Game.GetPlayer().EquipItem(Game.GetForm(Clothes))

endevent
