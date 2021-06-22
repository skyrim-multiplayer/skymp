Scriptname GM_Colors
{This file contains all colors used in the gamemode}

String Function Default() global
    Return "#{ffffff}"
EndFunction

String Function Selection() global
    Return "#{a8adad}"
EndFunction

String[] Function Rp() global
    Return StringUtil.Split("#{ffffff} #{dedede} #{bfbfbf} #{787878}", " ")
EndFunction

String Function NonRp() global
    Return "#{a19177}"
EndFunction

String Function Action() global
    Return "#{c9b475}"
EndFunction

String Function AdminAction() global
    Return "#{963f3f}"
EndFunction