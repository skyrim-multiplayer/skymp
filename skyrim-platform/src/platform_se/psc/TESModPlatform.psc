scriptName TESModPlatform

function MoveRefrToPosition(ObjectReference refr, cell cell, WorldSpace world, Float posX, Float posY, Float posZ, Float rotX, Float rotY, Float rotZ) global native

Int function Add(Int a1, Int a2, Int a3, Int a4, Int a5, Int a6, Int a7, Int a8, Int a9, Int a10, Int a11, Int a12) global native

function SetWeaponDrawnMode(Actor actor, Int mode) global native

Int function GetNthVtableElement(Form pointer, Int pointerOffset, Int elementIndex) global native

Bool Function IsPlayerRunningEnabled() global native

ColorForm Function GetSkinColor(ActorBase base) global native

ActorBase Function CreateNpc() global native

ActorBase Function EvaluateLeveledNpc(String commaSeparatedListOfIds) global native

Function SetNpcSex(ActorBase npc, Int sex) global native

Function SetNpcRace(ActorBase npc, Race race) global native

Function SetNpcSkinColor(ActorBase npc, Int skinColor) global native

Function SetNpcHairColor(ActorBase npc, Int hairColor) global native

Function ResizeHeadpartsArray(ActorBase npc, Int newSize) global native

Function ResizeTintsArray(Int newSize) global native

Function SetFormIdUnsafe(Form form, Int newId) global native

Function ClearTintMasks(Actor targetActor) global native

Function PushTintMask(Actor targetActor, Int type, Int argb, String texturePath) global native

Function PushWornState(Bool worn, Bool wornLeft) global native

Function AddItemEx(ObjectReference containerRefr, Form item, Int countDelta, Float health, Enchantment enchantment, Int maxCharge, Bool removeEnchantmentOnUnequip, Float chargePercent, String textDisplayData, Int soul, Potion poison, Int poisonCount) global native

Function UpdateEquipment(Actor actor, Form item, Bool leftHand) global native

Function ResetContainer(Form container) global native

Function BlockPapyrusEvents(Bool block) global native

ObjectReference Function CreateReferenceAtLocation(Form baseForm, Cell cell, WorldSpace world, Float posX, Float posY, Float posZ, Float rotX, Float rotY, Float rotZ, Bool persist) global native
