## Add API to contact back to game from browser

Added `window.skyrimPlatform.sendMessage` to be used on the browser side to talk back to the game. `sendMessage` accepts zero or more JSON-serializable values.

```js
window.skyrimPlatform.sendMessage({ foo: 'bar' });
window.skyrimPlatform.sendMessage(1, 2, 3, "yay");
window.skyrimPlatform.sendMessage();
```

You can call this function whenever you want: as a button callback, etc.

```html
<input type="button" value="Click me" onclick="window.skyrimPlatform.sendMessage({ foo: 'bar' });">
```

Calls to sendMessage result in a browserMessage event on the SP side. You can handle these events as any others.

```ts
on("browserMessage", (event) => {
  printConsole(JSON.stringify(event.arguments));
});
```
