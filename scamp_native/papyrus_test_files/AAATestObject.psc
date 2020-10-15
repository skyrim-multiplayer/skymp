 Scriptname AAATestObject

Float Function ReturbBackValue(Float value)
  Return value
EndFunction

Int parentVariable

Int Property parentProperty

  Int Function get()
    Return parentVariable
  EndFunction

  Function set(int value)
    parentVariable = value
  EndFunction

EndProperty
