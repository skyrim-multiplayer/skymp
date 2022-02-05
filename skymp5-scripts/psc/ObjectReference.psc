ScriptName ObjectReference extends Form

float Function GetDistance(ObjectReference akOther) Native
Function RemoveAllItems(ObjectReference akTransferTo = None, bool abKeepOwnership = false, bool abRemoveQuestItems = false) Native
Function AddItem(Form akItemToAdd, int aiCount = 1, bool abSilent = false) Native
Function RemoveItem(Form akItemToRemove, int aiCount = 1, bool abSilent = false, ObjectReference akOtherContainer = None) Native
int Function GetItemCount(Form akItem) Native
ObjectReference Function PlaceAtMe(Form akFormToPlace, int aiCount = 1, bool abForcePersist = false, bool abInitiallyDisabled = false) Native
int Function GetCurrentDestructionStage() Native
Form[] Function GetContainerForms() Native
string Function GetDisplayName() Native
Function Disable(bool abFadeOut = False) Native
bool Function SetDisplayName(string name, bool force = false) Native
Form Function GetBaseObject() Native
Function SetPosition(float afX, float afY, float afZ) Native
float Function GetPositionX() native
float Function GetPositionY() native
float Function GetPositionZ() native
Function SetAngle(float afXAngle, float afYAngle, float afZAngle) Native
Function DamageObject(float afDamage) Native
Function ClearDestruction() Native
bool Function IsInInterior() Native
Cell Function GetParentCell() Native
WorldSpace Function GetWorldSpace() Native
Function SetScale(float afScale) Native
Function MoveTo(ObjectReference akTarget, float afXOffset = 0.0, float afYOffset = 0.0, float afZOffset = 0.0, bool abMatchRotation = true) Native
Function SetOpen(bool abOpen = true) Native
Function BlockActivation(bool abBlocked = True) native
bool Function IsActivationBlocked() native
Function Enable(bool abFadeIn = false) native
Function SetMotionType(int aeMotionType, bool abAllowActivate = true) native
float Function GetHeadingAngle(ObjectReference akOther) native
bool Function Activate(ObjectReference akActivator, bool abDefaultProcessingOnly = false) native
Event OnActivate(ObjectReference akActionRef)
EndEvent
Event OnTrigger(ObjectReference akActionRef)
EndEvent
Event OnTriggerEnter(ObjectReference akActionRef)
EndEvent
Event OnTriggerLeave(ObjectReference akActionRef)
EndEvent

int Property Motion_Dynamic = 1 AutoReadOnly
int Property Motion_SphereIntertia = 2 AutoReadOnly
int Property Motion_BoxIntertia = 3 AutoReadOnly
int Property Motion_Keyframed = 4 AutoReadOnly
int Property Motion_Fixed = 5 AutoReadOnly
int Property Motion_ThinBoxIntertia = 6 AutoReadOnly
int Property Motion_Character = 7 AutoReadOnly

float Property X = 0 AutoReadOnly
float Property Y = 0 AutoReadOnly
float Property Z = 0 AutoReadOnly