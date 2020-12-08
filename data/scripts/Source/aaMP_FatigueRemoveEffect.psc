Scriptname aaMP_FatigueRemoveEffect extends ActiveMagicEffect

; ======== Event ========

;/
	! Необходимо, чтобы у эффектов была указана длительность не равная 0, иначе функция GetMagnitude() не сработает.
/;
event onEffectStart(Actor akTarget, Actor akCaster)
	(akCaster as aaMP_PlayerFatigue).modFatigue(-getMagnitude())
endEvent