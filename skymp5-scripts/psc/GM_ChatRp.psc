Scriptname GM_ChatRp
{The documentation string.}

Function HandleRpMessage(Actor ac, String text) global
    String formattedText = WrapRpActions(text, GM_Colors.Default(), GM_Colors.Action())
    String msg = GM_Colors.Default() + ac.GetDisplayName() + ": " + formattedText
    String[] msgVariants = CreateMessageVariants(msg)

    GM_ChatSend.SendToPlayersNearVariadic(ac, msgVariants, GM_Distances.ChatDistance())
EndFunction

String[] Function CreateMessageVariants(String msg) global
    String[] colors = GM_Colors.Rp()
    String[] msgVariants = Utility.CreateStringArray(colors.Length)
    int index = 0
    While index < msgVariants.Length
        msgVariants[index] = GM_String.ReplaceAll(msg, GM_Colors.Default(), colors[index])
        index += 1
    EndWhile
    Return msgVariants
EndFunction

String Function WrapRpActions(String text, String defaultColor, String actionColor) global
    String[] tokens = StringUtil.Split(text, "*")
    String res
    int index = 0
    String delimiter = ""
    While index < tokens.Length
        String token = tokens[index]
        If index % 2 == 0 
            res += delimiter + defaultColor
        Else
            res += actionColor + delimiter
        EndIf
        res += token
        index += 1
        delimiter = "*"
    EndWhile

    If tokens.Length % 2 == 0
        res += delimiter
    EndIf

    Return res
EndFunction
