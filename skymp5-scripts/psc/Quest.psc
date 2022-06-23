Scriptname Quest extends Form Hidden

bool Function ModObjectiveGlobal(float afModValue, GlobalVariable aModGlobal, int aiObjectiveID = -1, float afTargetValue = -1.0, bool abCountingUp = true, bool abCompleteObjective = true, bool abRedisplayObjective = true)
	aModGlobal.Mod(afModValue)
	UpdateCurrentInstanceGlobal(aModGlobal)
	if aiObjectiveID >= 0
		if afTargetValue > -1
			if (abCountingUp && aModGlobal.value >= afTargetValue) || (!abCountingUp && aModGlobal.value <= afTargetValue)
				if (abCompleteObjective)
					SetObjectiveCompleted(aiObjectiveID)
					return true
				Else
					SetObjectiveFailed(aiObjectiveID)
					return true
				Endif
			elseIf (abRedisplayObjective)
				SetObjectiveDisplayed(aiObjectiveID, true, true)
			Else
				SetObjectiveDisplayed(aiObjectiveID, true, false)
			endif
		elseIf (abRedisplayObjective)
			SetObjectiveDisplayed(aiObjectiveID, true, true)
		Else
			SetObjectiveDisplayed(aiObjectiveID, true, false)
		endif
	endif
	return false
endFunction
Function CompleteAllObjectives() native
Function CompleteQuest() native
Function FailAllObjectives() native
Alias Function GetAlias(int aiAliasID) native
int Function GetCurrentStageID() native
int Function GetStage()
  return GetCurrentStageID()
EndFunction
bool Function GetStageDone(int aiStage)
  return IsStageDone(aiStage)
EndFunction
bool Function IsActive() native
bool Function IsCompleted() native
bool Function IsObjectiveCompleted(int aiObjective) native
bool Function IsObjectiveDisplayed(int aiObjective) native
bool Function IsObjectiveFailed(int aiObjective) native
bool Function IsRunning() native
bool Function IsStageDone(int aiStage) native
bool Function IsStarting() native
bool Function IsStopping() native
bool Function IsStopped() native
Function Reset() native
Function SetActive(bool abActive = true) native
bool Function SetCurrentStageID(int aiStageID) native
Function SetObjectiveCompleted(int aiObjective, bool abCompleted = true) native
Function SetObjectiveDisplayed(int aiObjective, bool abDisplayed = true, bool abForce = false) native
Function SetObjectiveFailed(int aiObjective, bool abFailed = true) native
bool Function SetStage(int aiStage)
  return SetCurrentStageID(aiStage)
EndFunction
bool Function Start() native
Function Stop() native
bool Function UpdateCurrentInstanceGlobal( GlobalVariable aUpdateGlobal ) native
Event OnStoryAddToPlayer(ObjectReference akOwner, ObjectReference akContainer, Location akLocation, Form akItemBase, int aiAcquireType)
EndEvent
Event OnStoryArrest(ObjectReference akArrestingGuard, ObjectReference akCriminal, Location akLocation, int aiCrime)
EndEvent
Event OnStoryAssaultActor(ObjectReference akVictim, ObjectReference akAttacker, Location akLocation, int aiCrime)
EndEvent
Event OnStoryBribeNPC(ObjectReference akActor)
EndEvent
Event OnStoryCastMagic(ObjectReference akCastingActor, ObjectReference akSpellTarget, Location akLocation, Form akSpell)
EndEvent
Event OnStoryChangeLocation(ObjectReference akActor, Location akOldLocation, Location akNewLocation)
EndEvent
Event OnStoryCrimeGold(ObjectReference akVictim, ObjectReference akCriminal, Form akFaction, int aiGoldAmount, int aiCrime)
EndEvent
Event OnStoryCure(Form akInfection)
EndEvent
Event OnStoryDialogue(Location akLocation, ObjectReference akActor1, ObjectReference akActor2)
EndEvent
Event OnStoryDiscoverDeadBody(ObjectReference akActor, ObjectReference akDeadActor, Location akLocation)
EndEvent
Event OnStoryEscapeJail(Location akLocation, Form akCrimeGroup)
EndEvent
Event OnStoryActivateActor(Location akLocation, ObjectReference akActor)
EndEvent
Event OnStoryFlatterNPC(ObjectReference akActor)
EndEvent
Event OnStoryHello(Location akLocation, ObjectReference akActor1, ObjectReference akActor2)
EndEvent
Event OnStoryIncreaseLevel(int aiNewLevel)
EndEvent
Event OnStoryIncreaseSkill(string asSkill)
EndEvent
Event OnStoryInfection(ObjectReference akTransmittingActor, Form akInfection)
EndEvent
Event OnStoryIntimidateNPC(ObjectReference akActor)
EndEvent
Event OnStoryJail(ObjectReference akGuard, Form akCrimeGroup, Location akLocation, int aiCrimeGold)
EndEvent
Event OnStoryKillActor(ObjectReference akVictim, ObjectReference akKiller, Location akLocation, int aiCrimeStatus, int aiRelationshipRank)
EndEvent
Event OnStoryCraftItem(ObjectReference akBench, Location akLocation, Form akCreatedItem)
EndEvent
Event OnStoryNewVoicePower(ObjectReference akActor, Form akVoicePower)
EndEvent
Event OnStoryPickLock(ObjectReference akActor, ObjectReference akLock)
EndEvent
Event OnStoryPayFine(ObjectReference akCriminal, ObjectReference akGuard, Form akCrimeGroup, int aiCrimeGold)
EndEvent
Event OnStoryPlayerGetsFavor(ObjectReference akActor)
EndEvent
Event OnStoryRelationshipChange(ObjectReference akActor1, ObjectReference akActor2, int aiOldRelationship, int aiNewRelationship)
EndEvent
Event OnStoryRemoveFromPlayer(ObjectReference akOwner, ObjectReference akItem, Location akLocation, Form akItemBase, int aiRemoveType)
EndEvent
Event OnStoryScript(Keyword akKeyword, Location akLocation, ObjectReference akRef1, ObjectReference akRef2, int aiValue1, int aiValue2)
EndEvent
Event OnStoryServedTime(Location akLocation, Form akCrimeGroup, int aiCrimeGold, int aiDaysJail)
EndEvent
Event OnStoryTrespass(ObjectReference akVictim, ObjectReference akTrespasser, Location akLocation, int aiCrime)
EndEvent
Quest Function GetQuest(string editorId) global native
string Function GetID() native
int Function GetPriority() native
int Function GetNumAliases() native
Alias Function GetNthAlias(int index) native
Alias Function GetAliasByName(string name) native
Alias Function GetAliasById(int aliasId) native
Alias[] Function GetAliases() native