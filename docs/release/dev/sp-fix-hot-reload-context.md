## Hot reload strictly in non-game context

Made hot reload to happen strictly in a non-game context.

```ts
import { Game } from "skyrimPlatform";

// This function call worked in 50% cases of hot reload:
Game.getPlayer();

// Now always throws as expected
```
