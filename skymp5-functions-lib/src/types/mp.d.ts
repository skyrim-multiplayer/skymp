export interface MakePropertyOptions {
  /**
   * If set to false, `updateOwner` would never be invoked
   * Player's client wouldn't see it's own value of this property
   * Reasonable for passwords and other secret values.
   */
  isVisibleByOwner: boolean;

  /**
   * If set to false, `updateNeighbor` would never be invoked.
   * Player's client wouldn't see values of neighbor Actors/ObjectReferences.
   */
  isVisibleByNeighbors: boolean;

  /**
   * Body of the function that would be invoked on client every update
   * for the PlayerCharacter.
   */
  updateOwner: string;

  /**
   * Body of the function that would be invoked on client every update
   * for each synchronized Actor/ObjectReference.
   */
  updateNeighbor: string;
}

export type JsonSerializablePrimitive = string | number | null;
export type JsonSerializable =
  | JsonSerializablePrimitive
  | JsonSerializable[]
  | { [key: string]: JsonSerializable }
  | Record<string, JsonSerializable>;

export interface EspmField {
  readonly type: string;
  readonly data: Uint8Array;
}

export interface EspmRecord {
  readonly id: number;
  readonly editorId: string;
  readonly type: string;
  readonly flags: number;
  readonly fields: EspmField[];
}

export interface EspmLookupResult {
  readonly record: EspmRecord;
  readonly fileIndex: number;
  readonly toGlobalRecordId: (localRecordId: number) => number;
}

export interface PapyrusObject {
  desc: string;
  type: 'form' | 'espm';
}

export type PapyrusValue = null | PapyrusObject | string | number | boolean | PapyrusValue[];

export type PapyrusMethod = (
  self: PapyrusObject,
  args: PapyrusValue[]
) => PapyrusValue | Promise<PapyrusValue> | void | Promise<void>;

export type PapyrusGlobalFunction = (
  self: null,
  args: PapyrusValue[]
) => PapyrusValue | Promise<PapyrusValue> | void | Promise<void>;

export interface Inventory {
  entries: InventoryItem[];
}
export interface InventoryItem {
  baseId: number;
  count: number;
}
export interface InventoryEq {
  entries: InventoryItemEq[];
}
export interface InventoryItemEq {
  baseId: number;
  count: number;
  worn: boolean;
}
export interface Equipment {
  inv: InventoryEq;
  numChanges: number;
}
export interface LocationalData {
  cellOrWorldDesc: string;
  pos: [number, number, number];
  rot: [number, number, number];
}

export interface PacketHistory {
  buffer: Uint8Array;
  packets: Array<{
    offset: number;
    size: number;
    timeMs: number;
  }>
}

export interface StartPoint {
  pos: [number, number, number];
  worldOrCell: string; // hex form id (like "0x3c", use parseInt)
  angleZ: number;
}

export interface ServerSettings {
  name?: string;
  ip?: string;
  port?: number;
  maxPlayers?: number;
  dataDir?: string;
  loadOrder?: string[];
  lang?: string;
  offlineMode?: boolean;
  databaseDriver?: string;
  reloot?: Record<string, number | undefined>;
  gamemodePath?: string;
  startPoints?: StartPoint[];
  isPapyrusHotReloadEnabled?: boolean;
  locale?: string;

  // TODO: add missing settings items here if any, sync with docs/docs_server_configuration_reference.md
  sweetPieMinimumPlayersToStart?: number;
  sweetPieAllowCheats?: boolean;
  sweetPieChatSettings?: {
    hearingRadiusNormal?: number
  };
  discordAuth?: {
    baseKitRoleId?: string;
    adminRoleId?: string;
  }

  readonly [key: string]: unknown;
}

