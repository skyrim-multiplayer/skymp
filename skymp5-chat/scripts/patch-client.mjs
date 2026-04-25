import { spawnSync } from "node:child_process";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const chatDir = resolve(scriptDir, "..");
const repoDir = resolve(chatDir, "..");
const patcherDir = resolve(repoDir, "skymp5-patcher");
const cliFile = resolve(patcherDir, "packages/cli/dist/index.js");

const args = [
  cliFile,
  "run",
  "--target",
  "client",
  "--src",
  resolve(repoDir, "skymp5-client/src"),
  "--tmp",
  resolve(patcherDir, "tmp/client"),
  "--out",
  resolve(repoDir, "build/dist/client/Data/Platform/Plugins/skymp5-client.js"),
  "--patches",
  resolve(chatDir, "src/patches"),
];

const result = spawnSync(process.execPath, args, {
  cwd: patcherDir,
  stdio: "inherit",
});

process.exit(result.status ?? 1);
