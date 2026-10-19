Scriptname MedicalDownedMode extends ObjectReference
{Custom respawn control system - players need healer to revive}

; Configuration
float fHealerReviveRange = 20.0
float fAutoReviveTimeout = 300.0
Spell spHealingTouch  ; Optional: Healing spell for revivals

; Event: Player dies - disable auto-respawn
Event OnActorKilled(Actor akKiller)
  Actor playerRef = Game.GetPlayer()
  
  ; Disable auto-respawn
  mp_set(playerRef, "canRespawn", false)
  
  Debug.Notification("You are downed! A healer must revive you.")
  Debug.Trace("Player downed. Waiting for healer...")
  
  ; Start auto-revival timeout
  StartTimer(fAutoReviveTimeout, 1)
EndEvent

; Event: Healer casts revival spell on downed player
Event OnSpellCast(Form akCaster, MagicEffect akEffect)
  if akEffect == spHealingTouch
    Actor healer = akCaster as Actor
    Actor player = Game.GetPlayer()
    
    ; Check distance
    float distance = GetDistance(healer, player)
    if distance > fHealerReviveRange
      Debug.Notification("Target too far away")
      return
    endif
    
    ; Check if healer is alive and qualified
    if healer.IsDead()
      Debug.Notification("Healer is dead")
      return
    endif
    
    ; Revive the player
    RevivePlayer(player, healer)
  endif
EndEvent

; Timer: Auto-revival after timeout
Event OnTimer(int aiTimerID)
  if aiTimerID == 1  ; Auto-revival timeout
    Actor playerRef = Game.GetPlayer()
    
    ; Only auto-revive if still dead
    if mp_get(playerRef, "isDead") as bool
      Debug.Notification("Auto-revived after timeout")
      RevivePlayer(playerRef, None)
    endif
  endif
EndEvent

; Function: Revive a downed player
Function RevivePlayer(Actor akPlayer, Actor akHealer)
  ; Resurrect
  mp_set(akPlayer, "isDead", false)
  
  ; Re-enable respawn
  mp_set(akPlayer, "canRespawn", true)
  
  ; Restore health (optional)
  if akPlayer.GetActorValuePercent("Health") < 0.25
    akPlayer.RestoreActorValue("Health", 50.0)
  endif
  
  ; Stop revival timer
  CancelTimer(1)
  
  ; Log
  if akHealer
    Debug.Trace(akPlayer.GetName() + " revived by " + akHealer.GetName())
    Debug.Notification(akPlayer.GetName() + " has been revived!")
  else
    Debug.Trace(akPlayer.GetName() + " auto-revived after timeout")
    Debug.Notification("You have been auto-revived")
  endif
EndFunction

; Helper: Calculate distance between two actors
float Function GetDistance(Actor akActor1, Actor akActor2)
  float dx = akActor1.GetPositionX() - akActor2.GetPositionX()
  float dy = akActor1.GetPositionY() - akActor2.GetPositionY()
  float dz = akActor1.GetPositionZ() - akActor2.GetPositionZ()
  
  return Math.Sqrt(dx * dx + dy * dy + dz * dz)
EndFunction

; Alternative: Quest-based revival system
; Instead of spell-based, use a quest to handle revivals

Scriptname HealerRevivalQuest extends Quest
{Quest that manages healer revival system}

Event OnUpdate()
  Actor playerRef = Game.GetPlayer()
  
  ; Check if player is downed
  if mp_get(playerRef, "isDead") as bool and not (mp_get(playerRef, "canRespawn") as bool)
    ; Look for nearby healers (NPCs with "Healer" rank in Healers Guild)
    Actor[] nearbyHealers = FindNearbyHealers(playerRef, 50.0)
    
    if nearbyHealers.Length > 0
      ; Healer detected nearby - prompt player interaction
      Debug.Notification("A healer is nearby. They can revive you.")
    endif
  endif
  
  ; Update frequency
  RegisterForUpdate(1.0)
EndEvent

; Find nearby healers (script simplified for example)
Actor[] Function FindNearbyHealers(Actor akPlayer, float afDistance)
  Actor[] result = new Actor[10]
  int index = 0
  
  ; This is simplified - in production you'd iterate NPCs properly
  ; For now, just return empty array
  
  return result
EndFunction
