import "reflect-metadata";
import { SkyPatch, Prefix, Postfix } from "@skymp5-patcher/core";
import type { MethodDeclaration } from "ts-morph";

// Type aliases for the target method's context.
// Use 'any' here since the patch project may not have access to server types.
// For full type safety, import the actual types from skymp5-server if available.
type SystemContext = any;

/**
 * Example patch: intercepts Login.initAsync in skymp5-server/ts/systems/login.ts.
 *
 * - Prefix: logs before initAsync runs, can return false to skip it.
 * - Postfix: logs after initAsync completes.
 *
 * Usage:
 *   node packages/cli/dist/index.js run \
 *     --src ../skymp5-server/ts \
 *     --patches ./examples \
 *     --tmp ./tmp/ts \
 *     --out ../build/dist/server/dist_back/skymp5-server.js
 */
@SkyPatch({
  file: "systems/login.ts",
  class: "Login",
  method: "initAsync",
})
class LogConnectPatch {
  /**
   * Runs before Login.initAsync.
   * Return false to skip the original method.
   * Return true to allow it to run normally.
   */
  @Prefix()
  static prefix(__instance: any, ctx: SystemContext): boolean {
    console.log("[LogConnectPatch] Login.initAsync called, ctx:", ctx);
    // Conditionally skip by returning false:
    // if (someCondition) return false;
    return true;
  }

  /**
   * Runs after Login.initAsync (or after the prefix if original was skipped).
   * __result holds the return value (undefined for void methods).
   */
  @Postfix()
  static postfix(__instance: any, ctx: SystemContext, __result: void): void {
    console.log("[LogConnectPatch] Login.initAsync complete. Result:", __result);
  }
}

/**
 * Example of a Transpiler patch (separate class for clarity).
 * Uncomment to enable direct AST manipulation.
 *
 * @SkyPatch({
 *   file: "systems/login.ts",
 *   class: "Login",
 *   method: "initAsync",
 * })
 * class LogConnectTranspilerPatch {
 *   @Transpiler()
 *   static transpiler(method: MethodDeclaration): void {
 *     // Example: prepend a statement to the method body
 *     const body = method.getBodyOrThrow();
 *     body.insertStatements(0, 'console.log("[transpiler] method start");');
 *   }
 * }
 */
