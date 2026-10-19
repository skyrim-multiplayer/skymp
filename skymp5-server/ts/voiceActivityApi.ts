/**
 * Voice Activity Detection (VAD) API
 *
 * This module defines the standardized interface for voice activity detection
 * and synchronization. The API is provider-agnostic: any voice system
 * (YACA-TS, Discord, Mumble, custom VoIP) can integrate by implementing
 * the Voice Provider Adapter interface.
 *
 * Architecture:
 * - Voice Provider (TeamSpeak, Discord, etc.) generates voice events
 * - Provider Adapter translates provider-specific events → VoiceActivityState
 * - Server Voice Activity Manager receives state updates from any adapter
 * - Manager broadcasts "player X is speaking" to proximity-aware clients
 * - Clients apply lip-sync animations (IdleDialogueLock, etc.)
 */

/**
 * Standard voice activity state for any voice provider.
 * This is the contract between adapters and the Voice Activity Manager.
 */
export interface VoiceActivityState {
  /** Actor/player ID in Skyrim world (base ID or reference ID) */
  actorId: string;

  /** Whether the player is currently speaking */
  isSpeaking: boolean;

  /** Voice range tier (1.0, 3.0, 8.0, ..., 40.0 meters, or continuous distance 0-100m) */
  voiceRange: number;

  /** Optional: provider identifier for diagnostics (e.g., "yaca-ts", "discord-bot", "custom-voip") */
  providerId?: string;

  /** Timestamp when state was captured (milliseconds since epoch) */
  timestamp: number;

  /** Optional: additional provider-specific metadata for debugging */
  metadata?: Record<string, any>;
}

/**
 * Resolves the set of recipient user IDs for a voice activity update.
 */
export type VoiceActivityRecipientResolver = (
  state: VoiceActivityState,
) => Promise<string[]> | string[];

/**
 * Broadcast callback used by the manager after recipients have been resolved.
 */
export type VoiceActivityBroadcastCallback = (
  state: VoiceActivityState,
  recipientUserIds: string[],
) => Promise<void>;

/**
 * Configuration for voice activity detection behavior.
 */
export interface VoiceActivityConfig {
  /** Whether VAD is globally enabled */
  enabled: boolean;

  /** Supported voice range tiers in meters. Discrete tiers (e.g., [1.0, 3.0, 8.0, ...])
   * If null, accepts continuous values from providers.
   */
  voiceRangeTiers: number[] | null;

  /** Default voice range in meters if not specified by adapter */
  defaultVoiceRange: number;

  /** Proximity filter: only broadcast to players within this range of speaker */
  proximityDistance: number;

  /** Inactivity timeout: clear isSpeaking=false after N milliseconds of no updates */
  inactivityTimeoutMs: number;

  /** Maximum concurrent speakers per cell to prevent broadcast storm */
  maxConcurrentSpeakersPerCell?: number;

  /** Optional: provider allowlist (if null, all providers accepted) */
  allowedProviders?: string[] | null;
}

/**
 * Per-player voice activity state and metadata (server-side tracking).
 */
export interface VoiceActivityPlayerState {
  /** Most recent voice activity state from any provider */
  currentState: VoiceActivityState;

  /** Timestamp of last state update */
  lastUpdate: number;

  /** Which adapter/provider last updated this player's state */
  lastProviderId: string | null;

  /** Whether client has been notified of current state */
  clientNotified: boolean;

  /** Inactivity cleanup timer (if any) */
  inactivityTimer?: NodeJS.Timeout;
}

/**
 * Interface for voice provider adapters.
 * Each voice system (YACA-TS, Discord, etc.) implements this adapter
 * to integrate with the Voice Activity Manager.
 */
export interface IVoiceProviderAdapter {
  /** Human-readable name of this provider (e.g., "yaca-ts", "discord-bot") */
  readonly providerId: string;

  /** Whether this adapter is currently initialized and ready */
  readonly isReady: boolean;

  /**
   * Initialize the adapter.
   * Should set up event listeners, connections, etc.
   * Called once at server startup.
   *
   * @returns Promise that resolves when adapter is ready, or rejects if init fails
   */
  initialize(): Promise<void>;

  /**
   * Shutdown the adapter gracefully.
   * Should clean up resources, unsubscribe from events, etc.
   *
   * @returns Promise that resolves when cleanup is done
   */
  shutdown(): Promise<void>;

  /**
   * Register a callback to receive voice activity updates from this provider.
   * Adapter calls this callback whenever voice state changes.
   *
   * @param callback Function to call with new VoiceActivityState
   */
  onVoiceActivityUpdate(callback: (state: VoiceActivityState) => void): void;

