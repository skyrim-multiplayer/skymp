Scriptname MP_AddWaterFromWell extends objectreference

Miscobject property Bucket auto
Miscobject property WaterBucket auto

Event OnActivate(ObjectReference akActionRef)
	if akActionRef.getitemCount(Bucket) != 0
		akActionRef.additem(WaterBucket, akActionRef.getitemCount(Bucket), false)
		akActionRef.removeitem(Bucket, akActionRef.getitemCount(Bucket), false, none)
		Debug.Notification("Ведра наполены")
	endif
endevent
