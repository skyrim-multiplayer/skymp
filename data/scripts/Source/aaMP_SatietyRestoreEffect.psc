Scriptname aaMP_SatietyRestoreEffect extends ActiveMagicEffect  

; ======== Event ========

;/
	! Необходимо, чтобы у эффектов была указана длительность не равная 0, иначе функция GetMagnitude() не сработает.
/;
event onEffectStart(Actor akTarget, Actor akCaster)
	(akCaster as aaMp_PlayerSatiety).addSatietyPoint(GetMagnitude())
	(akCaster as aaMp_PlayerSatiety).updateCurrentSatietyStage()
endEvent