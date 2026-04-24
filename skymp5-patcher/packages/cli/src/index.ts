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
import type { PatchResult } from "@skymp5-patcher/core";

const program = new Command();

program
  .name("skymp5-patcher")
  .description("Patches SkyMP TypeScript sources via AST transformations before building")
  .version("1.0.0");

program
  .command("run")
  .description(
    "Copy --src to --tmp, apply all *.patch.ts from --patches, then esbuild to --out"
  )
  .requiredOption("--src <path>", "Path to the TypeScript source root (e.g. skymp5-server/ts)")
  .requiredOption(
    "--patches <path>",
    "Path to directory containing *.patch.ts patch files"
  )
  .option(
    "--tmp <path>",
    "Temp directory for the patched source copy",
    "./tmp/ts"
  )
  .option(
    "--out <path>",
    "Final esbuild output file path",
    "../../build/dist/server/dist_back/skymp5-server.js"
  )
  .action(
    async (options: { src: string; patches: string; tmp: string; out: string }) => {
      const srcDir = path.resolve(options.src);
      const patchesDir = path.resolve(options.patches);
      const outDir = path.resolve(options.tmp);
      const buildOutFile = path.resolve(options.out);

      // Validate inputs.
      if (!fs.existsSync(srcDir)) {
        console.error(`[patcher] ERROR: --src path does not exist: ${srcDir}`);
        process.exit(1);
      }
      if (!fs.existsSync(patchesDir)) {
        console.error(`[patcher] ERROR: --patches path does not exist: ${patchesDir}`);
        process.exit(1);
      }

      console.log("[patcher] Starting patch run");
      console.log(`  src:     ${srcDir}`);
      console.log(`  patches: ${patchesDir}`);
      console.log(`  tmp:     ${outDir}`);
      console.log(`  out:     ${buildOutFile}`);

      const runner = new PatchRunner();
      let results: PatchResult[];

      try {
        results = await runner.run({ srcDir, patchesDir, outDir, buildOutFile });
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
