import { copyFileSync, mkdirSync } from "node:fs";
import { resolve } from "node:path";

const sourceDir = resolve("src/client");
const distDir = resolve("dist");
const files = ["index.html", "build.js", "style.css"];

mkdirSync(distDir, { recursive: true });

for (const file of files) {
  copyFileSync(resolve(sourceDir, file), resolve(distDir, file));
  console.log(`built ${file}`);
}
