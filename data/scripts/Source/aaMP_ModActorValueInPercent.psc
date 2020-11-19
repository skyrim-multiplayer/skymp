ScriptName aaMP_ModActorValueInPercent extends ActiveMagicEffect

; ======== Properties ========

string property _actorValueName auto
float property _updateTime auto

; ======== Variables ========

bool isRun = false
Actor currentActor
float magnitude = 0.000000
float totalModificator = 0.000000
float carryWeightDelta = 0.0100000
string actorSpeedValue = "SpeedMult"
string carryWeightName = "CarryWeight"

; ======== Events ========

event onUpdate()
    changeActorValueInPercent(-magnitude)
    if (isRun)
        registerForSingleUpdate(_updateTime)
    else
        changeActorValueInPercent(0.000000)
    endif
endevent


event onEffectStart(Actor akTarget, Actor akCaster)
    currentActor = akTarget
    magnitude = getMagnitude()
    totalModificator = 0.000000
    isRun = true
    changeActorValueInPercent(-magnitude)
    registerForSingleUpdate(0.500000)
endevent


event onEffectFinish(Actor akTarget, Actor akCaster)
    isRun = false
    changeActorValueInPercent(0.000000)
endevent

; ======== Functions ========

function changeActorValueInPercent(float percent)
    float baseValue = currentActor.getAV(_actorValueName) - totalModificator
    float newModificator = baseValue * percent / 100.000
    changeActorValue(newModificator - totalModificator)
endfunction


function changeActorValue(float value)
    updateTotalModificator(value)
    currentActor.modAV(_actorValueName, value)
    if (actorSpeedValue == _actorValueName)
        handleSpeedUpdate()
    endif
endfunction


function handleSpeedUpdate()
	currentActor.modAV(carryWeightName, -carryWeightDelta)
	currentActor.modAV(carryWeightName, carryWeightDelta)
endfunction


function updateTotalModificator(float value)
	totalModificator += value
endfunction