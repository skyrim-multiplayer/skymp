ScriptName aaMP_PlayerSatiety extends Actor  

; ======== Properties ========

float property _satietyUpdateTimeUnit = 5.0 auto
float property _satietyPointRemovePerTimeUnit = 6.0 auto

float property _satietyValueAfterDeath = 70.0 auto

float property _foodEffectivenessMultiplier = 1.0 auto
float property _multiplierBonusForHungry = 0.05 auto
float property _satietyConsideredHungry = 90.0 auto

float property _overeatStarveMultiplier = 5.0 auto
float property _overeatStarveMultiplierLimit = 100.0 auto
float property _satietyLowerLimit = 0.0 auto
float property _satietyHigherLimit = 105.0 auto
GlobalVariable property _satietyValue auto

; ======== Events ========

event onInit()
	registerForSingleUpdate(_satietyUpdateTimeUnit)
endevent


event onUpdate()
	if (isEnabled())
		removeSatietyPoint(_satietyPointRemovePerTimeUnit)
		if (isSatietyOnDeathLimit())
			handlePlayerSturveDeath()
		endIf
	endIf
	registerForSingleUpdate(_satietyUpdateTimeUnit)
endevent


; ======== Functions ========

function showSatietyStatus()
	Debug.notification("Сытость: " + Math.floor(_satietyValue.getValue()))
endfunction


function removeSatietyPoint(float value)
	float newSatietyValue = _satietyValue.getValue() - value
	if (newSatietyValue < _satietyLowerLimit)
		_satietyValue.setValue(_satietyLowerLimit)
	else
		if (isOvereat())
			_satietyValue.setValue(removeSatietyAfterOvereat(value))
		else
			_satietyValue.setValue(newSatietyValue)
		endif
	endif
	showSatietyStatus()
endfunction


bool function isOvereat()
	return _satietyValue.getValue() > _overeatStarveMultiplierLimit
endfunction 


float function removeSatietyAfterOvereat(float value)
	float newSatietyValue = _satietyValue.getValue() - (value * _overeatStarveMultiplier)
	return max(newSatietyValue, _overeatStarveMultiplierLimit)
endfunction


bool function isSatietyOnDeathLimit()
	return _satietyValue.getValue() == _satietyLowerLimit
endfunction


function handlePlayerSturveDeath()
	kill()
	_satietyValue.setValue(_satietyValueAfterDeath)
endfunction


function addSatietyPoint(float value)
	float newSatietyValue = _satietyValue.getValue() + (value * calculateSatietyMultiplier())
	_satietyValue.setValue(min(newSatietyValue, _satietyHigherLimit))
	showSatietyStatus()
endfunction


float function calculateSatietyMultiplier()
	if (_satietyValue.getValue() >= _satietyConsideredHungry)
		return _foodEffectivenessMultiplier
	else
		return _foodEffectivenessMultiplier+ ((_satietyConsideredHungry - _satietyValue.getValue()) * _multiplierBonusForHungry)
	endif
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