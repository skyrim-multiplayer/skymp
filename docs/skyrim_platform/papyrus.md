# Papyrus types from the original game

All types in SkyrimPlatform have the same name as in Papyrus, for example: `Game`, `Actor`, `Form`, `Spell`, `Perk`, etc.

To use types from Papyrus, including calling methods and static functions that they have, they need to be imported:

  ```typescript
  import { Game, Actor } from "skyrimPlatform";
  ```

***WARNING***: Papyrus types and methods are only available inside [hooks][Hooks] or [new events][NewEvents] introduced by Skyrim Platform (except `tick`).

An exception will be thrown if you try to use them outside any of those contexts.

## Form

- Form (`Form`) is inherited by most game types, which have methods such as `Actor`, `Weapon`, etc.
- Each form has an ID, which is a 32-bit unsigned number (`uint32_t`). In SkyrimPlatform, represented by the type `number`.
- If you need to find a form by its ID, use `Game.getFormEx`.\
  Note that it is `Game.getFormEx`, not `Game.getForm`. The latter always returns `null` for IDs above 0x80000000 (the behavior of the original game).
- You can get the form ID using the `getFormID` method. It is guaranteed that `Game.getFormEx` will find the form by the ID returned by this method if the form was not destroyed by the game.

## Object casting

- If you have a `Form` object that is a weapon, and you need a `Weapon` object, you can use casting:
  ```typescript
  let sword = Game.getFormEx(swordId); // Get Form
  let weapon = Weapon.from(sword); // Cast to Weapon
  ```
- If you specify an ID for a form that is not actually a weapon, the `weapon` variable will be `null`.
- Passing `null` to the function for casting types as an argument will not throw an exception, but will return `null`:
  ```typescript
  ObjectReference.from(null); // null
  ```
- An attempt to cast to a type that has no instances or is incompatible in the inheritance hierarchy will also return `null`:
  ```typescript
  Game.from(Game.getPlayer()); // null
  Spell.from(Game.getPlayer()); // null
  ```
- You can also use typecasting to get an object of the base type, including `Form`:
  ```typescript
  let refr = ObjectReference.from(Game.getPlayer());
  let form = Form.from(refr);
  ```
- Casting an object to its own type will return the original object:
  ```typescript
  let actor = Actor.from(Game.getPlayer());
  ```

# Safe use of objects

- After you get the object, you need to make sure that it is not `null`:

  ```typescript
  let actor = Game.findClosestActor(x, y, z, radius);
  if (actor) {
    let isInCombat = actor.isInCombat();
  }
  ```

  Or

  ```typescript
  let actor = Game.findClosestActor(x, y, z, radius);
  if (!actor) return;
  let isInCombat = actor.isInCombat();
  ```
- It is guaranteed that `Game.getPlayer` never returns `null`.

## Compiler checks

- Use the `strict` option in `tsconfig.json` to enable/disable compiler null checks and other correctness checks. \
  Learn more here: https://www.typescriptlang.org/tsconfig#strict.
- It's also possible to disable compiler checks on a per case basis.

  You can disable compiler checkings for a whole *.ts file if said file starts with the following comment:

  ```typescript
  // @ts-nocheck
  ```

  Or you can disable checking for a single line by including the following comment on the previous line:

  ```typescript
  // @ts-ignore
  ```

# Object comparison

To compare objects in SkyrimPlatform, you need to compare their IDs:

```typescript
if (object1.getFormId() === object2.getFormId()) {
  // ...
}
```

# Casting objects to string

Types ported from Papyrus have limited support for a number of operations normal for regular JS objects such as `toString`, `toJSON`.

```typescript
Game.getPlayer().ToString(); // '[object Actor]'
JSON.stringify(Game.getPlayer()); // `{}`
  ```

[Hooks]: events.md
[NewEvents]: new_events.md
