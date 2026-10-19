/**
 * YACA-TS Voice Provider Adapter
 *
 * Implements the IVoiceProviderAdapter interface for YACA TeamSpeak integration.
 * This is a reference implementation showing how to bridge YacaBridgeQuest
 * Papyrus events to the standardized Voice Activity API.
 *
 * Architecture:
 * 1. YacaBridgeQuest (Papyrus) listens to TeamSpeak YACA plugin events
 * 2. Sends events to server via custom packet (mp.callGamemodeApi)
 * 3. Adapter receives events and converts to VoiceActivityState
 * 4. Voice Activity Manager broadcasts to proximity-aware clients
 */

import { IVoiceProviderAdapter, VoiceActivityState } from "../voiceActivityApi";

export class YacaTeamSpeakAdapter implements IVoiceProviderAdapter {
  readonly providerId = "yaca-ts";
  private voiceActivityCallbacks: Set<(state: VoiceActivityState) => void> = new Set();
  private isInitialized = false;
  private startTime: number = 0;
  private activeSpeakersCount = 0;
  private lastEventTime: number = 0;
  private errorLog: string[] = [];

  get isReady(): boolean {
    return this.isInitialized;
  }

  /**
   * Initialize the YACA-TS adapter.
   * Sets up listeners for YacaBridgeQuest events.
   *
   * This is a skeleton implementation. In a real deployment:
   * - Register a custom packet handler for YACA voice events
   * - Connect to the gamemode API that YacaBridgeQuest will call
   * - Validate player IDs and voice ranges
   */
  async initialize(): Promise<void> {
    console.log("[YACA-TS Adapter] Initializing...");
    this.startTime = Date.now();

    // TODO: Register custom packet handler for YACA events
    // Example pattern (pseudo-code):
    //   customPacketRouter.on("yaca:voiceActivity", (playerId, state) => {
    //     this.handleYacaVoiceEvent(playerId, state);
    //   });

    // For now, mark as ready
    this.isInitialized = true;
    console.log("[YACA-TS Adapter] Initialized successfully");
  }

  /**
   * Shutdown the adapter.
   */
  async shutdown(): Promise<void> {
    // TODO: Unregister event handlers
    this.isInitialized = false;
    this.voiceActivityCallbacks.clear();
    console.log("[YACA-TS Adapter] Shutdown complete");
  }

  /**
   * Register callback for voice activity updates.
   * Called by Voice Activity Manager.
   *
   * @param callback Function to call on voice activity changes
   */
  onVoiceActivityUpdate(callback: (state: VoiceActivityState) => void): void {
    this.voiceActivityCallbacks.add(callback);
  }

  /**
   * Get health status of this adapter.
   * Used for diagnostics and monitoring.
   *
   * @returns Health status object
   */
  getHealthStatus() {
    return {
      isHealthy: this.isInitialized && this.errorLog.length === 0,
      uptime: Date.now() - this.startTime,
      activeSpeakers: this.activeSpeakersCount,
      lastEventTimestamp: this.lastEventTime,
      errors: this.errorLog.slice(-10), // Last 10 errors
    };
  }

  /**
   * Internal: Handle voice activity event from YacaBridgeQuest.
   * Called when a player's voice state changes in TeamSpeak.
   *
   * Expected to be called from YacaBridgeQuest via custom packet:
   * ```
   * mp.callGamemodeApi({
   *   method: "yaca:voiceActivity",
   *   actorId: "0x000ABC",
   *   isSpeaking: true,
   *   voiceRange: 8.0
   * })
   * ```
   *
   * @param actorId Skyrim actor ID
   * @param isSpeaking Whether player is speaking
   * @param voiceRange Voice range in meters
   */
  handleYacaVoiceEvent(actorId: string, isSpeaking: boolean, voiceRange: number): void {
    try {
      // Validate inputs
      if (!actorId || typeof actorId !== "string") {
        throw new Error(`Invalid actorId: ${actorId}`);
      }
      if (typeof isSpeaking !== "boolean") {
        throw new Error(`Invalid isSpeaking: ${isSpeaking}`);
      }
      if (typeof voiceRange !== "number" || voiceRange <= 0) {
        throw new Error(`Invalid voiceRange: ${voiceRange}`);
      }

      // Create voice activity state
      const state: VoiceActivityState = {
        actorId,
        isSpeaking,
        voiceRange,
        providerId: this.providerId,
        timestamp: Date.now(),
        metadata: {
          teamSpeakVoiceChannel: "ingame", // Example: could track TS channel
        },
      };

      // Track active speaker count
      if (isSpeaking) {
        this.activeSpeakersCount++;
      } else {
        this.activeSpeakersCount = Math.max(0, this.activeSpeakersCount - 1);
      }

      // Record event time
      this.lastEventTime = Date.now();

      // Broadcast to manager
      for (const callback of this.voiceActivityCallbacks) {
        callback(state);
      }
    } catch (err) {
      const errorMsg = err instanceof Error ? err.message : String(err);
      this.errorLog.push(`[${new Date().toISOString()}] ${errorMsg}`);
      console.error(`[YACA-TS Adapter] Error handling voice event:`, err);
    }
  }

  /**
   * Optional: Get current active speaker list from TeamSpeak.
   * Could be used for periodic validation or diagnostics.
   *
   * Requires integration with YACA plugin API.
   *
   * @returns Promise resolving to list of active speakers
   */
  async getActiveSpeakersFromTeamSpeak(): Promise<
    { actorId: string; voiceRange: number }[]
  > {
    // TODO: Query YACA plugin API for current speaker list
    // This would involve:
    // 1. Calling YACA-TS plugin API endpoint
    // 2. Mapping TeamSpeak user IDs to Skyrim actor IDs
    // 3. Returning normalized list
    return [];
  }

  /**
   * Optional: Set voice range for a player.
   * Can be called by admin API to dynamically adjust ranges.
   *
   * @param actorId Actor ID
   * @param voiceRange New voice range in meters
   */
  async setPlayerVoiceRange(actorId: string, voiceRange: number): Promise<void> {
    // TODO: Call YACA plugin API to update TeamSpeak user voice range
    console.log(
      `[YACA-TS Adapter] Set voice range for ${actorId}: ${voiceRange}m (not yet implemented)`
    );
  }
}

/**
 * Factory function to create YACA-TS adapter.
 *
 * @returns New YacaTeamSpeakAdapter instance
 */
export function createYacaTeamSpeakAdapter(): YacaTeamSpeakAdapter {
  return new YacaTeamSpeakAdapter();
}
