// ── SkyMP native types ────────────────────────────────────────────────────────
// Sourced from skymp/skymp5-server/ts/scampNative.ts and observed runtime usage.

/** A bot actor managed by the server. */
export interface Bot {
  destroy(): void
  send(msg: Record<string, unknown>): void
}

/** A single spawn location defined in server-settings.json. */
export interface StartPoint {
  pos: [number, number, number]
  worldOrCell: string
  angleZ: number
}

/**
 * The SkyMP `mp` object available as `globalThis.mp` in the gamemode.
 *
 * Declared methods come from ScampServer (scampNative.ts).
 * Undeclared native extensions (get, set, makeProperty, makeEventSource,
 * findFormsByPropertyValue, sendCustomPacket with 3 args) are used as
 * `(mp as any)` in the server internals but are fully available in gamemode
 * context — they are typed here for convenience.
 */
export interface Mp {
  // ── Lifecycle events ────────────────────────────────────────────────────────
  on(event: 'connect',      handler: (userId: number) => void): void
  on(event: 'disconnect',   handler: (userId: number) => void): void
  on(event: 'customPacket', handler: (userId: number, content: string) => void): void
  on(event: string,         handler: (...args: any[]) => void): void

  // ── Actor management ────────────────────────────────────────────────────────
  /** Create a new actor. Returns the new actorId. */
  createActor(
    formId: number,
    pos: number[],
    angleZ: number,
    cellOrWorld: number,
    userProfileId?: number,
  ): number
  destroyActor(formId: number): void
  setEnabled(actorId: number, enabled: boolean): void
  setRaceMenuOpen(formId: number, open: boolean): void

  // ── User ↔ actor association ─────────────────────────────────────────────────
  setUserActor(userId: number, actorFormId: number): void
  getUserActor(userId: number): number
  getUserByActor(formId: number): number
  getActorsByProfileId(profileId: number): number[]

  // ── User info ───────────────────────────────────────────────────────────────
  getUserGuid(userId: number): string
  getUserIp(userId: number): string
  isConnected(userId: number): boolean
  kick(userId: number): void

  // ── Actor info ──────────────────────────────────────────────────────────────
  getActorName(actorId: number): string
  /** Returns [x, y, z] position or null if unavailable. */
  getActorPos(actorId: number): [number, number, number] | null
  getActorCellOrWorld(actorId: number): number

  // ── Custom properties (undeclared native extension) ─────────────────────────
  /** Read a property from an actor/form. Returns null if not set. */
  get(formId: number, propertyName: string): any
  /** Write a property on an actor/form. */
  set(formId: number, propertyName: string, value: unknown): void
  /**
   * Find all form IDs where a property equals a given value.
   * Requires the property to have been created with an index via makeProperty.
   */
  findFormsByPropertyValue(propertyName: string, value: unknown): number[]

  // ── Property & event source registration ────────────────────────────────────
  /** Register a synced actor property visible to owner and/or neighbors. */
  makeProperty(name: string, config: MpPropertyConfig): void
  /**
   * Register a client-side event source. The functionBody runs in the
   * Skyrim Platform JS context and can call ctx.sendEvent() to fire the
   * named server-side handler (e.g. mp['eventName']).
   */
  makeEventSource(name: string, functionBody: string): void

  // ── Networking ──────────────────────────────────────────────────────────────
  /**
   * Send a custom packet to a specific user (system-level).
   * jsonContent must be a JSON string.
   */
  sendCustomPacket(userId: number, jsonContent: string): void
  /**
   * Send a named packet with a data payload to an actor (gamemode-level).
   * This is an undeclared native extension available in gamemode context.
   */
  sendCustomPacket(actorId: number, packetName: string, data: Record<string, unknown>): void

  // ── Bot management ──────────────────────────────────────────────────────────
  createBot(): Bot

