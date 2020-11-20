ScriptName aaMP_PlayerSatiety extends Actor  

; ======== Properties ========

float property _satietyUpdateTimeUnit = 5.0 auto
float property _satietyPointRemovePerTimeUnit = 6.0 auto

float property _satietyValueAfterDeath = 70.0 auto

float property _foodEffectivenessMultiplier = 1.0 auto
float property _foodEffectivenessMultiplierPerValue = 0.05 auto
float property _foodEffectivenessMultiplierLimit = 90.0 auto

float property _starveMultiplier = 5.0 auto
float property _starveMultiplierLimit = 100.0 auto

float[] property _satietyStages auto
int[] property _stagesDescriptions auto
Faction property _aaMPf_SatietyFaction auto
Spell property _aaMPs_SatietyStatus auto

int property _statusCheckKeyCode = 49 auto

; ======== Variables ========

float satietyValue = 100.0
float lowerThresholdValueForCurrentStage
float higherThresholdValueForCurrentStage

; ======== Events ========

event onInit()
	addToFaction(_aaMPf_SatietyFaction)
	registerForSingleUpdate(_satietyUpdateTimeUnit)
	registerForKey(_statusCheckKeyCode)
	handleSatietyChanges()
endevent


event onUpdate()
	if (!isEnabled())
		return
	endIf
	
	removeSatietyPoint(_satietyPointRemovePerTimeUnit)
	if (isSatietyOnDeathLimit())
		handlePlayerSturveDeath()
	endIf
	registerForSingleUpdate(_satietyUpdateTimeUnit)
endevent


event onKeyDown(int keyCode)
	if (_statusCheckKeyCode == keyCode)
		showSatietyStatus()
	endif
endevent

; ======== Functions ========

function showSatietyStatus()
	Debug.notification("You satiety is " + Math.floor(satietyValue) + " points.")
endfunction


function removeSatietyPoint(float value)
	float tempSatietyValue = satietyValue - value
	float lowerAllowableSatietyValue = _satietyStages[1]
	if (isLess(tempSatietyValue, lowerAllowableSatietyValue ))
		satietyValue = lowerAllowableSatietyValue 
	else
		if (isOvereat())
			satietyValue = removeSatietyAfterOvereat(value)
		else
			satietyValue = tempSatietyValue
		endif
	endif
	handleSatietyChanges()
endfunction


bool function isOvereat()
	return isMore(satietyValue, _starveMultiplierLimit)
endfunction 


float function removeSatietyAfterOvereat(float value)
	float tempSatietyValue = satietyValue - (value * _starveMultiplier)
	if (isMore(tempSatietyValue, _starveMultiplierLimit))
		return tempSatietyValue
	else
		return _starveMultiplierLimit
	endif
endfunction


bool function isSatietyOnDeathLimit()
	return satietyValue == _satietyStages[1]
endfunction


function handlePlayerSturveDeath()
	kill()
	satietyValue = _satietyValueAfterDeath
endfunction


function addSatietyPoint(float value)
	float tempSatietyValue = satietyValue + (value * calculateSatietyMultiplier())
	float higherAllowableSatietyValue = _satietyStages[_satietyStages.length - 2]
	if (isMore(tempSatietyValue, higherAllowableSatietyValue))
		satietyValue = higherAllowableSatietyValue
	else 
		satietyValue = tempSatietyValue
	endIf
	handleSatietyChanges()
	showSatietyStatus()
endfunction


float function getActualSatietyMultiplier()
	if(satietyValue >= _foodEffectivenessMultiplierLimit)
		return _foodEffectivenessMultiplier
	else
		return calculateSatietyMultiplier()
	endif
endfunction


float function calculateSatietyMultiplier()
	if (isMoreOrEqual(satietyValue, _foodEffectivenessMultiplierLimit))
		return _foodEffectivenessMultiplier 
	else
		return _foodEffectivenessMultiplier + ((_foodEffectivenessMultiplierLimit - satietyValue) * _foodEffectivenessMultiplierPerValue)
	endif
endfunction


function handleSatietyChanges()
	updateCurrentSatietyStage()
	updateSatietyStatusMagicEffect()
endfunction


function updateSatietyStatusMagicEffect()
	removeSpell(_aaMPs_SatietyStatus)
	int descriptionId = _stagesDescriptions[getFactionRank(_aaMPf_SatietyFaction)]
	if (descriptionId != -1)
		_aaMPs_SatietyStatus.setNthEffectMagnitude(descriptionId, satietyValue)
		;_aaMPs_SatietyStatus.cast((self as Actor), (self as Actor))
		addSpell(_aaMPs_SatietyStatus, false)
	endif
endfunction


function updateCurrentSatietyStage()
	if (!isSatietyInCurrentInterval())
		setFactionRank(_aaMPf_SatietyFaction, getActualSatietyStage())
	endIf
endfunction


bool function isSatietyInCurrentInterval()
	return comparePointInInterval(satietyValue, lowerThresholdValueForCurrentStage, higherThresholdValueForCurrentStage)
endfunction


int function getActualSatietyStage()
	int cycleIteration = 0
	while (!comparePointInInterval(satietyValue, _satietyStages[cycleIteration], _satietyStages[cycleIteration + 1]))
		cycleIteration += 1
		if (cycleIteration == _satietyStages.length)
			return -1
		endif
	endwhile
	updateThresholdValues(cycleIteration)
	return cycleIteration
endfunction


function updateThresholdValues(int newStage)
	lowerThresholdValueForCurrentStage = _satietyStages[newStage]
	higherThresholdValueForCurrentStage = _satietyStages[newStage + 1]
endfunction


bool function comparePointInInterval(float value, float moreThen, float lessAndEqualThen)
	return (isMore(value, moreThen) && isLessOrEqual(value, lessAndEqualThen))
endfunction


bool function isMore(float value, float moreThen)
	return value > moreThen;
endfunction


bool function isLess(float value, float moreThen)
	return value < moreThen;
endfunction


bool function isLessOrEqual(float value, float lessAndEqualThen)
	return value <= lessAndEqualThen;
endfunction


bool function isMoreOrEqual(float value, float moreAndEqualThen)
	return value >= moreAndEqualThen;
endfunction