# skymp5-patcher

A pre-build TypeScript AST patching framework for SkyMP. Before the server is compiled, this tool copies the source files to a temp directory, applies patches declared via decorators, then runs esbuild on the patched output — producing the same `skymp5-server.js` that the normal build would, but with injected code.

The concept is similar to HarmonyPatch in C#: patch classes declare what to inject and where, without touching the original source files.

---

## How it works

### Pipeline

```
skymp5-server/ts/           (original, never modified)
        |
        | fs.cpSync
        v
skymp5-patcher/tmp/ts/      (temp copy)
        |
        | ts-morph AST patches applied in-place
        v
skymp5-patcher/tmp/ts/      (patched copy)
        |
        | esbuild (same flags as skymp5-server build-ts)
        v
build/dist/server/dist_back/skymp5-server.js
```

1. The CLI discovers all `*.patch.ts` files in the patches directory.
2. Each patch file is `require()`'d — this triggers the `@SkyPatch` class decorator, which registers the patch in a global `PatchRegistry`.
3. `PatchRegistry.resolveAll()` reads each patch class's method bodies from the TypeScript AST using ts-morph (to extract the exact source text of prefix/postfix methods).
4. `AstEngine` opens the copied source files in a second ts-morph project, finds the target class and method, and rewrites the method body to inject the patch code.
5. The modified files are saved to disk, then esbuild bundles them into the final output.

### Decorator API

Patch classes use three method decorators and one class decorator:

**`@SkyPatch(target)`** — class decorator. Declares which file, class, and method this patch targets.

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
        types.ts        interfaces: SkyPatchTarget, PatchInfo, PatchResult, RunnerOptions
        decorators.ts   @SkyPatch, @Prefix, @Postfix, @Transpiler
        registry.ts     PatchRegistry singleton
        ast-engine.ts   AstEngine — ts-morph based method body rewriter
        runner.ts       PatchRunner — orchestrates copy, patch, build
        index.ts        public API exports
    cli/                @skymp5-patcher/cli
      src/
        index.ts        CLI entry point (commander)
  examples/
    log-connect.patch.ts
  tmp/                  gitignored — patched source copy lives here during a run
```

---

## Writing a patch

Create a file named `something.patch.ts` anywhere in your patches directory.

```typescript
import "reflect-metadata";
import { SkyPatch, Prefix, Postfix, Transpiler } from "@skymp5-patcher/core";
import type { MethodDeclaration } from "ts-morph";

@SkyPatch({
  file: "systems/login.ts",   // relative to the --src root
  class: "Login",
  method: "initAsync",
})
class MyPatch {

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
```

Parameter naming convention:
- `__instance` — always maps to `this` of the target method
- `__result` — always maps to the return value (postfix only)
- All other parameters are matched positionally to the original method's parameters

---

## CLI

```
skymp5-patcher run [options]

  --src <path>      Source root to patch (e.g. ../skymp5-server/ts)   [required]
  --patches <path>  Directory containing *.patch.ts files             [required]
  --tmp <path>      Temp directory for the patched copy      [default: ./tmp/ts]
  --out <path>      Final JS output path
                    [default: ../../build/dist/server/dist_back/skymp5-server.js]
```

Example:

```bash
node packages/cli/dist/index.js run \
  --src ../skymp5-server/ts \
  --patches ./examples \
  --tmp ./tmp/ts \
  --out ../build/dist/server/dist_back/skymp5-server.js
```

Output:

```
[patcher] Starting patch run
  src:     .../skymp5-server/ts
  patches: .../examples
  tmp:     .../tmp/ts
  out:     .../build/dist/server/dist_back/skymp5-server.js
[patcher] Copying ...
[patcher] Found 1 patch file(s)
[patcher] Loading patch: log-connect.patch.ts
[patcher] Running esbuild → ...
  [APPLIED]  systems/login.ts :: Login.initAsync
[patcher] Done. 1 applied, 0 skipped, 0 failed.
```

Per-patch status:
- `[APPLIED]` — patch was found and injected successfully
- `[SKIPPED]` — target file, class, or method was not found (not a fatal error)
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
- esbuild is not a direct dependency — the runner uses the copy already installed in `skymp5-server/node_modules`
