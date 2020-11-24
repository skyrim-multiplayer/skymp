ScriptName aaMP_ModActorValueInPercent extends ActiveMagicEffect

; ======== Properties ========

string property _actorValueName auto

; ======== Variables ========

Actor currentActor
float magnitude = 0.000000
float totalModificator = 0.000000
float carryWeightDelta = 0.0100000
string actorSpeedValue = "SpeedMult"
string carryWeightName = "CarryWeight"

; ======== Events ========

event onEffectStart(Actor akTarget, Actor akCaster)
    currentActor = akTarget
    magnitude = getMagnitude()
    totalModificator = 0.000000
    changeActorValueInPercent(-magnitude)
endevent


event onEffectFinish(Actor akTarget, Actor akCaster)
    changeActorValueInPercent(0.000000)
endevent

; ======== Functions ========

function changeActorValueInPercent(float percent)
    float baseValue = currentActor.getAV(_actorValueName) - totalModificator
    float newModificator = baseValue * percent / 100.000
    changeActorValue(newModificator - totalModificator)
endfunction


function changeActorValue(float value)
	totalModificator += value
    currentActor.modAV(_actorValueName, value)
    if (actorSpeedValue == _actorValueName)
        handleSpeedUpdate()
    endif
endfunction


function handleSpeedUpdate()
	currentActor.modAV(carryWeightName, -carryWeightDelta)
	currentActor.modAV(carryWeightName, carryWeightDelta)
endfunction