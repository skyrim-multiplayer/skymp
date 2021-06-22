Scriptname GM_ChatBind
{The documentation string.}

Bool Function HandleBindMessage(Actor ac, String text) global
    If text == ")"
        GM_ChatAction.HandleActionMessage(ac, "*" + M.GetText("smiled"))
        Return true
    EndIf
    Return false
EndFunction