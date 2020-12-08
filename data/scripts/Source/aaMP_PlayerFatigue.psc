ScriptName aaMP_PlayerFatigue extends Actor  

; ======== Properties ========

float property _fatigueAccumulationTimeUnit = 5.0 auto
float property _fatiguePerTimeUnit = 6.0 auto

float property _fatigueValueAfterDeath = 30.0 auto

float property _fatigueLowerLimit = 0.0 auto
float property _fatigueHigherLimit = 100.0 auto
GlobalVariable property _fatigueValue auto

;======== Variables ========

bool isFatigueAccumulationAllow = true
float fatigueRemoveMultiplier = 1.0

; ======== Events ========

event onInit()
	registerForSingleUpdate(_fatigueAccumulationTimeUnit)
endevent

event onUpdate()
	handlePlayerFatigueUpdateEvent()
	registerForSingleUpdate(_fatigueAccumulationTimeUnit)
endevent

event onDeath(Actor akKiller)
	handleOnDeathEvent()
endevent

; ======== Functions ========


function showFatigueStatus()
	Debug.notification("Усталость: " + Math.floor(_fatigueValue.getValue()))
endfunction

function handlePlayerFatigueUpdateEvent()
	if (isEnabled())
		if (isFatigueAccumulationAllow)
			modFatigue(_fatiguePerTimeUnit)
		endIf	
	endIf
endfunction

function handleOnDeathEvent()
	_fatigueValue.setValue(_fatigueValueAfterDeath)
endfunction

function modFatigueWithMultiplier(float value)
	float newFatingueValue = _fatigueValue.getValue() + (value * fatigueRemoveMultiplier)
	_fatigueValue.setValue(max(newFatingueValue, _fatigueLowerLimit))
endfunction

function modFatigue(float value)
	float newFatingueValue = _fatigueValue.getValue() + value
	_fatigueValue.setValue(max(min(newFatingueValue, _fatigueHigherLimit), _fatigueLowerLimit))
endfunction

function resetFatigueRemoveMultiplier()
	fatigueRemoveMultiplier = 1.0
endfunction

float function modFatigueRemoveMultiplier(float value)
	fatigueRemoveMultiplier+= value
endfunction

float function min(float a, float b)
	if (a > b)
		return b
	endif 
	return a
endfunction

float function max(float a, float b)
	if (a > b)
		return a
	endif 
	return b
endfunction

; ======== Getters ========

float function getFatigue()
	return _fatigueValue.getValue()
endfunction

float function getFatigueLowerLimit()
	return _fatigueLowerLimit
endfunction

float function getFatigueHigherLimit()
	return _fatigueHigherLimit
endfunction

bool function isFatigueAccumulationAllow()
	return isFatigueAccumulationAllow
endfunction

; ======== Setters ========

function setFatigueAccumulationAllow(bool value)
	isFatigueAccumulationAllow = value
endfunction
