import { copyFileSync, mkdirSync, rmSync } from "node:fs";
import { dirname, resolve } from "node:path";

const source = resolve("src/server/chat.ts");
const target = resolve("../frostfall-gamemode/src/systems/communication/chat.ts");
const legacyDistFolder = resolve("../build/dist/server/skymp5-gamemode");

mkdirSync(dirname(target), { recursive: true });
copyFileSync(source, target);
console.log(`deployed ${source} -> ${target}`);

rmSync(legacyDistFolder, { recursive: true, force: true });
