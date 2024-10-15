## Added API for Magic functions

### Spell Casting and Interrupting
```ts
function castSpellImmediate(
  actorCasterFormId: number, 
  castingSource: SpellType, 
  formIdSpell: number, 
  formIdTarget: number, 
  animationVariables: ActorAnimationVariables
): void;

function interruptCast(
  actorCasterFormId: number, 
  castingSource: SpellType, 
  animationVariables: ActorAnimationVariables
): void;
```

### Handling Animation Variables
```ts
function getAnimationVariablesFromActor(actorFormId: number): ActorAnimationVariables;
function applyAnimationVariablesToActor(actorFormId: number, animationVariables: ActorAnimationVariables): boolean;
```

### SpellCast event handling improvments
- The `RE::TESSpellCastEvent` now triggers and handles the `spellCast` event consistently, regardless of the presence of a `MagicCaster` in the slot where the spell is located.
