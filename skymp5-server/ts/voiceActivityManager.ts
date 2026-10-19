/**
 * Voice Activity Manager Implementation
 *
 * Handles voice state from multiple providers, manages proximity filtering,
 * and broadcasts to clients. Responsible for:
 * - Managing registered voice provider adapters
 * - Aggregating voice activity from all providers
 * - Proximity-based broadcasting to clients
 * - Health monitoring and diagnostics
 */

import {
  VoiceActivityState,
  VoiceActivityConfig,
  VoiceActivityPlayerState,
  IVoiceProviderAdapter,
  IVoiceActivityManager,
  VoiceActivityBroadcastCallback,
  VoiceActivityRecipientResolver,
  DEFAULT_VOICE_ACTIVITY_CONFIG,
} from "./voiceActivityApi";

export class VoiceActivityManager implements IVoiceActivityManager {
  private config: VoiceActivityConfig;
  private adapters: Map<string, IVoiceProviderAdapter> = new Map();
  private playerStates: Map<string, VoiceActivityPlayerState> = new Map();
  private broadcastCallback: VoiceActivityBroadcastCallback | null = null;
  private recipientResolver: VoiceActivityRecipientResolver | null = null;
  private lastBroadcastTime: number = 0;
  private initErrors: { providerId: string; error: string }[] = [];

  constructor(config?: Partial<VoiceActivityConfig>) {
    this.config = {
      ...DEFAULT_VOICE_ACTIVITY_CONFIG,
      ...config,
    };
  }

  /**
   * Set the broadcast callback used to send voice state to clients.
   * Must be called during initialization before registering adapters.
   *
   * @param callback Async function to send state to clients
   */
  setBroadcastCallback(
    callback: VoiceActivityBroadcastCallback,
    recipientResolver?: VoiceActivityRecipientResolver,
  ): void {
    this.broadcastCallback = callback;
    this.recipientResolver = recipientResolver ?? null;
  }

  /**
   * Check if VAD is globally enabled.
   */
  isEnabled(): boolean {
    return this.config.enabled;
  }

  /**
   * Remove a player's voice activity state immediately.
   * Used when a user disconnects or despawns.
   */
  removePlayer(actorId: string): void {
    const playerState = this.playerStates.get(actorId);
    if (!playerState) {
      return;
    }

    if (playerState.inactivityTimer) {
      clearTimeout(playerState.inactivityTimer);
    }

    this.playerStates.delete(actorId);
  }

  /**
   * Register a voice provider adapter.
   * Multiple adapters can be active simultaneously (e.g., YACA + Discord).
   *
   * @param adapter Provider adapter to register
   * @throws Error if provider validation fails or init fails
   */
  async registerAdapter(adapter: IVoiceProviderAdapter): Promise<void> {
    if (!this.isEnabled()) {
      throw new Error("Voice Activity Manager is disabled; cannot register adapters");
    }

    const providerId = adapter.providerId;

    // Check provider allowlist
    if (
      this.config.allowedProviders &&
      !this.config.allowedProviders.includes(providerId)
    ) {
      throw new Error(
        `Provider '${providerId}' is not in allowlist: ${this.config.allowedProviders.join(", ")}`
      );
    }

    // Check for duplicates
    if (this.adapters.has(providerId)) {
      throw new Error(`Provider '${providerId}' is already registered`);
    }

    try {
      // Initialize the adapter
      await adapter.initialize();

      // Register voice activity callback
      adapter.onVoiceActivityUpdate((state) => {
        this.handleVoiceActivityUpdate(state).catch((err) => {
          console.error(
            `[Voice Activity Manager] Error handling update from ${providerId}:`,
            err
          );
        });
      });

      this.adapters.set(providerId, adapter);
      console.log(`[Voice Activity Manager] Registered provider: ${providerId}`);
    } catch (err) {
      const errorMsg = err instanceof Error ? err.message : String(err);
      this.initErrors.push({ providerId, error: errorMsg });
      throw new Error(`Failed to initialize provider '${providerId}': ${errorMsg}`);
    }
  }

  /**
   * Unregister a voice provider adapter.
   *
   * @param providerId Provider ID to unregister
   */
  async unregisterAdapter(providerId: string): Promise<void> {
    const adapter = this.adapters.get(providerId);
    if (!adapter) {
      throw new Error(`Provider '${providerId}' is not registered`);
    }

    await adapter.shutdown();
    this.adapters.delete(providerId);

    // Clear all state from this provider
    for (const [actorId, playerState] of this.playerStates.entries()) {
      if (playerState.lastProviderId === providerId) {
        if (playerState.inactivityTimer) {
          clearTimeout(playerState.inactivityTimer);
        }
        this.playerStates.delete(actorId);
      }
    }

    console.log(`[Voice Activity Manager] Unregistered provider: ${providerId}`);
  }

  /**
   * Get current voice activity state for a player.
   *
   * @param actorId Actor ID
   * @returns VoiceActivityState or null if no active state
   */
  getPlayerVoiceState(actorId: string): VoiceActivityState | null {
    const playerState = this.playerStates.get(actorId);
    if (!playerState) return null;

    // Check for inactivity timeout
    const elapsed = Date.now() - playerState.lastUpdate;
    if (elapsed > this.config.inactivityTimeoutMs && playerState.currentState.isSpeaking) {
      // Auto-clear speaking state
      return {
        ...playerState.currentState,
        isSpeaking: false,
      };
    }

    return playerState.currentState;
  }

