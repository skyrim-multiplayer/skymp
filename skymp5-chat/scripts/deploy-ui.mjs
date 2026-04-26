import { copyFileSync, existsSync, mkdirSync } from "node:fs";
import { resolve } from "node:path";

const defaultTarget = resolve("../build/dist/client/Data/Platform/UI");
const targetDir = resolve(process.argv[2] || defaultTarget);
const distDir = resolve("dist");
const files = ["index.html", "build.js", "style.css"];

if (!existsSync(resolve(distDir, "index.html"))) {
  throw new Error("dist is missing. Run npm run build first.");
}

mkdirSync(targetDir, { recursive: true });

for (const file of files) {
  copyFileSync(resolve(distDir, file), resolve(targetDir, file));
  console.log(`deployed ${file} -> ${targetDir}`);
}
