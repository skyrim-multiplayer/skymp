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
