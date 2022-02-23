ScriptName Game Hidden

Form Function GetForm(int aiFormID) native global
Form Function GetFormEx(int aiFormID) native global
Form Function GetFormFromFile(int aiFormID, string asFilename) Native Global
Actor Function GetPlayer() native global
ObjectReference Function FindClosestReferenceOfAnyTypeInList(FormList arBaseObjects, float afX, float afY, float afZ, float afRadius) native global
ObjectReference Function FindClosestReferenceOfAnyTypeInListFromRef(FormList arBaseObjects, ObjectReference arCenter, float afRadius) global
	return FindClosestReferenceOfAnyTypeInList(arBaseObjects, arCenter.X, arCenter.Y, arCenter.Z, afRadius)
endFunction
ObjectReference Function FindRandomReferenceOfAnyTypeInList(FormList arBaseObjects, float afX, float afY, float afZ, float afRadius) native global
ObjectReference Function FindRandomReferenceOfAnyTypeInListFromRef(FormList arBaseObjects, ObjectReference arCenter, float afRadius) global
	return FindRandomReferenceOfAnyTypeInList(arBaseObjects, arCenter.X, arCenter.Y, arCenter.Z, afRadius)
endFunction
Function ShowRaceMenu() native global
Function ShowLimitedRaceMenu() native global
Function ForceThirdPerson() native global
