Scriptname LatentTest

Function LatentFunc() global native
Function NonLatentFunc() global native
Int Function LatentAdd(Int a, Int b) global native

Function Main() global
  NonLatentFunc()
  LatentFunc()
  NonLatentFunc()
EndFunction

Int Function Main2() global
  Int c = LatentAdd(2, 3)
  Return c
EndFunction
