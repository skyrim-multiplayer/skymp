Scriptname UIMenuBase extends Quest

bool Property isResetting = false Auto
bool _waitLock = false

Function Lock()
	_waitLock = true
EndFunction

bool Function WaitLock()
	int lockOut = 0
	While _waitLock
		If !(UI.IsMenuOpen("CustomMenu"))
			lockOut += 1
		EndIf
		If lockOut > 50 ; Took more than 5 sec
			_waitLock = false
			return false
		Endif
		Utility.Wait(0.1)
	EndWhile
	return true
EndFunction

Function Unlock()
	_waitLock = false
EndFunction

bool Function BlockUntilClosed()
	int counter = 0
	While UI.IsMenuOpen("CustomMenu")
		counter += 1
		If counter > 50
			return false
		Endif
		Utility.Wait(0.1)
	EndWhile

	return true
EndFunction

bool Function WaitForReset()
	int counter = 0
	While isResetting
		counter += 1
		If counter > 50
			isResetting = false
			return false
		Endif
		Utility.Wait(0.1)
	EndWhile

	return true
EndFunction

int Function OpenMenu(Form akForm = None, Form akReceiver = None)
	return -1
EndFunction

string Function GetMenuName()
	return ""
EndFunction

Event OnGameReload()
	
EndEvent

Function ResetMenu()

EndFunction

float Function GetResultFloat()
	return 0.0
EndFunction

int Function GetResultInt()
	return 0
EndFunction

string Function GetResultString()
	return ""
EndFunction

Form Function GetResultForm()
	return None
EndFunction

; Property functions

; Getters
int Function GetPropertyInt(string propertyName)
	return 0
EndFunction

bool Function GetPropertyBool(string propertyName)
	return false
EndFunction

string Function GetPropertyString(string propertyName)
	return ""
EndFunction

float Function GetPropertyFloat(string propertyName)
	return 0.0
EndFunction

Form Function GetPropertyForm(string propertyName)
	return None
EndFunction

Alias Function GetPropertyAlias(string propertyName)
	return None
EndFunction

; Setters
Function SetPropertyInt(string propertyName, int value)

EndFunction

Function SetPropertyBool(string propertyName, bool value)

EndFunction

Function SetPropertyString(string propertyName, string value)

EndFunction

Function SetPropertyFloat(string propertyName, float value)

EndFunction

Function SetPropertyForm(string propertyName, Form value)

EndFunction

Function SetPropertyAlias(string propertyName, Alias value)

EndFunction

; Property Index functions
Function SetPropertyIndexInt(string propertyName, int index, int value)

EndFunction

Function SetPropertyIndexBool(string propertyName, int index, bool value)

EndFunction

Function SetPropertyIndexString(string propertyName, int index, string value)

EndFunction

Function SetPropertyIndexFloat(string propertyName, int index, float value)

EndFunction

Function SetPropertyIndexForm(string propertyName, int index, Form value)

EndFunction

Function SetPropertyIndexAlias(string propertyName, int index, Alias value)

EndFunction

; Array Functions
Function SetPropertyIntA(string propertyName, int[] value)

EndFunction

Function SetPropertyBoolA(string propertyName, bool[] value)

EndFunction

Function SetPropertyStringA(string propertyName, string[] value)

EndFunction

Function SetPropertyFloatA(string propertyName, float[] value)

EndFunction

Function SetPropertyFormA(string propertyName, Form[] value)

EndFunction

Function SetPropertyAliasA(string propertyName, Alias[] value)

EndFunction
