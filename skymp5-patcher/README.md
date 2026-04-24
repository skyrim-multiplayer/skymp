# skymp5-patcher

A pre-build TypeScript AST patching framework for SkyMP. Before a package is compiled, this tool copies its source files to a temp directory, applies patches declared via decorators, then runs the appropriate build tool on the patched output.

Supports two targets:
- **server** — patches `skymp5-server/ts/`, bundles with esbuild, outputs `skymp5-server.js`
- **client** — patches `skymp5-client/src/`, bundles with webpack, outputs `skymp5-client.js`

The concept is similar to HarmonyPatch in C#: patch classes declare what to inject and where, without touching the original source files.

---

## How it works

### Pipeline

```
skymp5-server/ts/               (original, never modified)
       |
       | fs.cpSync
       v
skymp5-patcher/tmp/server/      (temp copy)
       |
       | ts-morph AST patches applied in-place
       v
skymp5-patcher/tmp/server/      (patched copy)
       |
       | esbuild (same flags as skymp5-server build-ts)
       v
build/dist/server/dist_back/skymp5-server.js


skymp5-client/src/              (original, never modified)
       |
       | fs.cpSync
       v
skymp5-patcher/tmp/client/      (temp copy)
       |
       | ts-morph AST patches applied in-place
       v
skymp5-patcher/tmp/client/      (patched copy)
       |
       | webpack (same config as skymp5-client webpack.config.js)
       v
build/dist/client/Data/Platform/Plugins/skymp5-client.js
```

1. The CLI discovers all `*.patch.ts` files in the patches directory.
2. Each patch file is `require()`'d — this triggers the `@SkyPatch` class decorator, which registers the patch in a global `PatchRegistry`.
3. `PatchRegistry.resolveAll()` reads each patch class's method bodies from the TypeScript AST using ts-morph (to extract the exact source text of prefix/postfix methods).
4. `AstEngine` opens the copied source files in a second ts-morph project, finds the target class and method, and rewrites the method body to inject the patch code.
5. The modified files are saved to disk, then the appropriate bundler runs on them.

Patches that target server files are automatically skipped when running in client mode and vice versa — the patch status will show `[SKIPPED]` with "source file not found". This means a single patches directory can contain both server and client patches and each run only applies the relevant ones.

### Decorator API

Patch classes use three method decorators and one class decorator:

**`@SkyPatch(target)`** — class decorator. Declares which file, class, and method this patch targets. The `file` is relative to the source root (`skymp5-server/ts/` for server, `skymp5-client/src/` for client).

**`@Prefix()`** — marks a static method to run before the original. Return `false` to skip the original entirely, `true` to let it run.

**`@Postfix()`** — marks a static method to run after the original. Receives `__instance` (the `this` value) and `__result` (the return value).

**`@Transpiler()`** — marks a static method that receives the raw ts-morph `MethodDeclaration` for direct AST manipulation. Runs before prefix/postfix injection.

### What the injected code looks like

Given this original method:

```typescript
class Login {
  async initAsync(ctx: SystemContext): Promise<void> {
    await this.doSetup(ctx);
  }
}
```

After a prefix + postfix patch, the temp copy becomes:

```typescript
class Login {
  async initAsync(ctx: SystemContext): Promise<void> {
    const __instance = this;
    const __prefixResult = (function(__instance: any, ctx: SystemContext): boolean {
      console.log("before");
      return true;
    })(__instance, ctx);
    if (__prefixResult === false) { return; }
    await this.doSetup(ctx);
    const __result = undefined;
    (function(__instance: any, ctx: SystemContext, __result: void): void {
      console.log("after");
    })(__instance, ctx, __result);
  }
}
```

The original source file is never touched. The injection only exists in `tmp/`.

---

## Project structure

```
skymp5-patcher/
  packages/
    core/               @skymp5-patcher/core
      src/
        types.ts        BuildTarget, SkyPatchTarget, PatchInfo, PatchResult, RunnerOptions
        decorators.ts   @SkyPatch, @Prefix, @Postfix, @Transpiler
        registry.ts     PatchRegistry singleton
        ast-engine.ts   AstEngine — ts-morph based method body rewriter
        runner.ts       PatchRunner — copy, patch, then esbuild or webpack
        index.ts        public API exports
    cli/                @skymp5-patcher/cli
      src/
        index.ts        CLI entry point (commander)
  examples/
    log-connect.patch.ts    server patch: Login.initAsync
    log-auth.patch.ts       client patch: AuthService.onAuthNeeded
  tmp/                  gitignored — patched source copies live here during a run
```

---

