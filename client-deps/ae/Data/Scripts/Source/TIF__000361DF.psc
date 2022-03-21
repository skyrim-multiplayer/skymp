;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 1
Scriptname TIF__000361DF Extends TopicInfo Hidden

;BEGIN FRAGMENT Fragment_0
Function Fragment_0(ObjectReference akSpeakerRef)
Actor akSpeaker = akSpeakerRef as Actor
;BEGIN CODE
; set fake malborn to be a teammate
;Alias_PlayerGearContainerHidden.GetActorRef().SetPlayerTeammate()
;Alias_PlayerGearContainerHidden.GetActorRef().OpenInventory(true)

(GetOwningQuest().GetAlias(73) as ReferenceAlias).GetActorRef().SetPlayerTeammate()
(GetOwningQuest().GetAlias(73) as ReferenceAlias).GetActorRef().OpenInventory(true)

While (Utility.IsInMenuMode())
Utility.Wait(0.1)
EndWhile
GetOwningQuest().SetStage(70)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment
