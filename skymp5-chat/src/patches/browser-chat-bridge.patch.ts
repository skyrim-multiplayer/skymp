import "reflect-metadata";
import { SkyPatch, Prefix } from "@skymp5-patcher/core";

/**
 * Legacy compatibility patch.
 *
 * BrowserService now forwards allow-listed browser messages directly to the
 * server as CustomPacket messages. This patch is intentionally a no-op so old
 * patch scripts can still run without double-sending chat packets.
 */
@SkyPatch({
  file: "services/services/browserService.ts",
  class: "BrowserService",
  method: "onBrowserMessage",
})
export class BrowserChatBridgePatch {
  @Prefix()
  static prefix(): boolean {
    return true;
  }
}