## Writing a patch

Create a file named `something.patch.ts` anywhere in your patches directory. A single patch class targets one specific method. A single file can contain multiple patch classes.

```typescript
import "reflect-metadata";
import { SkyPatch, Prefix, Postfix, Transpiler } from "@skymp5-patcher/core";
import type { MethodDeclaration } from "ts-morph";

// Server patch — file path relative to skymp5-server/ts/
@SkyPatch({
  file: "systems/login.ts",
  class: "Login",
  method: "initAsync",
})
class MyServerPatch {

  // Runs before Login.initAsync.
  // Return false to skip the original method.
  @Prefix()
  static prefix(__instance: any, ctx: any): boolean {
    console.log("before initAsync");
    return true;
  }

  // Runs after Login.initAsync.
  // __result is the return value (undefined for void methods).
  @Postfix()
  static postfix(__instance: any, ctx: any, __result: void): void {
    console.log("after initAsync");
  }

  // Direct AST access via ts-morph. Runs before prefix/postfix.
  @Transpiler()
  static transpiler(method: MethodDeclaration): void {
    method.getBodyOrThrow().insertStatements(0, '// injected by transpiler');
  }
}

// Client patch — file path relative to skymp5-client/src/
@SkyPatch({
  file: "services/services/authService.ts",
  class: "AuthService",
  method: "onAuthNeeded",
})
class MyClientPatch {
  @Prefix()
  static prefix(__instance: any, e: any): boolean {
    console.log("auth needed event:", e);
    return true;
  }
}
```

Parameter naming convention:
- `__instance` — always maps to `this` of the target method
- `__result` — always maps to the return value (postfix only)
- All other parameters are matched positionally to the original method's parameters

When a patch targets a server file but the run is `--target client` (or vice versa), the patch is silently skipped with `[SKIPPED]` status. You can safely keep all patches in one directory.

---

## CLI

```
skymp5-patcher run [options]

  --target <server|client>  Which package to patch and build        [required]
  --patches <path>          Directory containing *.patch.ts files   [required]
  --src <path>              Override the source root to patch
  --tmp <path>              Override the temp directory for the patched copy
  --out <path>              Override the final bundled JS output path
```

Default paths when not overridden:

| | server | client |
|---|---|---|
| `--src` | `../skymp5-server/ts` | `../skymp5-client/src` |
| `--tmp` | `./tmp/server` | `./tmp/client` |
| `--out` | `../../build/dist/server/dist_back/skymp5-server.js` | `../../build/dist/client/Data/Platform/Plugins/skymp5-client.js` |

Examples:

```bash
# Patch and build the server (uses all defaults)
node packages/cli/dist/index.js run --target server --patches ./examples

# Patch and build the client (uses all defaults)
node packages/cli/dist/index.js run --target client --patches ./examples

# Override the output path
node packages/cli/dist/index.js run --target server --patches ./my-patches --out ./dist/server.js
```

Output:

```
[patcher] Starting patch run (target: server)
  src:     .../skymp5-server/ts
  patches: .../examples
  tmp:     .../tmp/server
  out:     .../build/dist/server/dist_back/skymp5-server.js
[patcher:server] Copying ...
[patcher:server] Found 2 patch file(s)
[patcher:server] Loading patch: log-connect.patch.ts
[patcher:server] Loading patch: log-auth.patch.ts
[patcher:server] Running esbuild → ...
  [APPLIED]  systems/login.ts :: Login.initAsync
  [SKIPPED]  services/services/authService.ts :: AuthService.onAuthNeeded — Source file not found in project: ...
[patcher] Done. 1 applied, 1 skipped, 0 failed.
```

Per-patch status:
- `[APPLIED]` — patch was found and injected successfully
- `[SKIPPED]` — target file, class, or method was not found (not a fatal error; expected when a client patch runs against --target server or vice versa)
- `[FAILED]` — unexpected error during patch application (exits with code 1)

---

## Building

```bash
cd skymp5-patcher
npm install
npm run build
```

This compiles both `packages/core` and `packages/cli` to their respective `dist/` directories.

---

## Dependencies

- `ts-morph` — TypeScript AST manipulation (reading patch method bodies, rewriting target methods)
- `reflect-metadata` — stores decorator metadata on patch class constructors
- `ts-node` — executes `.patch.ts` files at runtime without a separate compile step
- `commander` — CLI argument parsing
- esbuild is not a direct dependency — the runner uses the copy installed in `skymp5-server/node_modules`
- webpack and ts-loader are not direct dependencies — the runner uses the copies installed in `skymp5-client/node_modules`
