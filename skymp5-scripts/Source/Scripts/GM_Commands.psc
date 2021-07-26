Scriptname GM_Commands
{The documentation string.}

Bool Function HandleCommand(Actor ac, String[] tokens) global
    String command = tokens[0]
    If command == "/ban"
        GM_CommandBan.Execute(ac, tokens)
        Return true
    Else
    EndIf
    Return false
EndFunction