export interface Mp {
  /**
   * Returns the actual value of a specified property. If there is no value, then
   * `undefined` returned.
   * @param formId A number representing ID of MpActor or MpObjectReference.
   * @param propertyName Name of the property we are reading.
   */
  get(formId: number, propertyName: string): JsonSerializable | undefined;
  get(formId: number, propertyName: 'neighbors'): number[];
  get(formId: number, propertyName: 'actorNeighbors'): number[];
  get(formId: number, propertyName: 'type'): 'MpActor' | 'MpObjectReference';
  get(formId: number, propertyName: 'appearance'): Record<string, unknown>;
  get(formId: number, propertyName: 'pos'): [number, number, number];
  get(formId: number, propertyName: 'locationalData'): LocationalData;
  get(formId: number, propertyName: 'spawnPoint'): LocationalData;
  get(formId: number, propertyName: 'angle'): [number, number, number];
  get(formId: number, propertyName: 'inventory'): Inventory;
  get(formId: number, propertyName: 'equipment'): Equipment;
  get(formId: number, propertyName: 'baseDesc'): string;
  get(formId: 0, propertyName: 'onlinePlayers'): number[];
  get(formId: number, propertyName: 'isDead'): boolean;
  get(formId: number, propertyName: 'worldOrCellDesc'): string;
  get(formId: number, propertyName: 'percentages'): { health: number, magicka: number, stamina: number };
  get(formId: number, propertyName: 'profileId'): number;

  /**
   * Modifies value of the specified property.
   * @param formId A number representing ID of MpActor or MpObjectReference.
   * @param propertyName Name of the property we are modifying.
   * @param newValue A new value for the property.
   */
  set(formId: number, propertyName: string, newValue: JsonSerializable): void;
  set(formId: number, propertyName: 'inventory', newValue: Inventory): void;
  set(formId: number, propertyName: 'locationalData', newValue: LocationalData): void;
  set(formId: number, propertyName: 'spawnPoint', newValue: LocationalData): void;
  set(formId: number, propertyName: 'isDead', newValue: boolean): void;
  set(formId: number, propertyName: 'percentages', newValue: { health: number, magicka: number, stamina: number }): void;

  /**
   * Creates a new property that would be attached to all instances of
   * `MpActor` and `MpObjectReference`. Values are saved to database automatically.
   * @param propertyName A unique name for the property being created.
   * @param options Property options.
   */
  makeProperty(propertyName: string, options: MakePropertyOptions): void;

  /**
   * Creates a new event source allowing you to catch specific game
   * situations and pass them to a server as events.
   * @param eventName A unique name for the event being created.
   * @param functionBody Body of the function that will be used to initialize event
   * source on client.
   */
  makeEventSource(eventName: string, functionBody: string): void;

  /**
   * Clears added properties and event sources.
   */
  clear(): void;

  lookupEspmRecordById(globalRecordId: number): EspmLookupResult | Partial<EspmLookupResult>;

  getEspmLoadOrder(): string[];

  getDescFromId(formId: number): string;

  getIdFromDesc(formDesc: string): number;

  getLocalizedString(id: number): string;

  place(globalRecordId: number): number;

  registerPapyrusFunction(callType: 'method', className: string, functionName: string, f: PapyrusMethod): void;
  registerPapyrusFunction(callType: 'global', className: string, functionName: string, f: PapyrusGlobalFunction): void;

  callPapyrusFunction(
    callType: 'method',
    className: string,
    functionName: string,
    self: PapyrusObject,
    args: PapyrusValue[]
  ): PapyrusValue;
  callPapyrusFunction(
    callType: 'global',
    className: string,
    functionName: string,
    self: null,
    args: PapyrusValue[]
  ): PapyrusValue;

  getServerSettings(): ServerSettings;

  setPacketHistoryRecording(userId: number, enabled: boolean): void;
  getPacketHistory(userId: number): PacketHistory;
  clearPacketHistory(userId: number): void;
  requestPacketHistoryPlayback(userId: number, packetHistory: PacketHistory): void;

  [key: string]: unknown;
}
