Scriptname GM_Chat
{The documentation string.}

Function OnChatInput(Actor ac, String text) global
    If StringUtil.GetNthChar(text, 0) == "/"
        HandleChatCommand(ac, text)
    Else
        HandleChatMessage(ac, text)
    EndIf
EndFunction

Function HandleChatCommand(Actor ac, String text) global
    String[] tokens = StringUtil.Split(text, " ")

    If !GM_Commands.HandleCommand(ac, tokens)
        M.SendChatMessage(ac, GM_Colors.Default() + M.GetText("Unknown command") + ": " + GM_Colors.Selection() + tokens[0])
    EndIf
EndFunction

Function HandleChatMessage(Actor ac, String text) global
    GM_ChatNonRp.HandleNonRpMessage(ac, text) || GM_ChatBind.HandleBindMessage(ac, text) || GM_ChatAction.HandleActionMessage(ac, text) || GM_ChatRp.HandleRpMessage(ac, text)
EndFunction