import * as fs from "fs";
import * as path from "path";
import {
  Project,
  IndentationText,
  NewLineKind,
  QuoteKind,
  ScriptTarget,
  ModuleKind,
} from "ts-morph";
import type { RunnerOptions, PatchResult } from "./types";
import { PatchRegistry } from "./registry";
import { AstEngine } from "./ast-engine";

/**
 * Orchestrates the full patch pipeline:
 *
 *   1. Copy srcDir → outDir (preserving the directory tree).
 *   2. Discover *.patch.ts files in patchesDir.
 *   3. Build a ts-morph Project for the patch source files (for body extraction).
 *   4. require() each patch file — triggers @SkyPatch decorator registration.
 *   5. Resolve all registered patches via PatchRegistry.resolveAll().
 *   6. Build a ts-morph Project over the copied outDir files.
 *   7. Apply all patches via AstEngine.
 *   8. Bundle the patched output:
 *        server → esbuild (same flags as skymp5-server build-ts)
 *        client → webpack (same config as skymp5-client webpack.config.js)
 */
export class PatchRunner {
  async run(options: RunnerOptions): Promise<PatchResult[]> {
    const { buildTarget, srcDir, patchesDir, outDir, buildOutFile } = options;

    // Step 1: Copy source tree to temp dir.
    console.log(`[patcher:${buildTarget}] Copying ${srcDir} → ${outDir}`);
    fs.rmSync(outDir, { recursive: true, force: true });
    fs.cpSync(srcDir, outDir, { recursive: true });

    // Step 2: Discover patch files.
    const patchFiles = discoverPatchFiles(patchesDir);
    if (patchFiles.length === 0) {
      console.warn(`[patcher:${buildTarget}] No *.patch.ts files found in: ${patchesDir}`);
      return [];
    }
    console.log(`[patcher:${buildTarget}] Found ${patchFiles.length} patch file(s)`);

    // Step 3: Build ts-morph Project for patch source files (used by resolveAll).
    const patchProject = new Project({
      compilerOptions: {
        experimentalDecorators: true,
        emitDecoratorMetadata: true,
        strict: true,
        target: ScriptTarget.ESNext,
        module: ModuleKind.CommonJS,
        esModuleInterop: true,
        skipLibCheck: true,
      },
      manipulationSettings: {
        indentationText: IndentationText.TwoSpaces,
        newLineKind: NewLineKind.LineFeed,
        quoteKind: QuoteKind.Double,
        usePrefixAndSuffixTextForRename: false,
      },
      skipAddingFilesFromTsConfig: true,
    });

    const patchSourceFiles = patchFiles.map((fp) =>
      patchProject.addSourceFileAtPath(fp)
    );

    // Step 4: Load patch files — triggers @SkyPatch registration as side effects.
    // ts-node must already be registered by the CLI before this point.
    for (const patchFile of patchFiles) {
      console.log(`[patcher:${buildTarget}] Loading patch: ${path.basename(patchFile)}`);
      // eslint-disable-next-line @typescript-eslint/no-require-imports
      require(patchFile);
    }

    // Step 5: Resolve patches.
    const registry = PatchRegistry.getInstance();
    const resolvedPatches = registry.resolveAll(patchProject, patchSourceFiles);

    if (resolvedPatches.length === 0) {
      console.warn(`[patcher:${buildTarget}] No patches registered after loading patch files.`);
      return [];
    }

    // Step 6: Build ts-morph Project over the copied output files.
    const outProject = new Project({
      compilerOptions: {
        strict: false,
        skipLibCheck: true,
        allowJs: true,
        noEmit: true,
        experimentalDecorators: true,
        target: ScriptTarget.ES2022,
        module: ModuleKind.CommonJS,
      },
      manipulationSettings: {
        indentationText: IndentationText.TwoSpaces,
        newLineKind: NewLineKind.LineFeed,
        quoteKind: QuoteKind.Double,
        usePrefixAndSuffixTextForRename: false,
      },
      skipAddingFilesFromTsConfig: true,
    });

    addTsFilesRecursive(outProject, outDir);

    // Step 7: Apply all patches.
    const engine = new AstEngine(outProject);
    const results: PatchResult[] = [];

    for (const patch of resolvedPatches) {
      const result = engine.applyPatch(patch);
      results.push(result);
    }

    // Step 8: Bundle the patched output using the appropriate build tool.
    fs.mkdirSync(path.dirname(buildOutFile), { recursive: true });

    if (buildTarget === "server") {
      await buildServer(srcDir, outDir, buildOutFile);
    } else {
      await buildClient(srcDir, outDir, buildOutFile);
    }

    return results;
  }
}

// ── Server build (esbuild) ────────────────────────────────────────────────────

/**
 * Bundles the patched server source using esbuild.
 *
 * Mirrors the skymp5-server build-ts script:
 *   esbuild ts/index.ts --loader:.node=copy --bundle --platform=node
 *     --target=node16 --keep-names --minify --sourcemap
 *     --outfile=../build/dist/server/dist_back/skymp5-server.js
 *
 * Uses the esbuild JS API (not CLI) so that nodePaths can be set,
 * allowing the temp dir entry file to resolve packages from skymp5-server/node_modules.
 */