  /**
   * Get all players currently speaking.
   *
   * @returns Array of active speaker states
   */
  getActiveSpeakers(): VoiceActivityState[] {
    const speakers: VoiceActivityState[] = [];

    for (const [_actorId, playerState] of this.playerStates.entries()) {
      const state = this.getPlayerVoiceState(_actorId);
      if (state && state.isSpeaking) {
        speakers.push(state);
      }
    }

    return speakers;
  }

  /**
   * Internal: Handle incoming voice activity update from an adapter.
   * Validates, stores state, and triggers broadcast.
   *
   * @param state New voice activity state from adapter
   */
  private async handleVoiceActivityUpdate(state: VoiceActivityState): Promise<void> {
    if (!this.isEnabled()) {
      return; // Silently ignore if disabled
    }

    // Validate voice range
    if (this.config.voiceRangeTiers) {
      if (!this.config.voiceRangeTiers.includes(state.voiceRange)) {
        console.warn(
          `[Voice Activity Manager] Invalid voice range ${state.voiceRange} from ${state.providerId}, using default`
        );
        state.voiceRange = this.config.defaultVoiceRange;
      }
    }

    // Store or update player state
    let playerState = this.playerStates.get(state.actorId);
    if (!playerState) {
      playerState = {
        currentState: state,
        lastUpdate: state.timestamp,
        lastProviderId: state.providerId || null,
        clientNotified: false,
      };
      this.playerStates.set(state.actorId, playerState);
    } else {
      // Clear old inactivity timer
      if (playerState.inactivityTimer) {
        clearTimeout(playerState.inactivityTimer);
      }

      // Update state
      playerState.currentState = state;
      playerState.lastUpdate = state.timestamp;
      if (state.providerId) {
        playerState.lastProviderId = state.providerId;
      }
      playerState.clientNotified = false;
    }

    // Set inactivity timer if speaking
    if (state.isSpeaking) {
      playerState.inactivityTimer = setTimeout(() => {
        // Timeout: clear speaking state
        const clearedState: VoiceActivityState = {
          ...playerState!.currentState,
          isSpeaking: false,
          timestamp: Date.now(),
        };
        this.handleVoiceActivityUpdate(clearedState).catch((err) => {
          console.error("[Voice Activity Manager] Error clearing inactivity timeout:", err);
        });
      }, this.config.inactivityTimeoutMs);
    }

    // Broadcast to clients
    await this.broadcastVoiceActivity(state);
  }

  /**
   * Broadcast voice activity update to clients.
   * Applied proximity filtering and rate limiting.
   *
   * @param state Voice activity state to broadcast
   */
  async broadcastVoiceActivity(state: VoiceActivityState): Promise<void> {
    if (!this.broadcastCallback) {
      console.warn("[Voice Activity Manager] No broadcast callback set; state not sent to clients");
      return;
    }

    try {
      const recipientUserIds = this.recipientResolver
        ? await this.recipientResolver(state)
        : [];

      await this.broadcastCallback(state, recipientUserIds);
      this.lastBroadcastTime = Date.now();
    } catch (err) {
      console.error("[Voice Activity Manager] Error broadcasting voice state:", err);
    }
  }

  /**
   * Get overall health status of the VAD system.
   * Used for admin diagnostics.
   *
   * @returns Health status report
   */
  getHealthStatus(): {
    isEnabled: boolean;
    activeSpeakers: number;
    registeredAdapters: { providerId: string; isHealthy: boolean; uptime: number }[];
    lastBroadcastTime: number;
    errors: { providerId: string; error: string }[];
  } {
    const registeredAdapters = Array.from(this.adapters.values()).map((adapter) => {
      const health = adapter.getHealthStatus?.();
      return {
        providerId: adapter.providerId,
        isHealthy: health?.isHealthy ?? true,
        uptime: health?.uptime ?? 0,
      };
    });

    return {
      isEnabled: this.isEnabled(),
      activeSpeakers: this.getActiveSpeakers().length,
      registeredAdapters,
      lastBroadcastTime: this.lastBroadcastTime,
      errors: this.initErrors,
    };
  }

  /**
   * Shutdown the Voice Activity Manager.
   * Called during server shutdown.
   */
  async shutdown(): Promise<void> {
    // Clear all timers
    for (const playerState of this.playerStates.values()) {
      if (playerState.inactivityTimer) {
        clearTimeout(playerState.inactivityTimer);
      }
    }

    // Shutdown all adapters
    const shutdownPromises = Array.from(this.adapters.values()).map((adapter) =>
      adapter.shutdown().catch((err) => {
        console.error(`[Voice Activity Manager] Error shutting down ${adapter.providerId}:`, err);
      })
    );

    await Promise.all(shutdownPromises);
    this.playerStates.clear();
    this.adapters.clear();
    console.log("[Voice Activity Manager] Shutdown complete");
  }
}

/**
 * Factory function to create and initialize Voice Activity Manager.
 * Call this during server startup if VAD is configured.
 *
 * @param config Optional custom configuration
 * @returns Initialized Voice Activity Manager instance
 */
export function createVoiceActivityManager(
  config?: Partial<VoiceActivityConfig>
): VoiceActivityManager {
  return new VoiceActivityManager(config);
}
