Scriptname GlobalVariable extends Form Hidden

float Function GetValue() native
Function SetValue(float afNewValue) native
int Function GetValueInt()
	return GetValue() as int
endFunction
Function SetValueInt(int aiNewValue)
	SetValue(aiNewValue as float)
endFunction
float Property Value Hidden
  float Function get()
    return GetValue()
  EndFunction
  
  Function set(float afValue)
    SetValue(afValue)
  EndFunction
EndProperty
float Function Mod(float afHowMuch)
                Value += afHowMuch
                Return Value
EndFunction
