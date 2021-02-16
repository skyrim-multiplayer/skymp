ScriptName aaMP_RestArea extends ObjectReference  

float property _fatigueRemovePerTimeUnit = 0.27 auto
float property _fatigueRemoveTimeUnit = 1.0 auto
float property _restPerPlayerMultiplier = 0.1 auto
float property _maxRestPerPlayerMultiplier = 0.3 auto
Form[] property _actorsInArea auto

float currentRestPerPlayerMultiplier = 0.0

; ======== Events ========

event onUpdate()
	removeFatigueInRestArea()
endevent

event onTriggerEnter(ObjectReference akTriggerRef)
	playerEnterRestZone(akTriggerRef)
endevent

event OnTriggerLeave(ObjectReference akTriggerRef)
	playerLeaveRestZone(akTriggerRef)
endevent

;  ======== Functions ========

function playerEnterRestZone(ObjectReference akTriggerRef)
	if (akTriggerRef as aaMP_PlayerFatigue)
		_actorsInArea = aaMP_ArrayUtils.add(_actorsInArea, akTriggerRef)
		(akTriggerRef as aaMP_PlayerFatigue).setFatigueAccumulationAllow(false)
	endif
	registerForSingleUpdate(_fatigueRemovePerTimeUnit)
endfunction

function playerLeaveRestZone(ObjectReference akTriggerRef)
	if (akTriggerRef as aaMP_PlayerFatigue)
		_actorsInArea = aaMP_ArrayUtils.remove(_actorsInArea, akTriggerRef)
		(akTriggerRef as aaMP_PlayerFatigue).setFatigueAccumulationAllow(true)
		(akTriggerRef as aaMP_PlayerFatigue).resetFatigueRemoveMultiplier()
	endif
	if (_actorsInArea.length <= 0)
		unregisterForUpdate()
	endif
endfunction

function updateCurrentRestPerPlayerMultiplier()
	if (currentRestPerPlayerMultiplier < _maxRestPerPlayerMultiplier)
		currentRestPerPlayerMultiplier = (_actorsInArea.length - 1) * _restPerPlayerMultiplier
	endif
endfunction

function removeFatigueInRestArea()
	updateCurrentRestPerPlayerMultiplier()
	int i = 0
	while (i < _actorsInArea.length)
		modPlayerFatigueWithCurrentMultiplier(_actorsInArea[i] as aaMP_PlayerFatigue)
		i += 1
	endwhile
	registerForSingleUpdate(_fatigueRemoveTimeUnit)
endfunction

function modPlayerFatigueWithCurrentMultiplier(aaMP_PlayerFatigue player)
	player.modFatigueRemoveMultiplier(currentRestPerPlayerMultiplier)
	player.modFatigueWithMultiplier(-_fatigueRemovePerTimeUnit)
	player.modFatigueRemoveMultiplier(- currentRestPerPlayerMultiplier)
endfunction