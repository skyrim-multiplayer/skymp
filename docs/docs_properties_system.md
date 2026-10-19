# Properties System

This page contains common information about the property system of the API.

## Built-in Properties List

Some of the properties are built-in. It means that they are defined by the skymp server itself, not by scripts. You are able to use `mp.get`/`mp.set` with built-in properties just like with properties added by your code.

### Modifiable properties

These properties can be modified by a script with `mp.set`.

- pos (`[0,0,0]`)
- angle (`[0,0,0]`)
- worldOrCellDesc (`"3c:Skyrim.esm"`)
- inventory
- appearance
- isOpen
- isDisabled
- isDead
- canRespawn
- actorValues

`actorValues` object supports the following numeric fields:

- health
- magicka
- stamina
- healRate
- magickaRate
- staminaRate
- healRateMult
- magickaRateMult
- staminaRateMult

### Readonly properties

These properties can NOT be modified by a script with `mp.set`.

- type (`"MpActor"/"MpObjectReference"`)
- baseDesc (`"12eb7:Skyrim.esm"`)
- formDesc (`"0"`, `"1"`, `"14:Skyrim.esm"`)
- equipment
- isOnline
- neighbors ([0xff000000, 0xff000001, ...])

## Migration: from `percentages` to `actorValues`

The older `percentages` property (`mp.set(id, 'percentages', { health, magicka, stamina })`) sets the *current/max ratios* for the three primary stats. It remains valid for that purpose.

If you also need to change the **base values** (max health, max magicka, max stamina) or the rate/multiplier fields, use `actorValues` instead:

```js
// Old: only adjusts current % of health/magicka/stamina
mp.set(actorId, 'percentages', { health: 1.0, magicka: 1.0, stamina: 1.0 });

// New: set absolute base values and rates
mp.set(actorId, 'actorValues', {
  health: 250,
  magicka: 150,
  stamina: 200,
  healRateMult: 150,
});
```

Both can be combined: use `actorValues` to set the base maximums, then `percentages` to position the current value as a fraction of that maximum.

## Related docs

- For detailed revive/respawn flow and behavior, see [Respawn Control System](docs_respawn_control_system.md).
