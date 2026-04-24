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
 *   8. Run esbuild to compile the patched output to buildOutFile.
 */
export class PatchRunner {
  async run(options: RunnerOptions): Promise<PatchResult[]> {
    const { srcDir, patchesDir, outDir, buildOutFile } = options;

    // Step 1: Copy source tree to temp dir.
    console.log(`[patcher] Copying ${srcDir} → ${outDir}`);
    fs.rmSync(outDir, { recursive: true, force: true });
    fs.cpSync(srcDir, outDir, { recursive: true });

    // Step 2: Discover patch files.
    const patchFiles = discoverPatchFiles(patchesDir);
    if (patchFiles.length === 0) {
      console.warn(`[patcher] No *.patch.ts files found in: ${patchesDir}`);
      return [];
    }
    console.log(`[patcher] Found ${patchFiles.length} patch file(s)`);

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
      console.log(`[patcher] Loading patch: ${path.basename(patchFile)}`);
      // eslint-disable-next-line @typescript-eslint/no-require-imports
      require(patchFile);
    }

    // Step 5: Resolve patches.
    const registry = PatchRegistry.getInstance();
    const resolvedPatches = registry.resolveAll(patchProject, patchSourceFiles);

    if (resolvedPatches.length === 0) {
      console.warn("[patcher] No patches registered after loading patch files.");
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

    // Step 8: Run esbuild — same command as skymp5-server's build-ts script,
    // but with outDir/index.ts as the entry point.
    const entryPoint = path.join(outDir, "index.ts");
    if (!fs.existsSync(entryPoint)) {
      throw new Error(
        `esbuild entry point not found: ${entryPoint}. ` +
        `Ensure the src directory contains index.ts at its root.`
      );
    }

    // Ensure the output directory exists.
    fs.mkdirSync(path.dirname(buildOutFile), { recursive: true });

    // Use skymp5-server's installed esbuild via its JS API.
    // The JS API supports 'nodePaths', which lets us resolve the server's
    // node_modules even though the entry file lives in our temp directory.
    // The CLI flag --node-paths does not exist; the JS API is required.
    const serverDir = path.dirname(srcDir);
    const serverNodeModules = path.join(serverDir, "node_modules");
    const esbuildPkg = path.join(serverNodeModules, "esbuild");

    type EsbuildModule = {
      build: (opts: Record<string, unknown>) => Promise<void>;
    };

    let esbuild: EsbuildModule;
    if (fs.existsSync(esbuildPkg)) {
      // eslint-disable-next-line @typescript-eslint/no-require-imports
      esbuild = require(esbuildPkg) as EsbuildModule;
    } else {
      // Fallback: try the workspace's own esbuild if present.
      // eslint-disable-next-line @typescript-eslint/no-require-imports
      esbuild = require("esbuild") as EsbuildModule;
    }

    console.log(`[patcher] Running esbuild → ${buildOutFile}`);
    await esbuild.build({
      entryPoints: [entryPoint],
      loader: { ".node": "copy" },
      bundle: true,
      platform: "node",
      target: "node16",
      keepNames: true,
      minify: true,
      sourcemap: true,
      nodePaths: [serverNodeModules],
      outfile: buildOutFile,
    });

    return results;
  }
}

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
