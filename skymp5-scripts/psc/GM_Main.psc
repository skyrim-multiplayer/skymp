Scriptname GM_Main
{The documentation string.}

Function _OnChatInput(Actor ac, String text)
    GM_Chat.OnChatInput(ac, text)
EndFunction

Function _OnPapyrusRegister()
    M.Log("Hello Papyrus")
EndFunction
