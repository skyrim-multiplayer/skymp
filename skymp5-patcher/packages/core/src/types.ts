import type { MethodDeclaration } from "ts-morph";

/**
 * Which SkyMP package is being patched.
 * Determines which build tool is invoked after patching:
 *   - "server" → esbuild (matches skymp5-server build-ts script)
 *   - "client" → webpack (matches skymp5-client webpack config)
 */
export type BuildTarget = "server" | "client";

/**
 * Identifies which class method in which file should be patched.
 * The `file` path is relative to the source root (srcDir).
 */
export interface SkyPatchTarget {
  /** Relative path from the src root, e.g. "systems/login.ts" or "services/authService.ts" */
  file: string;
  /** Class name, e.g. "Login" */
  class: string;
  /** Method name, e.g. "initAsync" */
  method: string;
}

/**
 * Prefix patch: injected before the original method body.
 * Returning false from the prefix function skips the original method.
 */
export interface PrefixPatchInfo {
  kind: "prefix";
  /** Statements from the prefix static method body */
  bodyText: string;
  /** Parameter names from the prefix method signature */
  paramNames: string[];
  /** Parameter type annotations, parallel to paramNames */
  paramTypes: string[];
  /** Return type annotation (should be boolean) */
  returnType: string;
}

/**
 * Postfix patch: injected after the original method body.
 * Receives __result (the return value) and __instance (this).
 */
export interface PostfixPatchInfo {
  kind: "postfix";
  /** Statements from the postfix static method body */
  bodyText: string;
  /** Parameter names from the postfix method signature */
  paramNames: string[];
  /** Parameter type annotations, parallel to paramNames */
  paramTypes: string[];
}

/**
 * Transpiler patch: receives the raw ts-morph MethodDeclaration for
 * direct AST manipulation. Runs before prefix/postfix injection.
 */
export interface TranspilerPatchInfo {
  kind: "transpiler";
  /** The actual static transpiler function reference */
  fn: (method: MethodDeclaration) => void;
}

export type PatchInfo = PrefixPatchInfo | PostfixPatchInfo | TranspilerPatchInfo;

/**
 * A fully resolved patch: target location plus all patch fragments to apply.
 */
export interface RegisteredPatch {
  target: SkyPatchTarget;
  patches: PatchInfo[];
}

export type PatchStatus = "applied" | "skipped" | "failed";

export interface PatchResult {
  target: SkyPatchTarget;
  status: PatchStatus;
  /** Set when status is "skipped" or "failed" */
  error?: Error;
}

export interface RunnerOptions {
  /**
   * Which SkyMP package is being patched.
   * Determines the build tool used after AST patching.
   */
  buildTarget: BuildTarget;
  /** Absolute path to the source root to copy (e.g. skymp5-server/ts or skymp5-client/src) */
  srcDir: string;
  /** Absolute path to directory containing *.patch.ts files */
  patchesDir: string;
  /** Absolute path to write the patched temp copy */
  outDir: string;
  /** Absolute path for the final bundled JS output */
  buildOutFile: string;
}
