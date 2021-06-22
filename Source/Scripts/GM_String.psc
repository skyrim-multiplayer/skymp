Scriptname GM_String
{The documentation string.}

Bool Function EndsWith(String str, String ending) global
    Int len = StringUtil.GetLength(str)
    Int endingLen = StringUtil.GetLength(ending)
    String realEnding = StringUtil.Substring(str, len - endingLen, 0)
    Return realEnding == ending
EndFunction

String Function ReplaceAll(String str, String from, String to) global
    String[] tokens = StringUtil.Split(str, from)
    String res
    Int index = 0
    While index < tokens.Length
        res += tokens[index]
        If index != tokens.Length - 1
            res += to
        EndIf
        index += 1
    EndWhile
    Return res
EndFunction