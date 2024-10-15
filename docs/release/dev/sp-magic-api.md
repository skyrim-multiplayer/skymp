## Added API for Magic functions

### For casting and interrupting spells
```ts
function castSpellImmediate(actorCasterFormId: number, castingSource: SpellType, formIdSpell: number, formIdTarget: number, animationVariables: ActorAnimationVariables): void;
function interruptCast(actorCasterFormId: number, castingSource: SpellType, animationVariables: ActorAnimationVariables): void;
```

### To get and apply magic animation variables from/to an Actor
```ts
function getAnimationVariablesFromActor(actorFormId: number): ActorAnimationVariables;
function applyAnimationVariablesToActor(actorFormId: number, animationVariables: ActorAnimationVariables): boolean;
```

### SpellCast event handling improvments
- Also now `RE::TESSpellCastEvent` handling and calling the `spellCast` event will always occur, even if there is no `MagicCaster` for the slot in which the spell is located
