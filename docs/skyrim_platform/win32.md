# Win32 shell calls

SkyrimPlatform provides support for opening links and more in os default browser by windows shell calls.

```typescript
import { win32 } from "skyrimPlatform";
let url = "https://google.com"; // URL should start with prefix https://
win32.loadUrl(url);
```

```typescript
import { win32 } from "skyrimPlatform";
win32.exitProcess() //terminates the process gracefully;
```
