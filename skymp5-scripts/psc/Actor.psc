ScriptName Actor extends ObjectReference

Function AddPerk(Perk akPerk) native
Function RemovePerk(Perk akPerk) native
bool Function HasPerk(Perk akPerk) native
bool Function IsEquipped(Form akItem) native
bool Function IsWeaponDrawn() native
Function EquipItem(Form akItem, bool abPreventRemoval = false, bool abSilent = false) native
Function EquipItemEx(Form item, int equipSlot = 0, bool preventUnequip = false, bool equipSound = true) native
Function UnequipItem(Form akItem, bool abPreventEquip = false, bool abSilent = false) native
Function UnequipItemEx(Form item, int equipSlot = 0, bool preventEquip = false) native
Function UnequipAll() native
Function UnequipItemSlot(int aiSlot) native
Function SetActorValue(string asValueName, float afValue) native
Function GetEquippedArmorInSlot(int aiSlot) native

Function SetAV(string asValueName, float afValue) native

float Function GetActorValue(string asValueName) native
float Function GetAV(string asValueName) native

Function DamageActorValue(string asValueName, float afDamage) native
Function DamageAV(string asValueName, float afDamage) native

Function RestoreActorValue(string asValueName, float afAmount) native
Function RestoreAV(string asValueName, float afAmount)  native
Function SetOutfit(Outfit akOutfit, bool abSleepOutfit = false) native
Function SetRace(Race akRace = None) native

bool Function IsDead() Native
