## Add on spellCast event

Now we can catch spellCast events

```typescript
on("spellCast", (event) => {
    printConsole(event.caster.getFormID());
    printConsole(event.spell.getName());
})
```

Event object contain fields `ObjectReference caster` and `Spell spell`
