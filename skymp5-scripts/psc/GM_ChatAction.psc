Scriptname GM_ChatAction
{The documentation string.}

Bool Function HandleActionMessage(Actor ac, String text) global
    If IsActionMessage(text)
        String textCropped = CropAction(text)
        String msg = GM_Colors.Action() + ac.GetDisplayName() + " " + textCropped
        GM_ChatSend.SendToPlayersNear(ac, msg, GM_Distances.ChatDistance())
        Return true
    EndIf
    Return false
EndFunction

Bool Function IsActionMessage(String text) global
    Return StringUtil.GetNthChar(text, 0) == "*"
EndFunction

String Function CropAction(String text) global
    return GM_String.ReplaceAll(text, "*", "")
EndFunction