  // ── Economy (dynamically added by MasterApiBalanceSystem) ───────────────────
  /** Returns the player's current master-api balance in credits. */
  getUserMasterApiBalance?(userId: number): Promise<number>
  /** Deduct credits from a player's master-api balance. */
  makeUserMasterApiPurchase?(userId: number, balanceToSpend: number): Promise<{ balanceSpent: number; success: boolean }>

  // ── Gamemode lifecycle hooks ─────────────────────────────────────────────────
  /**
   * Called by the login system before spawning a player.
   * Return false to reject the login (e.g. custom ban logic).
   */
  onLoginAttempt?(userProfileId: number): boolean

  // ── Server internals ────────────────────────────────────────────────────────
  attachSaveStorage(): void
  tick(): void
  clear(): void
  writeLogs(logLevel: string, message: string): void
  getPrometheusMetrics(): string
  executeJavaScriptOnChakra(src: string): void

  /** Allow dynamic property access for server-assigned callbacks (e.g. mp['cef_chat_send']). */
  [key: string]: any
}

/** Configuration for a synced actor property created with mp.makeProperty(). */
export interface MpPropertyConfig {
  /** Whether the property value is sent to the owning client. */
  isVisibleByOwner: boolean
  /** Whether the property value is sent to neighboring clients. */
  isVisibleByNeighbors: boolean
  /**
   * JavaScript function body that runs in the Skyrim Platform context on the
   * owner's machine whenever the property value changes. Has access to
   * `ctx.value`, `ctx.state`, `ctx.sp`, and `ctx.sendEvent`.
   */
  updateOwner: string
  /**
   * JavaScript function body that runs in the Skyrim Platform context on a
   * neighbor's machine whenever the property value changes.
   */
  updateNeighbor: string
}

// ── Gamemode store types ──────────────────────────────────────────────────────

export interface PlayerState {
  id: number
  actorId: number
  name: string
  holdId: string | null
  factions: string[]
  bounty: Record<string, number>
  isDown: boolean
  isCaptive: boolean
  downedAt: number | null
  captiveAt: number | null
  properties: string[]
  hungerLevel: number
  drunkLevel: number
  septims: number
  stipendPaidHours: number
  minutesOnline: number
  isStaff: boolean
  isLeader: boolean
}

export interface Store {
  register(id: number, actorId: number, name: string): void
  deregister(id: number): void
  get(id: number): PlayerState | null
  getAll(): PlayerState[]
  update(id: number, patch: Partial<PlayerState>): void
}

export interface BusEvent {
  type: string
  [key: string]: unknown
}

export interface Bus {
  on(type: string, fn: (event: BusEvent) => void): void
  off(type: string, fn: (event: BusEvent) => void): void
  dispatch(event: BusEvent): void
}

// ── System domain types ───────────────────────────────────────────────────────

export interface Notification {
  id: number
  type: string
  fromPlayerId: number
  toPlayerId: number
  holdId: string | null
  payload: Record<string, unknown>
  createdAt: number
  expiresAt: number
  read: boolean
}

export interface Inventory {
  entries: Array<{ baseId: number; count: number }>
}

export interface FactionMembership {
  factionId: string
  rank: number
  joinedAt: number
}

export interface FactionDocument {
  factionId: string
  benefits: string
  burdens: string
  bylaws: string
  updatedAt?: number
}

export interface PropertyDef {
  id: string
  name: string
  holdId: string
  type: 'home' | 'business'
}

export interface PropertyState {
  ownerId: number | null
  pendingOwnerId: number | null
}

export interface PropertyRecord extends PropertyDef, PropertyState {}

export interface PrisonQueueEntry {
  playerId: number
  holdId: string
  arrestedBy: number
  queuedAt: number
}

export type SentenceType = 'fine' | 'release' | 'banish'

export interface Sentence {
  type: SentenceType
  fineAmount?: number
}

export interface StudyBoost {
  skillId: string
  multiplier: number
  remainingOnlineMs: number
  sessionStart: number
}

export interface LectureSession {
  attendees: Set<number>
}

export interface TrainingSession {
  skillId: string
  attendees: Set<number>
}
