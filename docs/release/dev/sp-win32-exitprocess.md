## Add `win32.exitProcess` method

This method allows you to close the game by calling `std::exit` under the hood:

```typescript
import { once, win32 } from "skyrimPlatform";

once("activate", () => {
  // Exit game on any activation
  win32.exitProcess();
});
```
