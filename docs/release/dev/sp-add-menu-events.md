## Add menuOpen/menuClose events

New events were added: `menuOpen` and `menuClose`.

```ts
on("menuOpen", (e) => {
    printConsole(`The game opens menu: ${e.name}`);
});
```