  /**
   * Optional: Get current health/status of this adapter.
   * Used by monitoring/diagnostics systems.
   *
   * @returns Health status object (can be provider-specific)
   */
  getHealthStatus?(): {
    isHealthy: boolean;
    uptime: number; // milliseconds since init
    activeSpeakers: number;
    lastEventTimestamp: number;
    errors: string[];
  };
}

/**
 * Voice Activity Manager: server-side abstraction that handles
 * state from multiple providers and broadcasts to clients.
 */
export interface IVoiceActivityManager {
  /** Check if VAD is globally enabled */
  isEnabled(): boolean;

  /**
   * Set callback used to broadcast voice updates to clients and optional recipient resolver.
   */
  setBroadcastCallback(
    callback: VoiceActivityBroadcastCallback,
    recipientResolver?: VoiceActivityRecipientResolver,
  ): void;

  /**
   * Remove a player's voice state immediately, typically on disconnect.
   *
   * @param actorId Player/actor ID to remove
   */
  removePlayer(actorId: string): void;

  /**
   * Register a voice provider adapter.
   * Can be called multiple times to support multiple voice systems.
   *
   * @param adapter Provider adapter implementing IVoiceProviderAdapter
   * @returns Promise that resolves when adapter is registered and initialized
   */
  registerAdapter(adapter: IVoiceProviderAdapter): Promise<void>;

  /**
   * Unregister a voice provider adapter.
   *
   * @param providerId ID of provider to unregister
   * @returns Promise that resolves when adapter is shutdown
   */
  unregisterAdapter(providerId: string): Promise<void>;

  /**
   * Get current voice activity state for a player.
   * Returns null if player has no active state.
   *
   * @param actorId Player ID
   * @returns Current VoiceActivityState or null
   */
  getPlayerVoiceState(actorId: string): VoiceActivityState | null;

  /**
   * Get all players currently speaking.
   *
   * @returns Array of VoiceActivityState for active speakers
   */
  getActiveSpeakers(): VoiceActivityState[];

  /**
   * Broadcast voice activity update to relevant clients.
   * Internal method called when adapter sends new state.
   *
   * @param state New voice activity state
   */
  broadcastVoiceActivity(state: VoiceActivityState): Promise<void>;

  /**
   * Get overall health status of VAD system.
   * Used for admin diagnostics.
   *
   * @returns Health status with per-provider breakdowns
   */
  getHealthStatus(): {
    isEnabled: boolean;
    activeSpeakers: number;
    registeredAdapters: { providerId: string; isHealthy: boolean; uptime: number }[];
    lastBroadcastTime: number;
    errors: { providerId: string; error: string }[];
  };
}

/**
 * Configuration for voice range tiers.
 * Maps team speak YACA ranges to discrete distances.
 */
export const DEFAULT_VOICE_RANGE_TIERS = [1.0, 3.0, 8.0, 15.0, 20.0, 25.0, 30.0, 40.0];

/**
 * Default VAD configuration.
 */
export const DEFAULT_VOICE_ACTIVITY_CONFIG: VoiceActivityConfig = {
  enabled: false, // Disabled by default; operators enable explicitly
  voiceRangeTiers: DEFAULT_VOICE_RANGE_TIERS,
  defaultVoiceRange: 8.0,
  proximityDistance: 100.0, // Server broadcasts VAD only to players within 100m
  inactivityTimeoutMs: 1000, // Clear speaker state if no update for 1 second
  maxConcurrentSpeakersPerCell: 50,
  allowedProviders: null, // null = all providers accepted
};

/**
 * Export singleton factory and helpers.
 * Implementation in separate module (voiceActivityManager.ts).
 */
export let voiceActivityManager: IVoiceActivityManager | null = null;

/**
 * Initialize the Voice Activity Manager.
 * Call this once during server startup if VAD is configured.
 *
 * @param config Optional: custom VoiceActivityConfig (defaults to DEFAULT_VOICE_ACTIVITY_CONFIG)
 * @returns Instance of Voice Activity Manager
 */
export function initializeVoiceActivityManager(
  config?: Partial<VoiceActivityConfig>
): IVoiceActivityManager {
  // Loaded lazily to keep this file focused on contracts and avoid cycle issues.
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const { createVoiceActivityManager } = require('./voiceActivityManager');

  voiceActivityManager = createVoiceActivityManager(config);
  return voiceActivityManager;
}
