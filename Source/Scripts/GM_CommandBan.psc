Scriptname GM_CommandBan
{The documentation string.}

Int Function FF() global
    Return 108
EndFunction

Function Execute(Actor ac, String[] tokens) global
    String ff = "dlld"
    GM_ChatSend.SendToAll(FF())

    String name = tokens[1]
    String time = tokens[2]
    String reason = tokens[3]

    String fmt = "%s has been banned by %s. Reason: %s"
    String format = M.GetText(fmt)
    String[] formatArgs = new String[3]
    formatArgs[0] = name
    formatArgs[1] = ac.GetDisplayName()
    formatArgs[2] = reason
    String text = M.Format(format, formatArgs)
    GM_ChatSend.SendToAll(GM_Colors.AdminAction() + text)
EndFunction