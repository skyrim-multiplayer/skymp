Scriptname aaMP_SatietySystemSatietyRestore extends ActiveMagicEffect  

; ======== Event ========

;/
	Ивент срабатывает при поедании объекта с текущим магическим эффектом.
	В это время восполняется сытость игрока, актер убирает оружие, и воспроизводится анимация поедания я задержкой в секунду.
	
	! Необходимо, чтобы у эффектов была указана длительность не равная 0, иначе функция GetMagnitude() не сработает.
/;
event OnEffectStart(Actor akTarget, Actor akCaster)
	(akCaster as aaMp_SatietySystemPlayerController).addSatietyPoint(GetMagnitude())
	(akCaster as aaMp_SatietySystemPlayerController).updateCurrentSatietyStage()
endEvent