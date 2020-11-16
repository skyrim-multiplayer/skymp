Scriptname MP_StartWorkFarmer extends ObjectReference

miscobject Property ItemAllowsToPlant Auto
miscobject Property ItemAllowsEnter Auto
miscobject Property Gold001 Auto
message Property MessageStartWork Auto
message Property MessageStopWork Auto
formlist Property Products Auto
int[] Property ProductPrices Auto
ObjectReference Property ContainerWithProducts Auto
miscobject Property EmptyBucket Auto
miscobject Property WaterBucket Auto
miscobject Property Fertilizer Auto
miscobject Property Motyga Auto
potion[] Property Seeds Auto
int[] Property SeedsCount Auto

int FertilizeGet = 7
int BucketToGet = 1

Function AddSeeds(ObjectReference ActorSeeds)
	int i = 0
	While(i < seeds.Length)
		ActorSeeds.AddItem(Seeds[i], SeedsCount[i], true)
		i += 1
	EndWhile
EndFunction

Function RemoveSeeds(ObjectReference ActorSeeds)
	int i = 0
	While(i < seeds.Length)
		ActorSeeds.RemoveItem(Seeds[i], ActorSeeds.GetItemCount(Seeds[i]), true, none)
		i += 1
	EndWhile
EndFunction

Event OnInit()
	BlockActivation()
EndEvent

Event OnActivate(ObjectReference akActionRef)
	if akActionRef.GetItemCount(ItemAllowsEnter) == 0
		int Button = MessageStartWork.Show()
		if Button == 0
			(akActionRef).AddItem(EmptyBucket, BucketToGet, false)
			(akActionRef).AddItem(Fertilizer, FertilizeGet, false)
			(akActionRef).AddItem(Motyga, 1, false)
			(akActionRef).AddItem(ItemAllowsEnter, 1, true)	
			;Debug.Notification("Добавлеено: мотыга, бадья, семяна, удобрение")
			AddSeeds(akActionRef)
		endif
	else
		int Button = MessageStopWork.Show()
		if Button == 0
			if (akActionRef.GetItemCount(Motyga) >= 1) && ((akActionRef.GetItemCount(WaterBucket) + akActionRef.GetItemCount(EmptyBucket)) >= BucketToGet)
				PayForWork(akActionRef)
				RemoveSeeds(akActionRef)
				akActionRef.RemoveItem(Fertilizer, akActionRef.GetItemCount(Fertilizer), false, None)
				akActionRef.RemoveItem(Motyga, 1, false, None)
				if (akActionRef.GetItemCount(WaterBucket))
					akActionRef.RemoveItem(WaterBucket, 1, false, none)
				else
					akActionRef.RemoveItem(EmptyBucket, 1, false, none)
				endif
				(akActionRef).RemoveItem(ItemAllowsEnter, (akActionRef).GetItemCount(ItemAllowsEnter), True, none)
				;Debug.Notification("Инструменты удалены")
			else
				;Debug.Notification("Верни, что взял!!!")
			endif
		endif
	endif
EndEvent

Function PayForWork(objectReference ActorPay)
	int i = 0
	While(i<Products.GetSize())
		ActorPay.AddItem(Gold001, (ActorPay.GetItemCount(Products.GetAt(i)) * ProductPrices[i]), false)
		;Debug.Notification("Получено золото - " + (ActorPay.GetItemCount(Products.GetAt(i)) * ProductPrices[i]) + ".(" + (Products.GetAt(i)).GetName() + ")")
		ActorPay.RemoveItem(Products.GetAt(i), ActorPay.GetItemCount(Products.GetAt(i)), false, ContainerWithProducts)
		i += 1
	EndWhile
EndFunction
