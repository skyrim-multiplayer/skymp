# Server-Side Alchemy System Implementation Plan

## Goal

Implement a production-ready alchemy crafting system where players can:

1. Combine ingredients at alchemy workbenches.
2. Dynamically generate potions/poisons from ingredient effect combinations.
3. Persistently track learned ingredient effects per player.
4. Have all crafting logic validated and executed server-authoritatively.

The system must maintain server authority against client-side spoofing and match vanilla Skyrim alchemy mechanics.

## Current Baseline

- **Basic ingredient support**: `INGR` records are loaded from ESM/ESP files via `libespm`.
- **Effect reading**: `INGR::Data` includes `effects` (vector of `Effects::Effect`).
- **Potion data structure**: `ALCH::Data` includes `effects`, `weight`, `isFood`, `isPoison`.
- **Effect learning system** (TypeScript): `effectsLearningSystem.ts` tracks per-player learned ingredient effects in `private.learnedIngredientEffects`.
- **Consumption handling**: When a potion or ingredient is consumed via `OnEquip`, `EatItemEvent` fires and applies magic effects.
- **Craft Service** (for smithing): `CraftService` validates crafting using `COBJ` recipes, removes input items, adds output.
- **Papyrus API**: Basic potion queries exist (`IsFood`), but full potion interface is incomplete.
- **Skill system**: Alchemy skill exists in actor values (`espm::ActorValue::Alchemy`) but is never incremented.

## Scope

### In Scope (Phase 1)

- Implement `AlchemyService` mirroring `CraftService` architecture.
- Accept client alchemy craft requests (ingredients + target potion/poison FormID).
- Server-side effect intersection logic (what effects are learned/available).
- Ingredient consumption and potion generation.
- Skill advancement on successful craft.
- Synchronize crafted potion to actor inventory.
- Event firing (`OnAlchemyItemCrafted` or generic `OnCraft`).

### Out of Scope (Future Phases)

- Perk system (Alchemist, Physician, Concentrated Poison, etc.).
- Potion potency scaling based on player skill or perks.
- Potion value/weight dynamically computed.
- Ingredient effect discovery (learning from eating ingredients).
- UI/client-side alchemy menu logic (client-driven).
- Alchemy level-up progression curves.

## Architecture

### Data Model

#### Per-Player Alchemy State

Stored via `mp.set(actorId, ...)`:

```typescript
private.learnedIngredientEffects: {
  [ingredientFormId: number]: number[] // indices of effects learned
}
```

Example:
```json
{
  "0x3AD66": [0, 1],      // Blue Mountain Flower: effects 0 and 1 are known
  "0x06ABCB": [0]         // Salt Pile: only effect 0 is known
}
```

#### Alchemy Craft Request

Client sends via custom packet or game event when player attempts craft at workbench:

```cpp
struct AlchemyCraftRequest {
  uint32_t actorId;           // Actor performing craft
  uint32_t workbenchRefId;    // Reference ID of alchemy workbench
  uint32_t resultPotionFormId; // The potion/poison FormID to create (must exist in ESM/ESP)
  std::vector<uint32_t> ingredientFormIds; // Ingredient FormIDs used (with counts)
  // or:
  Inventory inputItems;       // Full inventory entry list of inputs
};
```

### Server Components (C++)

#### 1. `AlchemyService` Class

Similar structure to `CraftService`:

```cpp
class AlchemyService {
public:
  explicit AlchemyService(PartOne& partOne_);
  
  // Main craft entry point from action listener
  void OnAlchemyCraft(const RawMessageData& rawMsgData,
                      const Inventory& inputItems,
                      uint32_t workbenchRefId,
                      uint32_t resultPotionFormId);

private:
  PartOne& partOne;
  std::vector<espm::LookupResult> allIngredients; // cached
  
  // Validate workbench is actually an alchemy station
  bool IsValidAlchemyWorkbench(uint32_t baseId) const;
  
  // Check if player has access to workbench (location, faction, quest)
  bool CanUseWorkbench(MpActor* actor, uint32_t workbenchRefId) const;
  
  // Get all effects from ingredients that player has learned/has access to
  std::vector<espm::Effects::Effect> ComputeAvailableEffects(
    MpActor* actor,
    const std::vector<uint32_t>& ingredientFormIds,
    bool knownEffectsOnly = true // true = player must have discovered it
  ) const;
  
  // Get effects from the target potion FormID
  std::vector<espm::Effects::Effect> GetPotionEffects(
    uint32_t potionFormId
  ) const;
  
  // Check if crafted potion matches expected effects
  bool DoEffectsMatch(
    const std::vector<espm::Effects::Effect>& availableFromIngredients,
    const std::vector<espm::Effects::Effect>& expectedFromPotion
  ) const;
  
  // Apply skill advancement for alchemy
  void AdvanceAlchemySkill(MpActor* actor, float skillGain);
};
```

