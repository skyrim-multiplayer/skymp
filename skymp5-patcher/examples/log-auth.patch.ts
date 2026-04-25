import "reflect-metadata";
import { SkyPatch, Prefix } from "@skymp5-patcher/core";

/**
 * Example client patch: intercepts AuthService.onAuthNeeded
 * in skymp5-client/src/services/services/authService.ts.
 *
 * Usage:
 *   node packages/cli/dist/index.js run --target client --patches ./examples
 */
@SkyPatch({
  file: "services/services/authService.ts",
  class: "AuthService",
  method: "onAuthNeeded",
})
class LogAuthPatch {
  @Prefix()
  static prefix(__instance: any, e: any): boolean {
    console.log("[LogAuthPatch] AuthService.onAuthNeeded called, event:", e);
    return true;
  }
}
