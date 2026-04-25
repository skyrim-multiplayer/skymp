#!/usr/bin/env node

// ts-node must be registered before any patch files are require()'d.
// Using require() style (not import) guarantees initialization order in
// CommonJS output — static imports are hoisted, require() calls are not.
require("ts-node").register({
  transpileOnly: true, // skip type-checking patch files for speed
  compilerOptions: {
    module: "commonjs",
    experimentalDecorators: true,
    emitDecoratorMetadata: true,
    esModuleInterop: true,
    skipLibCheck: true,
    strict: false,
  },
});

// Must be loaded after ts-node so patch files can also import it.
require("reflect-metadata");

// eslint-disable-next-line @typescript-eslint/no-require-imports
const { PatchRunner } = require("@skymp5-patcher/core") as typeof import("@skymp5-patcher/core");
// eslint-disable-next-line @typescript-eslint/no-require-imports
const { Command } = require("commander") as typeof import("commander");

import * as path from "path";
import * as fs from "fs";
import type { PatchResult, BuildTarget } from "@skymp5-patcher/core";

// Default paths relative to the CLI working directory (skymp5-patcher/).
const DEFAULTS: Record<BuildTarget, { src: string; tmp: string; out: string }> = {
  server: {
    src: "../skymp5-server/ts",
    tmp: "./tmp/server",
    out: "../../build/dist/server/dist_back/skymp5-server.js",
  },
  client: {
    src: "../skymp5-client/src",
    tmp: "./tmp/client",
    out: "../../build/dist/client/Data/Platform/Plugins/skymp5-client.js",
  },
};

const program = new Command();

program
  .name("skymp5-patcher")
  .description("Patches SkyMP TypeScript sources via AST transformations before building")
  .version("1.0.0");

program
  .command("run")
  .description(
    "Copy source to a temp dir, apply *.patch.ts patches, then build to the output path.\n" +
    "Defaults for --src, --tmp, and --out are set automatically based on --target."
  )
  .requiredOption(
    "--target <server|client>",
    "Which SkyMP package to patch: 'server' (esbuild) or 'client' (webpack)"
  )
  .requiredOption(
    "--patches <path>",
    "Path to directory containing *.patch.ts patch files"
  )
  .option("--src <path>", "Override the source root to patch")
  .option("--tmp <path>", "Override the temp directory for the patched copy")
  .option("--out <path>", "Override the final bundled JS output path")
  .action(
    async (options: {
      target: string;
      patches: string;
      src?: string;
      tmp?: string;
      out?: string;
    }) => {
      // Validate target.
      if (options.target !== "server" && options.target !== "client") {
        console.error(
          `[patcher] ERROR: --target must be "server" or "client", got: "${options.target}"`
        );
        process.exit(1);
      }
      const buildTarget = options.target as BuildTarget;
      const defaults = DEFAULTS[buildTarget];

      const srcDir = path.resolve(options.src ?? defaults.src);
      const patchesDir = path.resolve(options.patches);
      const outDir = path.resolve(options.tmp ?? defaults.tmp);
      const buildOutFile = path.resolve(options.out ?? defaults.out);

      // Validate inputs.
      if (!fs.existsSync(srcDir)) {
        console.error(`[patcher] ERROR: source path does not exist: ${srcDir}`);
        console.error(`  (defaulted from --target ${buildTarget}, override with --src)`);
        process.exit(1);
      }
      if (!fs.existsSync(patchesDir)) {
        console.error(`[patcher] ERROR: --patches path does not exist: ${patchesDir}`);
        process.exit(1);
      }

      console.log(`[patcher] Starting patch run (target: ${buildTarget})`);
      console.log(`  src:     ${srcDir}`);
      console.log(`  patches: ${patchesDir}`);
      console.log(`  tmp:     ${outDir}`);
      console.log(`  out:     ${buildOutFile}`);

      const runner = new PatchRunner();
      let results: PatchResult[];

      try {
        results = await runner.run({ buildTarget, srcDir, patchesDir, outDir, buildOutFile });
      } catch (err) {
        console.error(`[patcher] FATAL: ${err}`);
        process.exit(1);
      }

      if (results.length === 0) {
        console.log("[patcher] No patches were applied.");
        return;
      }

      // Print per-patch status.
      let hasFailure = false;
      for (const result of results) {
        const loc = `${result.target.file} :: ${result.target.class}.${result.target.method}`;
        switch (result.status) {
          case "applied":
            console.log(`  [APPLIED]  ${loc}`);
            break;
          case "skipped":
            console.warn(
              `  [SKIPPED]  ${loc}${result.error ? ` — ${result.error.message}` : ""}`
            );
            break;
          case "failed":
            console.error(
              `  [FAILED]   ${loc}${result.error ? ` — ${result.error.message}` : ""}`
            );
            hasFailure = true;
            break;
        }
      }

      const applied = results.filter((r) => r.status === "applied").length;
      const skipped = results.filter((r) => r.status === "skipped").length;
      const failed = results.filter((r) => r.status === "failed").length;
      console.log(
        `[patcher] Done. ${applied} applied, ${skipped} skipped, ${failed} failed.`
      );

      if (hasFailure) {
        process.exit(1);
      }
    }
  );

program.parse(process.argv);
