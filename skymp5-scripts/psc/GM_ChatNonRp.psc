Scriptname GM_ChatNonRp
{The documentation string.}

Bool Function HandleNonRpMessage(Actor ac, String text) global
    If IsNonRpMessage(text)
        String textCropped = CropNonRpMessage(text)
        String msg = GM_Colors.NonRp() + "(( " + ac.GetDisplayName() + ": " + textCropped + " ))"
        GM_ChatSend.SendToPlayersNear(ac, msg, GM_Distances.ChatDistance())
        Return true
    EndIf
    Return false
EndFunction

Bool Function IsNonRpMessage(String text) global
    Return StringUtil.GetNthChar(text, 0) == "(" && StringUtil.GetNthChar(text, 1) == "("
EndFunction

String Function CropNonRpMessage(String text) global
    Int len = StringUtil.GetLength(text)

    If GM_String.EndsWith(text, "))")
        Return StringUtil.Substring(text, 2, len - 4)
    Else
        Return StringUtil.Substring(text, 2, len - 2)
    EndIf
EndFunction
