Scriptname Alias Hidden

Quest Function GetOwningQuest() native
bool Function RegisterForAnimationEvent(ObjectReference akSender, string asEventName) native
Function RegisterForLOS(Actor akViewer, ObjectReference akTarget) native
Function RegisterForSingleLOSGain(Actor akViewer, ObjectReference akTarget) native
Function RegisterForSingleLOSLost(Actor akViewer, ObjectReference akTarget) native
Function RegisterForSingleUpdate(float afInterval) native
Function RegisterForUpdate(float afInterval) native
Function RegisterForUpdateGameTime(float afInterval) native
Function RegisterForSingleUpdateGameTime(float afInterval) native
Function RegisterForSleep() native
Function RegisterForTrackedStatsEvent() native
Function StartObjectProfiling() native
Function StopObjectProfiling() native
Function UnregisterForLOS(Actor akViewer, ObjectReference akTarget) native
Function UnregisterForAnimationEvent(ObjectReference akSender, string asEventName) native
Function UnregisterForSleep() native
Function UnregisterForTrackedStatsEvent() native
Function UnregisterForUpdate() native
Function UnregisterForUpdateGameTime() native
Event OnAnimationEvent(ObjectReference akSource, string asEventName)
EndEvent
Event OnAnimationEventUnregistered(ObjectReference akSource, string asEventName)
EndEvent
Event OnGainLOS(Actor akViewer, ObjectReference akTarget)
EndEvent
Event OnLostLOS(Actor akViewer, ObjectReference akTarget)
EndEvent
Event OnSleepStart(float afSleepStartTime, float afDesiredSleepEndTime)
EndEvent
Event OnSleepStop(bool abInterrupted)
EndEvent
Event OnTrackedStatsEvent(string arStatName, int aiStatValue)
EndEvent
Event OnUpdate()
EndEvent
Event OnUpdateGameTime()
EndEvent