#### 2. Workbench Detection

Alchemy workbenches are marked in ESM/ESP with specific keywords:

```cpp
// In libespm or static constants
constexpr uint32_t kAlchemyKeywordId = 0x0010BB3D; // "AlchemyWorkbenchKeyword"
// or list of workbench base form IDs
```

Detect via:
- Furniture/Activator keyword matching
- Or explicit FormID list (similar to `ArmorTable`, `SharpeningWheel` in `CraftService`)

#### 3. Effect Matching Logic

**Vanilla Alchemy Rule**: A potion can be crafted if:
- All its effects are a **subset** of the available ingredient effects.
- OR: If the player has learned an effect, it's available from that ingredient even if other effects are unknown.

Example:
- Ingredient A has effects [Restore Health, Restore Magicka]
- Ingredient B has effects [Restore Health, Damage Stamina]
- Available intersection: [Restore Health]
- Player can craft any potion that uses **only** Restore Health

```cpp
bool AlchemyService::DoEffectsMatch(...) {
  // Simplified: check if all expected potion effects 
  // are present in available ingredient effects
  for (const auto& expectedEffect : expectedFromPotion) {
    bool found = false;
    for (const auto& availableEffect : availableFromIngredients) {
      if (availableEffect.effectId == expectedEffect.effectId) {
        found = true;
        break;
      }
    }
    if (!found) return false; // Effect not available
  }
  return true;
}
```

### Server-Side Validation Flow

```
1. Client sends AlchemyCraftRequest
   └─> ActionListener::OnAlchemyCraft() receives it
       
2. AlchemyService::OnAlchemyCraft():
   a) Validate workbench exists and is alchemy type
   b) Validate player has all ingredients in inventory
   c) Get learned effects from `private.learnedIngredientEffects`
   d) Compute available effects from those ingredients
   e) Get effects from target potion FormID
   f) Validate effects match (DoEffectsMatch)
   g) If valid:
      - Remove ingredients from inventory (Inventory::RemoveItems)
      - Add potion to inventory (Inventory::AddItem)
      - Fire OnCraft event (reuse CraftEvent or new OnAlchemyItemCrafted)
      - Advance alchemy skill
      - Save actor state
   h) If invalid:
      - Log error
      - Send failure message to client
      - Inventory unchanged
```

### Events

Reuse existing `CraftEvent` or create new:

```cpp
class AlchemyCraftEvent : public GamemodeEvent {
public:
  AlchemyCraftEvent(MpActor* actor_, uint32_t potionFormId_,
                    const std::vector<uint32_t>& ingredientFormIds_);
  
  const char* GetName() const override { return "onAlchemyCraft"; }
  
private:
  MpActor* actor;
  uint32_t potionFormId;
  std::vector<uint32_t> ingredientFormIds;
};
```

Papyrus script can hook:
```papyrus
Event OnAlchemyCraft(Actor akActor, Form akPotion, Form[] akIngredients)
  ; Custom logic: teleport, logging, quest progression, etc.
EndEvent
```

### Papyrus API (PapyrusPotion Completion)

Fill in missing methods in `PapyrusPotion`:

```cpp
VarValue PapyrusPotion::IsPoison(VarValue self, ...);
VarValue PapyrusPotion::GetNumEffects(VarValue self, ...);
VarValue PapyrusPotion::GetNthEffectMagnitude(VarValue self, ...);
VarValue PapyrusPotion::GetNthEffectDuration(VarValue self, ...);
VarValue PapyrusPotion::GetNthEffectArea(VarValue self, ...);
VarValue PapyrusPotion::GetNthMagicEffect(VarValue self, ...);
```

These wrap `ALCH::GetData(cache)` member access.

### Ingredient Effect Discovery (Out of Scope - Phase 2)

When player consumes ingredient (`OnEquip` for INGR):

1. First effect is always revealed.
2. Additional effects revealed only if player has high enough alchemy skill OR relevant perks.
3. Update `private.learnedIngredientEffects` accordingly.
4. Send update packet to client for UI feedback.

Currently partially implemented but effects application is commented out in `EatItemEvent.cpp` line 39.

## Client-Server Communication

### Packet Structure (TypeScript)

