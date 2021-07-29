Scriptname M
{The documentation string.}

Function ExecuteUiCommand(Actor actor, String commandType, String[] argumentNames, String[] tokens, String alter) global native

Function Log(String text) global native

String Function GetText(String msgid) global native

String Function Format(String format, String[] tokens) global native

Actor[] Function GetActorsInStreamZone(Actor actor) global native

Bool Function IsOnline(Actor actor) global native

Actor[] Function GetOnlinePlayers() global native

; ??
Function SetGlobalStorageValue(String key, String value) global native

; ??
String Function GetGlobalStorageValue(String key) global native

Function SendChatMessage(Actor ac, String msg) global
    M.ExecuteUiCommand(ac, "", None, None, msg)
EndFunction
