# Browser basics

Create `Data/Platform/UI/index.html` with contents below to test:

```html
<font color="white"><h1>Hello SP</h1></font>
```

SkyrimPlatform loads `Data/Platform/UI/index.html` if the file exists. It is also possible to load URLs in runtime.

```typescript
import { browser } from 'skyrimPlatform';

// Enable/disable browser visibility
browser.setVisible(true);

// Open cursor and redirect mouse and keyboard events to the browser
browser.setFocused(true);

// Read visibility/focus flags that were set before
const visible = browser.isVisible();
const focused = browser.isFocused();

// Load a specified URL. The current implementation loads URLs only after the user moves the mouse, except the default URL.
// CAUTION! Do not call this function with local URLs if you want your mod to work under MO2.
browser.loadUrl('file:///Data/Platform/UI/index.html'); // Default one
browser.loadUrl(''); // Same effect for empty URL
browser.loadUrl('file:///Data/Platform/UI/another-file.html'); // Load another page from Data
browser.loadUrl('https://google.com'); // Open websites
browser.loadUrl('http://localhost:9000'); // Open remote dev tools. You better open them in normal browser
browser.loadUrl('http://localhost:1234'); // Your favorite dev server in watch mode

// Execute JavaScript code in browser context
browser.executeJavaScript("console.log('Hello CEF')");

// SkyrimPlatform generates a unique token every game start
// In browser context, it appears in `window.spBrowserToken` after some time from page load moment
const str = browser.getToken();
```

#### Talking back to the game

Use `window.skyrimPlatform.sendMessage` on the browser side to talk back to the game. `sendMessage` accepts zero or more JSON-serializable values.

```js
window.skyrimPlatform.sendMessage({ foo: 'bar' });
window.skyrimPlatform.sendMessage(1, 2, 3, 'yay');
window.skyrimPlatform.sendMessage();
```

You can call this function whenever you want: as a button callback, etc.

```html
<input
  type="button"
  value="Click me"
  onclick="window.skyrimPlatform.sendMessage({ foo: 'bar' });"
/>
```

Calls to `sendMessage` result in a `browserMessage` event on the SP side. You can handle these events as any others.

```ts
on('browserMessage', (event) => {
  printConsole(JSON.stringify(event.arguments));
});
```

"Ping-pong" example: SP context communicates with browser context via `executeJavaScript`, and the browser context communicates back with `window.skyrimPlatform.sendMessage`.

```ts
once('tick', () => {
  browser.executeJavaScript("window.skyrimPlatform.sendMessage('yay')");
});

on('browserMessage', (event) => {
  printConsole(JSON.stringify(event.arguments));
  browser.executeJavaScript("window.skyrimPlatform.sendMessage('yay')");
});
```
