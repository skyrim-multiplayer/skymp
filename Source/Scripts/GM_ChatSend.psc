Scriptname GM_ChatSend
{The documentation string.}

Function SendToAll(String msg) global
    Actor[] players = M.GetOnlinePlayers()
    Int index = 0
    While index < players.Length
        Actor pl = players[index]
        M.SendChatMessage(pl, msg)
        index += 1
    EndWhile
EndFunction

Function SendToPlayersNear(Actor ac, String msg, Float maxDistance) global
    String[] msgVariants = new String[1]
    msgVariants[0] = msg
    SendToPlayersNearVariadic(ac, msgVariants, maxDistance)
EndFunction

Function SendToPlayersNearVariadic(Actor ac, String[] msgVariants, Float maxDistance) global
    Actor[] streamedActors = M.GetActorsInStreamZone(ac)
    int index = 0
    While index < streamedActors.Length
        SendToPlayerIfNear(ac, streamedActors[index], msgVariants, maxDistance)
        index += 1
    EndWhile
EndFunction

Function SendToPlayerIfNear(Actor ac, Actor target, String[] msgVariants, Float maxDistance) global
    Float distance = ac.GetDistance(target)
    If distance >= 0 && distance < maxDistance
        String msg = ChooseMessageVariant(msgVariants, distance, maxDistance)
        M.SendChatMessage(target, msg)
    EndIf
EndFunction

String Function ChooseMessageVariant(String[] msgVariants, Float distance, Float maxDistance) global
    Int i = (distance / maxDistance * msgVariants.Length) as Int
    Return msgVariants[i]
EndFunction