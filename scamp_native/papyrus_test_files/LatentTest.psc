Scriptname LatentTest

Function LatentFunc() global native
Function NonLatentFunc() global native

Function Main() global
  NonLatentFunc()
  LatentFunc()
  NonLatentFunc()
EndFunction