```typescript
interface AlchemyCraftMessage extends IMessageBase {
  customPacketType: 'alchemyCraft';
  actorId: number;
  workbenchRefId: number;
  resultPotionFormId: number;
  ingredientFormIds: number[]; // or full inventory entries
}

interface AlchemyCraftResultMessage extends IMessageBase {
  customPacketType: 'alchemyCraftResult';
  success: boolean;
  reason?: string; // "invalid-workbench", "missing-effects", etc.
}
```

Client detects alchemy workbench interaction and sends message via `mp.SendMessage(...)`.

## Security Rules (Must Have)

1. **Workbench Validation**: Verify `workbenchRefId` exists and its base form has alchemy keyword.
2. **Inventory Validation**: Before removing items, verify player owns all ingredients with required counts.
3. **Effect Validation**: Effects must be a valid subset of learned/available effects from ESM/ESP.
4. **Server Authority**: Never trust client-computed effects or result potion ID; always recompute server-side.
5. **Skill Advancement**: Only increment after successful craft; use standard `RestoreActorValue` API.
6. **Acid Test**: Unit tests must verify:
   - Invalid potion FormID is rejected.
   - Missing one ingredient fails craft.
   - Inventory is unchanged on failed craft.
   - Ingredients correctly removed on success.
   - Skill advancement matches vanilla formula.

## Implementation Phases

### Phase 1: Core Alchemy Service

1. Create `AlchemyService` class in `skymp5-server/cpp/server_guest_lib/`.
2. Implement workbench detection (FURN/ACTI keyword matching).
3. Implement `ComputeAvailableEffects()` (read from learned effects, query INGR records).
4. Implement `DoEffectsMatch()` validation.
5. Hook into `ActionListener::OnAlchemyCraft()`.
6. Write unit tests (mirroring `CraftTest.cpp`).
7. Fire `OnCraft` event or new `OnAlchemyCraft` event.

### Phase 2: Skill Advancement & UX

1. Complete `PapyrusPotion` API.
2. Implement skill gain formula (similar to vanilla).
3. Add effect discovery logic (eating ingredients).
4. Client-side menu integration (if doing client-side UI).

### Phase 3: Perks & Balance

1. Parse perk records and apply modifiers.
2. Potency scaling by player alchemy skill.
3. Value/weight computation.

## Testing Strategy

Create `AlchemyTest.cpp` (mirror of `CraftTest.cpp`):

```cpp
TEST_CASE("Player crafts potion with known effects", "[Alchemy]") {
  // Setup: create actor with blue mountain flower + canis root
  // Both have "Restore Health" as first effect
  // Attempt to craft "Healing Potion" which uses "Restore Health"
  // Expect: success, potion added, ingredients removed, skill advanced
}

TEST_CASE("Player crafts invalid potion", "[Alchemy]") {
  // Setup: actor has only Restore Health known from ingredients
  // Attempt to craft "Damage Health" potion
  // Expect: failure, inventory unchanged
}

TEST_CASE("Workbench validation", "[Alchemy]") {
  // Attempt craft at non-alchemy workbench
  // Expect: failure
}
```

## Open Questions / TBD

1. **Client-Side Menu**: Does client use SpSnippet to show vanilla alchemy menu, or custom UI?
   - Recommend: SpSnippet to native menu, then capture via gamemode event listener.

2. **Ingredient Counts**: Can player use 2x Blue Mountain Flower in one craft?
   - Recommend: Yes, allow duplicates; validate total count against ESM recipe.

3. **Potion Storage**: Generated potions are dynamically created at craft time.
   - Recommend: Use existing static ALCH FormID pool (e.g., 0x3CEAD for "Potion of Healing").
   - If custom potions needed: create a naming scheme (e.g., "SKYMP_AlchemyPotion_<hash>").

4. **Persistence**: Are crafted potions different from base potion?
   - Recommend: Same as vanilla — all "Potion of Healing" FormID 0x3CEAD are identical; only effects differ.

5. **Alchemy Skill Levels**: How much skill per craft?
   - Recommend: Calculate based on potion complexity (number of effects, magnitude).

## References

- [Skyrim UESP: Alchemy](https://en.uesp.net/wiki/Skyrim:Alchemy)
- [Vanilla Alchemy Recipe Algorithm](https://en.uesp.net/wiki/Skyrim:Alchemy#Recipe_Constraints)
- Existing: `CraftService.cpp`, `EatItemEvent.cpp`, `effectsLearningSystem.ts`
- libespm: `INGR.h`, `ALCH.h`, `Effects.h`
