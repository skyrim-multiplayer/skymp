## Add consoleMessage event

Called each time the game prints something to the console, including calls to `printConsole`.

Note: The message text can contain any characters, including `'` `"` `\`.
Before sending the text to the browser using "browser.executeJavaScript", it should be escaped.

```typescript
import { on, browser } from "skyrimPlatform";

const htmlEscapes: Record<string, string> = {
  '"': '\\"',
  "'": "\\'",
  '\\': '\\\\',
  '<': '\\<',
  '>': '\\>'
};

const htmlEscaper = /[&<>"'\\\/]/g;

// On every print to the game console, console.log it to the browser
on('consoleMessage', (e) => {
  const msg = e.message.replace(htmlEscaper, (match) => htmlEscapes[match]);
  browser.executeJavaScript('console.log("' + msg + '")');
});
```
