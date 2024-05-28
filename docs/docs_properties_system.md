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
- isDead (setting isDead to true will not initiate respawn timer. only "natural" actor deaths lead to respawn. consider using DamageActorValue)

### Readonly properties

These properties can NOT be modified by a script with `mp.set`.

- type (`"MpActor"/"MpObjectReference"`)
- baseDesc (`"12eb7:Skyrim.esm"`)
- formDesc (`"0"`, `"1"`, `"14:Skyrim.esm"`)
- equipment
- isOnline
- neighbors ([0xff000000, 0xff000001, ...])