async function buildServer(
  srcDir: string,
  outDir: string,
  buildOutFile: string
): Promise<void> {
  const entryPoint = path.join(outDir, "index.ts");
  if (!fs.existsSync(entryPoint)) {
    throw new Error(
      `esbuild entry point not found: ${entryPoint}. ` +
      `Ensure --src points to a directory that contains index.ts at its root.`
    );
  }

  // srcDir = skymp5-server/ts/ → serverDir = skymp5-server/
  const serverDir = path.dirname(srcDir);
  const serverNodeModules = path.join(serverDir, "node_modules");
  const esbuildPkg = path.join(serverNodeModules, "esbuild");

  type EsbuildModule = { build: (opts: Record<string, unknown>) => Promise<void> };
  let esbuild: EsbuildModule;

  if (fs.existsSync(esbuildPkg)) {
    // eslint-disable-next-line @typescript-eslint/no-require-imports
    esbuild = require(esbuildPkg) as EsbuildModule;
  } else {
    throw new Error(
      `esbuild not found at ${esbuildPkg}. ` +
      `Run npm install inside skymp5-server/ first.`
    );
  }

  console.log(`[patcher:server] Running esbuild → ${buildOutFile}`);
  await esbuild.build({
    entryPoints: [entryPoint],
    loader: { ".node": "copy" },
    bundle: true,
    platform: "node",
    target: "node16",
    keepNames: true,
    minify: true,
    sourcemap: true,
    // nodePaths lets esbuild find skymp5-server's packages even though the
    // entry file lives in the temp directory.
    nodePaths: [serverNodeModules],
    outfile: buildOutFile,
  });
}

// ── Client build (webpack) ────────────────────────────────────────────────────

/**
 * Bundles the patched client source using webpack.
 *
 * Mirrors the skymp5-client webpack.config.js configuration:
 *   - target: "node" (Skyrim Platform plugin environment)
 *   - mode: "development" with inline-source-map
 *   - ts-loader for TypeScript compilation
 *   - skyrimPlatform marked as external (not bundled)
 *
 * Uses webpack's JS API so we can override the entry point to the temp dir
 * while still resolving loaders and packages from skymp5-client/node_modules.
 */
async function buildClient(
  srcDir: string,
  outDir: string,
  buildOutFile: string
): Promise<void> {
  const entryPoint = path.join(outDir, "index.ts");
  if (!fs.existsSync(entryPoint)) {
    throw new Error(
      `webpack entry point not found: ${entryPoint}. ` +
      `Ensure --src points to a directory that contains index.ts at its root.`
    );
  }

  // srcDir = skymp5-client/src/ → clientDir = skymp5-client/
  const clientDir = path.dirname(srcDir);
  const clientNodeModules = path.join(clientDir, "node_modules");

  if (!fs.existsSync(clientNodeModules)) {
    throw new Error(
      `skymp5-client node_modules not found at ${clientNodeModules}. ` +
      `Run npm install inside skymp5-client/ first.`
    );
  }

  // eslint-disable-next-line @typescript-eslint/no-require-imports
  const webpack = require(path.join(clientNodeModules, "webpack")) as (
    config: Record<string, unknown>,
    callback: (err: Error | null, stats: { hasErrors(): boolean; toString(): string }) => void
  ) => void;

  // Resolve ts-loader from skymp5-client's node_modules.
  const tsLoaderPath = path.join(clientNodeModules, "ts-loader");
  if (!fs.existsSync(tsLoaderPath)) {
    throw new Error(
      `ts-loader not found at ${tsLoaderPath}. ` +
      `Run npm install inside skymp5-client/ first.`
    );
  }

  // Use skymp5-client's tsconfig for ts-loader, but with transpileOnly so that
  // the tsconfig 'include' list doesn't reject files living in the temp dir.
  const tsconfigPath = path.join(clientDir, "tsconfig.json");

  console.log(`[patcher:client] Running webpack → ${buildOutFile}`);

  const webpackConfig = {
    target: "node",
    mode: "development",
    devtool: "inline-source-map",
    entry: { main: entryPoint },
    output: {
      path: path.dirname(buildOutFile),
      filename: path.basename(buildOutFile),
    },
    resolve: {
      extensions: [".ts", ".tsx", ".js", ".jsx"],
      // Resolve imported modules from skymp5-client's node_modules.
      modules: [clientNodeModules, "node_modules"],
    },
    resolveLoader: {
      // Resolve loaders (ts-loader etc.) from skymp5-client's node_modules.
      modules: [clientNodeModules, "node_modules"],
    },
    externals: {
      // skyrimPlatform is injected at runtime by the Skyrim Platform engine.
      "@skyrim-platform/skyrim-platform": ["skyrimPlatform"],
      skyrimPlatform: ["skyrimPlatform"],
    },
    module: {
      rules: [
        {
          test: /\.tsx?$/,
          loader: tsLoaderPath,
          options: {
            configFile: tsconfigPath,
            // transpileOnly: skip type-checking so ts-loader doesn't reject
            // files outside the tsconfig 'include' list (i.e. the temp dir).
            transpileOnly: true,
          },
        },
      ],
    },
  };

  await new Promise<void>((resolve, reject) => {
    webpack(webpackConfig, (err, stats) => {
      if (err) {
        reject(err);
        return;
      }
      if (stats.hasErrors()) {
        reject(new Error(stats.toString()));
        return;
      }
      resolve();
    });
  });
}

// ── Utility Functions ─────────────────────────────────────────────────────────

/** Finds all *.patch.ts files recursively in a directory. */
function discoverPatchFiles(dir: string): string[] {
  const results: string[] = [];
  walkDir(dir, (filePath) => {
    if (filePath.endsWith(".patch.ts")) {
      results.push(filePath);
    }
  });
  return results;
}

/** Adds all .ts (non-.d.ts) files from dir recursively to a ts-morph Project. */
function addTsFilesRecursive(project: Project, dir: string): void {
  walkDir(dir, (filePath) => {
    if (filePath.endsWith(".ts") && !filePath.endsWith(".d.ts")) {
      project.addSourceFileAtPath(filePath);
    }
  });
}

function walkDir(dir: string, callback: (filePath: string) => void): void {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      walkDir(fullPath, callback);
    } else {
      callback(fullPath);
    }
  }
}
