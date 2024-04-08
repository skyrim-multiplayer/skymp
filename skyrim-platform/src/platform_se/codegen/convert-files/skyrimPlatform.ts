
/* eslint-disable @typescript-eslint/adjacent-overload-signatures */
/* eslint-disable @typescript-eslint/no-namespace */
// Generated automatically. Do not edit.

export declare class PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): PapyrusObject | null
}
export type PapyrusValue = PapyrusObject | number | string | boolean | null | PapyrusValue[]
export declare function printConsole(...arguments: unknown[]): void
export declare function writeLogs(pluginName: string, ...arguments: unknown[]): void
export declare function setPrintConsolePrefixesEnabled(enabled: boolean): void
export declare function callNative(className: string, functionName: string, self?: PapyrusObject, ...args: PapyrusValue[]): PapyrusValue
export declare function getJsMemoryUsage(): number
export declare function getPluginSourceCode(pluginName: string): string
export declare function writePlugin(pluginName: string, newSources: string): string
export declare function getPlatformVersion(): string
export declare function disableCtrlPrtScnHotkey(): void
export declare function blockPapyrusEvents(block: boolean): void
export declare function sendIpcMessage(targetSystemName: string, message: ArrayBuffer): void
export declare function encodeUtf8(text: string): ArrayBuffer
export declare function decodeUtf8(buffer: ArrayBuffer): string
export declare let storage: Record<string, unknown>
export declare let settings: Record<string, Record<string, unknown>>

export declare function on(eventName: 'update', callback: () => void): EventHandle
export declare function once(eventName: 'update', callback: () => void): EventHandle

export declare function on(eventName: 'tick', callback: () => void): EventHandle
export declare function once(eventName: 'tick', callback: () => void): EventHandle

export interface IpcMessageEvent {
  sourceSystemName: string
  message: ArrayBuffer
}

export declare function on(eventName: 'ipcMessage', callback: (event: IpcMessageEvent) => void): EventHandle
export declare function once(eventName: 'ipcMessage', callback: (event: IpcMessageEvent) => void): EventHandle

export interface Face {
  hairColor: number
  bodySkinColor: number
  headTextureSetId: number
  headPartIds: number[]
  presets: number[]
}

export interface ChangeFormNpc {
  raceId?: number
  name?: string
  face?: Face
}

export declare function loadGame(pos: number[], angle: number[], worldOrCell: number, changeFormNpc?: ChangeFormNpc, loadOrder?: string[], time?: { seconds: number, minutes: number, hours: number }): void

export declare function worldPointToScreenPoint(...args: number[][]): number[][]

export type PacketType = 'message' | 'disconnect' | 'connectionAccepted' | 'connectionFailed' | 'connectionDenied'

// Available only if multiplayer is installed on user's machine
interface MpClientPlugin {
  getVersion(): string
  createClient(host: string, port: number): void
  destroyClient(): void
  isConnected(): boolean
  tick(tickHandler: (packetType: PacketType, jsonContent: string, error: string) => void): void
  send(jsonContent: string, reliable: boolean): void
}
export declare let mpClientPlugin: MpClientPlugin

export interface Browser {
  setVisible(visible: boolean): void
  isVisible(): boolean
  setFocused(focused: boolean): void
  isFocused(): boolean
  loadUrl(url: string): void
  getToken(): string
  executeJavaScript(src: string): void
}
export declare let browser: Browser

export interface Win32 {
  loadUrl(url: string): void
  exitProcess(): void
}
export declare let win32: Win32

export interface ExtraData {
  type: 'Health' | 'Count' | 'Enchantment' | 'Charge' | 'TextDisplayData' | 'Soul' | 'Poison' | 'Worn' | 'WornLeft'
}
export interface ExtraHealth extends ExtraData {
  type: 'Health'
  health: number
}
export interface ExtraCount extends ExtraData {
  type: 'Count'
  count: number
}
export interface ExtraEnchantment extends ExtraData {
  type: 'Enchantment'
  enchantmentId: number
  maxCharge: number
  removeOnUnequip: boolean
}
export interface ExtraCharge extends ExtraData {
  type: 'Charge'
  charge: number
}
export interface ExtraTextDisplayData extends ExtraData {
  type: 'TextDisplayData'
  name: string
}
export interface ExtraSoul extends ExtraData {
  type: 'Soul'
  soul: 0 | 1 | 2 | 3 | 4 | 5
}
export interface ExtraPoison extends ExtraData {
  type: 'Poison'
  poisonId: number
  count: number
}
export interface ExtraWorn extends ExtraData {
  type: 'Worn'
}
export interface ExtraWornLeft extends ExtraData {
  type: 'WornLeft'
}
export type BaseExtraList = ExtraData[]
export interface InventoryChangesEntry {
  countDelta: number
  baseId: number
  extendDataList: BaseExtraList[]
}
export declare let getExtraContainerChanges: (objectReferenceId: number) => InventoryChangesEntry[] | null

export interface InventoryEntry {
  count: number
  baseId: number
}
export declare let getContainer: (baseId: number) => InventoryEntry[]

export interface ActivateEvent {
  target: ObjectReference
  caster: ObjectReference
  isCrimeToActivate: boolean
}

export interface MoveAttachDetachEvent {
  movedRef: ObjectReference
  isCellAttached: boolean
}
export interface WaitStopEvent {
  isInterrupted: boolean
}
export interface ObjectLoadedEvent {
  object: Form
  isLoaded: boolean
}
export interface LockChangedEvent {
  lockedObject: ObjectReference
}

export interface CellFullyLoadedEvent {
  cell: Cell
}

export interface GrabReleaseEvent {
  refr: ObjectReference
  isGrabbed: boolean
}

export interface SwitchRaceCompleteEvent {
  subject: ObjectReference
}

export interface UniqueIDChangeEvent {
  oldBaseID: number
  newBaseID: number
  oldUniqueID: number
  newUniqueID: number
}

export interface TrackedStatsEvent {
  statName: string
  newValue: number
}

export interface InitScriptEvent {
  initializedObject: ObjectReference
}

export interface ResetEvent {
  object: ObjectReference
}

export interface CombatEvent {
  target: ObjectReference
  actor: ObjectReference
  isCombat: boolean
  isSearching: boolean
}

export interface DeathEvent {
  actorDying: ObjectReference
  actorKiller: ObjectReference
}

export interface ContainerChangedEvent {
  oldContainer: ObjectReference
  newContainer: ObjectReference
  baseObj: Form
  numItems: number
  uniqueID: number
  reference: ObjectReference
}

export interface HitEvent {
  target: ObjectReference
  aggressor: ObjectReference
  source: Form
  projectile: Projectile
  isPowerAttack: boolean
  isSneakAttack: boolean
  isBashAttack: boolean
  isHitBlocked: boolean
}

export interface EquipEvent {
  actor: ObjectReference
  baseObj: Form
  uniqueId: number
  originalRefr: ObjectReference
}

export interface ActiveEffectApplyRemoveEvent {
  effect: MagicEffect
  caster: ObjectReference
  target: ObjectReference
}

export interface MenuOpenEvent {
  name: string
}

export interface MenuCloseEvent {
  name: string
}

export interface MagicEffectApplyEvent {
  activeEffect: ActiveMagicEffect
  effect: MagicEffect
  caster: ObjectReference
  target: ObjectReference
}

export interface BrowserMessageEvent {
  arguments: unknown[]
}

/*
 * Note: The message text can contain any characters, including `'` `"` `\`.
 * Before sending the text to the browser using "browser.executeJavaScript", it should be escaped.
 */
export interface ConsoleMessageEvent {
  message: string
}

export interface SpellCastEvent {
  caster: ObjectReference
  spell: Spell
}

export interface OpenCloseEvent {
  target: ObjectReference
  cause: ObjectReference | undefined
}

export interface QuestInitEvent {
  quest: Quest
}

export interface QuestStartStopEvent {
  quest: Quest
}

export interface QuestStageEvent {
  quest: Quest
  stage: number
}

export interface TriggerEvent {
  target: ObjectReference
  cause: ObjectReference
}

export interface SleepStartEvent {
  startTime: number
  desiredStopTime: number
}

export interface SleepStopEvent {
  isInterrupted: boolean
}

export interface CellAttachDetachEvent {
  refr: ObjectReference
}

export interface WaitStartEvent {
  startTime: number
  desiredStopTime: number
}

export interface LocationChangedEvent {
  actor: Actor
  oldLoc: Location
  newLoc: Location
}

export interface BookReadEvent {
  book: Book
}

export interface SellEvent {
  target: ObjectReference
  seller: ObjectReference
}

export interface FurnitureEvent {
  target: ObjectReference
  actor: ObjectReference
}

export interface WardHitEvent {
  target: ObjectReference
  caster: ObjectReference
  spell: Spell
  status: WardHitStatus
}

export interface PackageEvent {
  actor: ObjectReference
  package: Package
}

export interface EnterBleedoutEvent {
  actor: ObjectReference
}

export interface DestructionStageChangedEvent {
  target: ObjectReference
  oldStage: number
  newStage: number
}

export interface SceneActionEvent {
  actorAliasId: number
  actionIndex: number
  scene: Scene
  quest: Quest
}

export interface PlayerBowShotEvent {
  weapon: Weapon
  ammo: Ammo
  power: number
  isSunGazing: boolean
}

export interface FastTravelEnd {
  travelTimeGameHours: number
}

export interface PerkEntryRunEvent {
  cause: ObjectReference
  target: ObjectReference
  perk: Perk
  flag: number
}

export interface ActionEvent {
  actor: Actor
  source: Form
  slot: SlotType
}

export interface CameraStateChangedEvent {
  oldStateId: number
  newStateId: number
}

export interface CrosshairRefChangedEvent {
  reference: ObjectReference | undefined
}

export interface NiNodeUpdateEvent {
  reference: ObjectReference | undefined
}

export interface ModEvent {
  sender: Form
  eventName: string
  strArg: string
  numArg: number
}

export interface PositionPlayerEvent {
  eventType: PlayerPositionEventType
}

export interface FootstepEvent {
  tag: string
}

export interface TranslationEvent {
  reference: ObjectReference
}

export interface ButtonEvent {
  device: InputDeviceType
  code: DxScanCode
  userEventName: string
  value: number
  heldDuration: number
  isPressed: boolean
  isUp: boolean
  isDown: boolean
  isHeld: boolean
  isRepeating: boolean
}

export interface MouseMoveEvent {
  device: InputDeviceType
  code: DxScanCode
  userEventName: string
  inputX: number
  inputY: number
}

export interface ThumbstickEvent {
  device: InputDeviceType
  code: DxScanCode
  userEventName: string
  inputX: number
  inputY: number
  isLeft: boolean
  isRight: boolean
}

export interface KinectEvent {
  device: InputDeviceType
  code: DxScanCode
  userEventName: string
  heard: string
}

export interface DeviceConnectEvent {
  device: InputDeviceType
  isConnected: boolean
}

export interface EventHandle {
  uid: number
  eventName: string
}

export interface ActorKillEvent {
  killer: Actor
  victim: Actor
}

export interface BooksReadEvent {
  book: Book
}

export interface CriticalHitEvent {
  aggressor: ObjectReference
  weapon: Weapon
  isSneakHit: boolean
}

export interface DisarmedEvent {
  source: Actor
  target: Actor
}

export interface DragonSoulsGainedEvent {
  souls: number
}

export interface ItemHarvestedEvent {
  produceItem: Form
  harvester: Actor
}

export interface LevelIncreaseEvent {
  player: Actor
  newLevel: number
}

export interface LocationDiscoveryEvent {
  worldSpaceId: string
  name: string
  markerType: MarkerType
  isVisible: boolean
  canTravelTo: boolean
  isShowAllHidden: boolean
}

export interface ShoutAttackEvent {
  shout: Shout
}

export interface SkillIncreaseEvent {
  player: Actor
  actorValue: ActorValue
}

export interface SoulsTrappedEvent {
  trapper: Actor
  target: Actor
}

export interface SpellsLearnedEvent {
  spell: Spell
}

export declare function unsubscribe(handle: EventHandle): void

export declare function on(eventName: 'skyrimLoaded', callback: () => void): EventHandle
export declare function once(eventName: 'skyrimLoaded', callback: () => void): EventHandle

export declare function on(eventName: 'newGame', callback: () => void): EventHandle
export declare function once(eventName: 'newGame', callback: () => void): EventHandle

export declare function on(eventName: 'preLoadGame', callback: () => void): EventHandle
export declare function once(eventName: 'preLoadGame', callback: () => void): EventHandle

export declare function on(eventName: 'postLoadGame', callback: () => void): EventHandle
export declare function once(eventName: 'postLoadGame', callback: () => void): EventHandle

export declare function on(eventName: 'saveGame', callback: () => void): EventHandle
export declare function once(eventName: 'saveGame', callback: () => void): EventHandle

export declare function on(eventName: 'deleteGame', callback: () => void): EventHandle
export declare function once(eventName: 'deleteGame', callback: () => void): EventHandle

export declare function on(eventName: 'activate', callback: (event: ActivateEvent) => void): EventHandle
export declare function once(eventName: 'activate', callback: (event: ActivateEvent) => void): EventHandle

export declare function on(eventName: 'waitStop', callback: (event: WaitStopEvent) => void): EventHandle
export declare function once(eventName: 'waitStop', callback: (event: WaitStopEvent) => void): EventHandle

export declare function on(eventName: 'objectLoaded', callback: (event: ObjectLoadedEvent) => void): EventHandle
export declare function once(eventName: 'objectLoaded', callback: (event: ObjectLoadedEvent) => void): EventHandle

export declare function on(eventName: 'moveAttachDetach', callback: (event: MoveAttachDetachEvent) => void): EventHandle
export declare function once(eventName: 'moveAttachDetach', callback: (event: MoveAttachDetachEvent) => void): EventHandle

export declare function on(eventName: 'lockChanged', callback: (event: LockChangedEvent) => void): EventHandle
export declare function once(eventName: 'lockChanged', callback: (event: LockChangedEvent) => void): EventHandle

export declare function on(eventName: 'grabRelease', callback: (event: GrabReleaseEvent) => void): EventHandle
export declare function once(eventName: 'grabRelease', callback: (event: GrabReleaseEvent) => void): EventHandle

export declare function on(eventName: 'cellFullyLoaded', callback: (event: CellFullyLoadedEvent) => void): EventHandle
export declare function once(eventName: 'cellFullyLoaded', callback: (event: CellFullyLoadedEvent) => void): EventHandle

export declare function on(eventName: 'switchRaceComplete', callback: (event: SwitchRaceCompleteEvent) => void): EventHandle
export declare function once(eventName: 'switchRaceComplete', callback: (event: SwitchRaceCompleteEvent) => void): EventHandle

export declare function on(eventName: 'uniqueIdChange', callback: (event: UniqueIDChangeEvent) => void): EventHandle
export declare function once(eventName: 'uniqueIdChange', callback: (event: UniqueIDChangeEvent) => void): EventHandle

export declare function on(eventName: 'trackedStats', callback: (event: TrackedStatsEvent) => void): EventHandle
export declare function once(eventName: 'trackedStats', callback: (event: TrackedStatsEvent) => void): EventHandle

export declare function on(eventName: 'scriptInit', callback: (event: InitScriptEvent) => void): EventHandle
export declare function once(eventName: 'scriptInit', callback: (event: InitScriptEvent) => void): EventHandle

export declare function on(eventName: 'reset', callback: (event: ResetEvent) => void): EventHandle
export declare function once(eventName: 'reset', callback: (event: ResetEvent) => void): EventHandle

export declare function on(eventName: 'combatState', callback: (event: CombatEvent) => void): EventHandle
export declare function once(eventName: 'combatState', callback: (event: CombatEvent) => void): EventHandle

export declare function on(eventName: 'loadGame', callback: () => void): EventHandle
export declare function once(eventName: 'loadGame', callback: () => void): EventHandle

export declare function on(eventName: 'deathEnd', callback: (event: DeathEvent) => void): EventHandle
export declare function once(eventName: 'deathEnd', callback: (event: DeathEvent) => void): EventHandle

export declare function on(eventName: 'deathStart', callback: (event: DeathEvent) => void): EventHandle
export declare function once(eventName: 'deathStart', callback: (event: DeathEvent) => void): EventHandle

export declare function on(eventName: 'containerChanged', callback: (event: ContainerChangedEvent) => void): EventHandle
export declare function once(eventName: 'containerChanged', callback: (event: ContainerChangedEvent) => void): EventHandle

export declare function on(eventName: 'hit', callback: (event: HitEvent) => void): EventHandle
export declare function once(eventName: 'hit', callback: (event: HitEvent) => void): EventHandle

export declare function on(eventName: 'unequip', callback: (event: EquipEvent) => void): EventHandle
export declare function once(eventName: 'unequip', callback: (event: EquipEvent) => void): EventHandle

export declare function on(eventName: 'equip', callback: (event: EquipEvent) => void): EventHandle
export declare function once(eventName: 'equip', callback: (event: EquipEvent) => void): EventHandle

export declare function on(eventName: 'magicEffectApply', callback: (event: MagicEffectApplyEvent) => void): EventHandle
export declare function once(eventName: 'magicEffectApply', callback: (event: MagicEffectApplyEvent) => void): EventHandle

export declare function on(eventName: 'effectFinish', callback: (event: ActiveEffectApplyRemoveEvent) => void): EventHandle
export declare function once(eventName: 'effectFinish', callback: (event: ActiveEffectApplyRemoveEvent) => void): EventHandle

export declare function on(eventName: 'effectStart', callback: (event: ActiveEffectApplyRemoveEvent) => void): EventHandle
export declare function once(eventName: 'effectStart', callback: (event: ActiveEffectApplyRemoveEvent) => void): EventHandle

export declare function on(eventName: 'menuOpen', callback: (event: MenuOpenEvent) => void): EventHandle
export declare function once(eventName: 'menuOpen', callback: (event: MenuOpenEvent) => void): EventHandle

export declare function on(eventName: 'menuClose', callback: (event: MenuCloseEvent) => void): EventHandle
export declare function once(eventName: 'menuClose', callback: (event: MenuCloseEvent) => void): EventHandle

export declare function on(eventName: 'browserMessage', callback: (event: BrowserMessageEvent) => void): EventHandle
export declare function once(eventName: 'browserMessage', callback: (event: BrowserMessageEvent) => void): EventHandle

export declare function on(eventName: 'consoleMessage', callback: (event: ConsoleMessageEvent) => void): EventHandle
export declare function once(eventName: 'consoleMessage', callback: (event: ConsoleMessageEvent) => void): EventHandle

export declare function on(eventName: 'spellCast', callback: (event: SpellCastEvent) => void): EventHandle
export declare function once(eventName: 'spellCast', callback: (event: SpellCastEvent) => void): EventHandle

export declare function on(eventName: 'open', callback: (event: OpenCloseEvent) => void): EventHandle
export declare function once(eventName: 'open', callback: (event: OpenCloseEvent) => void): EventHandle

export declare function on(eventName: 'close', callback: (event: OpenCloseEvent) => void): EventHandle
export declare function once(eventName: 'close', callback: (event: OpenCloseEvent) => void): EventHandle

export declare function on(eventName: 'questInit', callback: (event: QuestInitEvent) => void): EventHandle
export declare function once(eventName: 'questInit', callback: (event: QuestInitEvent) => void): EventHandle

export declare function on(eventName: 'questStart', callback: (event: QuestStartStopEvent) => void): EventHandle
export declare function once(eventName: 'questStart', callback: (event: QuestStartStopEvent) => void): EventHandle

export declare function on(eventName: 'questStop', callback: (event: QuestStartStopEvent) => void): EventHandle
export declare function once(eventName: 'questStop', callback: (event: QuestStartStopEvent) => void): EventHandle

export declare function on(eventName: 'questStage', callback: (event: QuestStageEvent) => void): EventHandle
export declare function once(eventName: 'questStage', callback: (event: QuestStageEvent) => void): EventHandle

export declare function on(eventName: 'trigger', callback: (event: TriggerEvent) => void): EventHandle
export declare function once(eventName: 'trigger', callback: (event: TriggerEvent) => void): EventHandle

export declare function on(eventName: 'triggerEnter', callback: (event: TriggerEvent) => void): EventHandle
export declare function once(eventName: 'triggerEnter', callback: (event: TriggerEvent) => void): EventHandle

export declare function on(eventName: 'triggerLeave', callback: (event: TriggerEvent) => void): EventHandle
export declare function once(eventName: 'triggerLeave', callback: (event: TriggerEvent) => void): EventHandle

export declare function on(eventName: 'sleepStart', callback: (event: SleepStartEvent) => void): EventHandle
export declare function once(eventName: 'sleepStart', callback: (event: SleepStartEvent) => void): EventHandle

export declare function on(eventName: 'sleepStop', callback: (event: SleepStopEvent) => void): EventHandle
export declare function once(eventName: 'sleepStop', callback: (event: SleepStopEvent) => void): EventHandle

export declare function on(eventName: 'cellAttach', callback: (event: CellAttachDetachEvent) => void): EventHandle
export declare function once(eventName: 'cellAttach', callback: (event: CellAttachDetachEvent) => void): EventHandle

export declare function on(eventName: 'cellDetach', callback: (event: CellAttachDetachEvent) => void): EventHandle
export declare function once(eventName: 'cellDetach', callback: (event: CellAttachDetachEvent) => void): EventHandle

export declare function on(eventName: 'waitStart', callback: (event: WaitStartEvent) => void): EventHandle
export declare function once(eventName: 'waitStart', callback: (event: WaitStartEvent) => void): EventHandle

export declare function on(eventName: 'locationChanged', callback: (event: LocationChangedEvent) => void): EventHandle
export declare function once(eventName: 'locationChanged', callback: (event: LocationChangedEvent) => void): EventHandle

export declare function on(eventName: 'bookRead', callback: (event: BookReadEvent) => void): EventHandle
export declare function once(eventName: 'bookRead', callback: (event: BookReadEvent) => void): EventHandle

export declare function on(eventName: 'sell', callback: (event: SellEvent) => void): EventHandle
export declare function once(eventName: 'sell', callback: (event: SellEvent) => void): EventHandle

export declare function on(eventName: 'furnitureEnter', callback: (event: FurnitureEvent) => void): EventHandle
export declare function once(eventName: 'furnitureEnter', callback: (event: FurnitureEvent) => void): EventHandle

export declare function on(eventName: 'furnitureExit', callback: (event: FurnitureEvent) => void): EventHandle
export declare function once(eventName: 'furnitureExit', callback: (event: FurnitureEvent) => void): EventHandle

export declare function on(eventName: 'wardHit', callback: (event: WardHitEvent) => void): EventHandle
export declare function once(eventName: 'wardHit', callback: (event: WardHitEvent) => void): EventHandle

export declare function on(eventName: 'packageStart', callback: (event: PackageEvent) => void): EventHandle
export declare function once(eventName: 'packageStart', callback: (event: PackageEvent) => void): EventHandle

export declare function on(eventName: 'packageChange', callback: (event: PackageEvent) => void): EventHandle
export declare function once(eventName: 'packageChange', callback: (event: PackageEvent) => void): EventHandle

export declare function on(eventName: 'packageEnd', callback: (event: PackageEvent) => void): EventHandle
export declare function once(eventName: 'packageEnd', callback: (event: PackageEvent) => void): EventHandle

export declare function on(eventName: 'enterBleedout', callback: (event: EnterBleedoutEvent) => void): EventHandle
export declare function once(eventName: 'enterBleedout', callback: (event: EnterBleedoutEvent) => void): EventHandle

export declare function on(eventName: 'destructionStageChanged', callback: (event: DestructionStageChangedEvent) => void): EventHandle
export declare function once(eventName: 'destructionStageChanged', callback: (event: DestructionStageChangedEvent) => void): EventHandle

export declare function on(eventName: 'sceneAction', callback: (event: SceneActionEvent) => void): EventHandle
export declare function once(eventName: 'sceneAction', callback: (event: SceneActionEvent) => void): EventHandle

export declare function on(eventName: 'playerBowShot', callback: (event: PlayerBowShotEvent) => void): EventHandle
export declare function once(eventName: 'playerBowShot', callback: (event: PlayerBowShotEvent) => void): EventHandle

export declare function on(eventName: 'fastTravelEnd', callback: (event: FastTravelEnd) => void): EventHandle
export declare function once(eventName: 'fastTravelEnd', callback: (event: FastTravelEnd) => void): EventHandle

export declare function on(eventName: 'perkEntryRun', callback: (event: PerkEntryRunEvent) => void): EventHandle
export declare function once(eventName: 'perkEntryRun', callback: (event: PerkEntryRunEvent) => void): EventHandle

export declare function on(eventName: 'actionWeaponSwing', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionWeaponSwing', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionBeginDraw', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionBeginDraw', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionEndDraw', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionEndDraw', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionBowDraw', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionBowDraw', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionBowRelease', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionBowRelease', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionBeginSheathe', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionBeginSheathe', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionEndSheathe', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionEndSheathe', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionSpellCast', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionSpellCast', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionSpellFire', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionSpellFire', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionVoiceCast', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionVoiceCast', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'actionVoiceFire', callback: (event: ActionEvent) => void): EventHandle
export declare function once(eventName: 'actionVoiceFire', callback: (event: ActionEvent) => void): EventHandle

export declare function on(eventName: 'cameraStateChanged', callback: (event: CameraStateChangedEvent) => void): EventHandle
export declare function once(eventName: 'cameraStateChanged', callback: (event: CameraStateChangedEvent) => void): EventHandle

export declare function on(eventName: 'crosshairRefChanged', callback: (event: CrosshairRefChangedEvent) => void): EventHandle
export declare function once(eventName: 'crosshairRefChanged', callback: (event: CrosshairRefChangedEvent) => void): EventHandle

export declare function on(eventName: 'niNodeUpdate', callback: (event: NiNodeUpdateEvent) => void): EventHandle
export declare function once(eventName: 'niNodeUpdate', callback: (event: NiNodeUpdateEvent) => void): EventHandle

export declare function on(eventName: 'modEvent', callback: (event: ModEvent) => void): EventHandle
export declare function once(eventName: 'modEvent', callback: (event: ModEvent) => void): EventHandle

export declare function on(eventName: 'positionPlayer', callback: (event: PositionPlayerEvent) => void): EventHandle
export declare function once(eventName: 'positionPlayer', callback: (event: PositionPlayerEvent) => void): EventHandle

export declare function on(eventName: 'footstep', callback: (event: FootstepEvent) => void): EventHandle
export declare function once(eventName: 'footstep', callback: (event: FootstepEvent) => void): EventHandle

export declare function on(eventName: 'translationFailed', callback: (event: TranslationEvent) => void): EventHandle
export declare function once(eventName: 'translationFailed', callback: (event: TranslationEvent) => void): EventHandle

export declare function on(eventName: 'translationAlmostCompleted', callback: (event: TranslationEvent) => void): EventHandle
export declare function once(eventName: 'translationAlmostCompleted', callback: (event: TranslationEvent) => void): EventHandle

export declare function on(eventName: 'translationCompleted', callback: (event: TranslationEvent) => void): EventHandle
export declare function once(eventName: 'translationCompleted', callback: (event: TranslationEvent) => void): EventHandle

export declare function on(eventName: 'buttonEvent', callback: (event: ButtonEvent) => void): EventHandle
export declare function once(eventName: 'buttonEvent', callback: (event: ButtonEvent) => void): EventHandle

export declare function on(eventName: 'mouseMove', callback: (event: MouseMoveEvent) => void): EventHandle
export declare function once(eventName: 'mouseMove', callback: (event: MouseMoveEvent) => void): EventHandle

export declare function on(eventName: 'thumbstickEvent', callback: (event: ThumbstickEvent) => void): EventHandle
export declare function once(eventName: 'thumbstickEvent', callback: (event: ThumbstickEvent) => void): EventHandle

export declare function on(eventName: 'kinectEvent', callback: (event: KinectEvent) => void): EventHandle
export declare function once(eventName: 'kinectEvent', callback: (event: KinectEvent) => void): EventHandle

export declare function on(eventName: 'deviceConnect', callback: (event: DeviceConnectEvent) => void): EventHandle
export declare function once(eventName: 'deviceConnect', callback: (event: DeviceConnectEvent) => void): EventHandle

export declare function on(eventName: 'actorKill', callback: (event: ActorKillEvent) => void): EventHandle
export declare function once(eventName: 'actorKill', callback: (event: ActorKillEvent) => void): EventHandle

export declare function on(eventName: 'criticalHit', callback: (event: CriticalHitEvent) => void): EventHandle
export declare function once(eventName: 'criticalHit', callback: (event: CriticalHitEvent) => void): EventHandle

export declare function on(eventName: 'disarmedEvent', callback: (event: DisarmedEvent) => void): EventHandle
export declare function once(eventName: 'disarmedEvent', callback: (event: DisarmedEvent) => void): EventHandle

export declare function on(eventName: 'dragonSoulsGained', callback: (event: DragonSoulsGainedEvent) => void): EventHandle
export declare function once(eventName: 'dragonSoulsGained', callback: (event: DragonSoulsGainedEvent) => void): EventHandle

export declare function on(eventName: 'itemHarvested', callback: (event: ItemHarvestedEvent) => void): EventHandle
export declare function once(eventName: 'itemHarvested', callback: (event: ItemHarvestedEvent) => void): EventHandle

export declare function on(eventName: 'levelIncrease', callback: (event: LevelIncreaseEvent) => void): EventHandle
export declare function once(eventName: 'levelIncrease', callback: (event: LevelIncreaseEvent) => void): EventHandle

export declare function on(eventName: 'locationDiscovery', callback: (event: LocationDiscoveryEvent) => void): EventHandle
export declare function once(eventName: 'locationDiscovery', callback: (event: LocationDiscoveryEvent) => void): EventHandle

export declare function on(eventName: 'shoutAttack', callback: (event: ShoutAttackEvent) => void): EventHandle
export declare function once(eventName: 'shoutAttack', callback: (event: ShoutAttackEvent) => void): EventHandle

export declare function on(eventName: 'skillIncrease', callback: (event: SkillIncreaseEvent) => void): EventHandle
export declare function once(eventName: 'skillIncrease', callback: (event: SkillIncreaseEvent) => void): EventHandle

export declare function on(eventName: 'soulsTrapped', callback: (event: SoulsTrappedEvent) => void): EventHandle
export declare function once(eventName: 'soulsTrapped', callback: (event: SoulsTrappedEvent) => void): EventHandle

export declare function on(eventName: 'spellsLearned', callback: (event: SpellsLearnedEvent) => void): EventHandle
export declare function once(eventName: 'spellsLearned', callback: (event: SpellsLearnedEvent) => void): EventHandle

declare class ConsoleCommand {
  longName: string
  shortName: string
  numArgs: number
  execute: (...arguments: unknown[]) => boolean
}
export declare function findConsoleCommand(cmdName: string): ConsoleCommand | null

export const enum MarkerType {
  None = 0,
  City = 1,
  Town = 2,
  Settlement = 3,
  Cave = 4,
  Camp = 5,
  Fort = 6,
  NordicRuins = 7,
  DwemerRuin = 8,
  Shipwreck = 9,
  Grove = 10,
  Landmark = 11,
  DragonLair = 12,
  Farm = 13,
  WoodMill = 14,
  Mine = 15,
  ImperialCamp = 16,
  StormcloakCamp = 17,
  Doomstone = 18,
  WheatMill = 19,
  Smelter = 20,
  Stable = 21,
  ImperialTower = 22,
  Clearing = 23,
  Pass = 24,
  Alter = 25,
  Rock = 26,
  Lighthouse = 27,
  OrcStronghold = 28,
  GiantCamp = 29,
  Shack = 30,
  NordicTower = 31,
  NordicDwelling = 32,
  Docks = 33,
  Shrine = 34,
  RiftenCastle = 35,
  RiftenCapitol = 36,
  WindhelmCastle = 37,
  WindhelmCapitol = 38,
  WhiterunCastle = 39,
  WhiterunCapitol = 40,
  SolitudeCastle = 41,
  SolitudeCapitol = 42,
  MarkarthCastle = 43,
  MarkarthCapitol = 44,
  WinterholdCastle = 45,
  WinterholdCapitol = 46,
  MorthalCastle = 47,
  MorthalCapitol = 48,
  FalkreathCastle = 49,
  FalkreathCapitol = 50,
  DawnstarCastle = 51,
  DawnstarCapitol = 52,
  DLC02_TempleOfMiraak = 53,
  DLC02_RavenRock = 54,
  DLC02_BeastStone = 55,
  DLC02_TelMithryn = 56,
  DLC02_ToSkyrim = 57,
  DLC02_ToSolstheim = 58,
}

export const enum ActorValue {
  Aggresion = 0,
  Confidence = 1,
  Energy = 2,
  Morality = 3,
  Mood = 4,
  Assistance = 5,
  OneHanded = 6,
  TwoHanded = 7,
  Archery = 8,
  Block = 9,
  Smithing = 10,
  HeavyArmor = 11,
  LightArmor = 12,
  Pickpocket = 13,
  Lockpicking = 14,
  Sneak = 15,
  Alchemy = 16,
  Speech = 17,
  Alteration = 18,
  Conjuration = 19,
  Destruction = 20,
  Illusion = 21,
  Restoration = 22,
  Enchanting = 23,
  Health = 24,
  Magicka = 25,
  Stamina = 26,
  HealRate = 27,
  MagickaRate = 28,
  StaminaRate = 29,
  SpeedMult = 30,
  InventoryWeight = 31,
  CarryWeight = 32,
  CriticalChance = 33,
  MeleeDamage = 34,
  UnarmedDamage = 35,
  Mass = 36,
  VoicePoints = 37,
  VoiceRate = 38,
  DamageResist = 39,
  PoisonResist = 40,
  ResistFire = 41,
  ResistShock = 42,
  ResistFrost = 43,
  ResistMagic = 44,
  ResistDisease = 45,
  Unknown46 = 46,
  Unknown47 = 47,
  Unknown48 = 48,
  Unknown49 = 49,
  Unknown50 = 50,
  Unknown51 = 51,
  Unknown52 = 52,
  Paralysis = 53,
  Invisibility = 54,
  NightEye = 55,
  DetectLifeRange = 56,
  WaterBreathing = 57,
  WaterWalking = 58,
  Unknown59 = 59,
  Fame = 60,
  Infamy = 61,
  JumpingBonus = 62,
  WardPower = 63,
  RightItemCharge = 64,
  ArmorPerks = 65,
  ShieldPerks = 66,
  WardDeflection = 67,
  Variable01 = 68,
  Variable02 = 69,
  Variable03 = 70,
  Variable04 = 71,
  Variable05 = 72,
  Variable06 = 73,
  Variable07 = 74,
  Variable08 = 75,
  Variable09 = 76,
  Variable10 = 77,
  BowSpeedBonus = 78,
  FavorActive = 79,
  FavorsPerDay = 80,
  FavorsPerDayTimer = 81,
  LeftItemCharge = 82,
  AbsorbChance = 83,
  Blindness = 84,
  WeaponSpeedMult = 85,
  ShoutRecoveryMult = 86,
  BowStaggerBonus = 87,
  Telekinesis = 88,
  FavorPointsBonus = 89,
  LastBribedIntimidated = 90,
  LastFlattered = 91,
  MovementNoiseMult = 92,
  BypassVendorStolenCheck = 93,
  BypassVendorKeywordCheck = 94,
  WaitingForPlayer = 95,
  OneHandedModifier = 96,
  TwoHandedModifier = 97,
  MarksmanModifier = 98,
  BlockModifier = 99,
  SmithingModifier = 100,
  HeavyArmorModifier = 101,
  LightArmorModifier = 102,
  PickpocketModifier = 103,
  LockpickingModifier = 104,
  SneakingModifier = 105,
  AlchemyModifier = 106,
  SpeechcraftModifier = 107,
  AlterationModifier = 108,
  ConjurationModifier = 109,
  DestructionModifier = 110,
  IllusionModifier = 111,
  RestorationModifier = 112,
  EnchantingModifier = 113,
  OneHandedSkillAdvance = 114,
  TwoHandedSkillAdvance = 115,
  MarksmanSkillAdvance = 116,
  BlockSkillAdvance = 117,
  SmithingSkillAdvance = 118,
  HeavyArmorSkillAdvance = 119,
  LightArmorSkillAdvance = 120,
  PickpocketSkillAdvance = 121,
  LockpickingSkillAdvance = 122,
  SneakingSkillAdvance = 123,
  AlchemySkillAdvance = 124,
  SpeechcraftSkillAdvance = 125,
  AlterationSkillAdvance = 126,
  ConjurationSkillAdvance = 127,
  DestructionSkillAdvance = 128,
  IllusionSkillAdvance = 129,
  RestorationSkillAdvance = 130,
  EnchantingSkillAdvance = 131,
  LeftWeaponSpeedMultiply = 132,
  DragonSouls = 133,
  CombatHealthRegenMultiply = 134,
  OneHandedPowerModifier = 135,
  TwoHandedPowerModifier = 136,
  MarksmanPowerModifier = 137,
  BlockPowerModifier = 138,
  SmithingPowerModifier = 139,
  HeavyArmorPowerModifier = 140,
  LightArmorPowerModifier = 141,
  PickpocketPowerModifier = 142,
  LockpickingPowerModifier = 143,
  SneakingPowerModifier = 144,
  AlchemyPowerModifier = 145,
  SpeechcraftPowerModifier = 146,
  AlterationPowerModifier = 147,
  ConjurationPowerModifier = 148,
  DestructionPowerModifier = 149,
  IllusionPowerModifier = 150,
  RestorationPowerModifier = 151,
  EnchantingPowerModifier = 152,
  DragonRend = 153,
  AttackDamageMult = 154,
  HealRateMult = 155,
  MagickaRateMult = 156,
  StaminaRateMult = 157,
  WerewolfPerks = 158,
  VampirePerks = 159,
  GrabActorOffset = 160,
  Grabbed = 161,
  Unknown162 = 162,
  ReflectDamage = 163,
}

export const enum SlotType {
  Left = 1,
  Right = 2,
  Voice = 3,
}

export const enum WardHitStatus {
  Friendly = 0,
  Absorbed = 1,
  Broken = 2,
}

export const enum PlayerPositionEventType {
  PreCellTransition = 0,
  PreUpdatePackages = 1,
  PostUpdatePackages = 2,
  PostCellTransition = 3,
  CellTransitionFinish = 4,
}

export const enum InputDeviceType {
  Keyboard = 0,
  Mouse = 1,
  Gamepad = 2,
  VirtualKeyboard = 3,
}

export const enum MotionType {
  Dynamic = 1,
  SphereInertia = 2,
  BoxInertia = 3,
  Keyframed = 4,
  Fixed = 5,
  ThinBoxInertia = 6,
  Character = 7,
}

export const enum Menu {
  Barter = 'BarterMenu',
  Book = 'Book Menu',
  Console = 'Console',
  ConsoleNativeUI = 'Console Native UI Menu',
  Container = 'ContainerMenu',
  Crafting = 'Crafting Menu',
  Credits = 'Credits Menu',
  Cursor = 'Cursor Menu',
  Debug = 'Debug Text Menu',
  Dialogue = 'Dialogue Menu',
  Fader = 'Fader Menu',
  Favorites = 'FavoritesMenu',
  Gift = 'GiftMenu',
  HUD = 'HUD Menu',
  Inventory = 'InventoryMenu',
  Journal = 'Journal Menu',
  Kinect = 'Kinect Menu',
  LevelUp = 'LevelUp Menu',
  Loading = 'Loading Menu',
  Main = 'Main Menu',
  Lockpicking = 'Lockpicking Menu',
  Magic = 'MagicMenu',
  Map = 'MapMenu',
  MessageBox = 'MessageBoxMenu',
  Mist = 'Mist Menu',
  OverlayInteraction = 'Overlay Interaction Menu',
  Overlay = 'Overlay Menu',
  Quantity = 'Quantity Menu',
  RaceSex = 'RaceSex Menu',
  Sleep = 'Sleep/Wait Menu',
  Stats = 'StatsMenu',
  TitleSequence = 'TitleSequence Menu',
  Top = 'Top Menu',
  Training = 'Training Menu',
  Tutorial = 'Tutorial Menu',
  Tween = 'TweenMenu',
}

export const enum DxScanCode {
  None,
  Escape,
  N1,
  N2,
  N3,
  N4,
  N5,
  N6,
  N7,
  N8,
  N9,
  N0,
  Minus,
  Equals,
  Backspace,
  Tab,
  Q,
  W,
  E,
  R,
  T,
  Y,
  U,
  I,
  O,
  P,
  LeftBracket,
  RightBracket,
  Enter,
  LeftControl,
  A,
  S,
  D,
  F,
  G,
  H,
  J,
  K,
  L,
  Semicolon,
  Apostrophe,
  Console,
  LeftShift,
  BackSlash,
  Z,
  X,
  C,
  V,
  B,
  N,
  M,
  Comma,
  Period,
  ForwardSlash,
  RightShift,
  NumMult,
  LeftAlt,
  Spacebar,
  CapsLock,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  NumLock,
  ScrollLock,
  Num7,
  Num8,
  Num9,
  NumMinus,
  Num4,
  Num5,
  Num6,
  NumPlus,
  Num1,
  Num2,
  Num3,
  Num0,
  NumDot,
  F11 = 87,
  F12,
  NumEnter = 156,
  RightControl,
  NumSlash = 181,
  SysRqPtrScr = 183,
  RightAlt,
  Pause = 197,
  Home = 199,
  UpArrow,
  PgUp,
  LeftArrow = 203,
  RightArrow = 205,
  End = 207,
  DownArrow,
  PgDown,
  Insert,
  Delete,
  LeftMouseButton = 256,
  RightMouseButton,
  MiddleMouseButton,
  MouseButton3,
  MouseButton4,
  MouseButton5,
  MouseButton6,
  MouseButton7,
  MouseWheelUp,
  MouseWheelDown,
}

export const enum FormType {
  ANIO = 83,
  ARMA = 102,
  AcousticSpace = 16,
  Action = 6,
  Activator = 24,
  ActorValueInfo = 95,
  AddonNode = 94,
  Ammo = 42,
  Apparatus = 33,
  Armor = 26,
  ArrowProjectile = 64,
  Art = 125,
  AssociationType = 123,
  BarrierProjectile = 69,
  BeamProjectile = 66,
  BodyPartData = 93,
  Book = 27,
  CameraPath = 97,
  CameraShot = 96,
  Cell = 60,
  Character = 62,
  Class = 10,
  Climate = 55,
  CollisionLayer = 132,
  ColorForm = 133,
  CombatStyle = 80,
  ConeProjectile = 68,
  ConstructibleObject = 49,
  Container = 28,
  DLVW = 117,
  Debris = 88,
  DefaultObject = 107,
  DialogueBranch = 115,
  Door = 29,
  DualCastData = 129,
  EffectSetting = 18,
  EffectShader = 85,
  Enchantment = 21,
  EncounterZone = 103,
  EquipSlot = 120,
  Explosion = 87,
  Eyes = 13,
  Faction = 11,
  FlameProjectile = 67,
  Flora = 39,
  Footstep = 110,
  FootstepSet = 111,
  Furniture = 40,
  GMST = 3,
  Global = 9,
  Grass = 37,
  GrenadeProjectile = 65,
  Group = 2,
  Hazard = 51,
  HeadPart = 12,
  Idle = 78,
  IdleMarker = 47,
  ImageSpace = 89,
  ImageSpaceModifier = 90,
  ImpactData = 100,
  ImpactDataSet = 101,
  Ingredient = 30,
  Key = 45,
  Keyword = 4,
  Land = 72,
  LandTexture = 20,
  LeveledCharacter = 44,
  LeveledItem = 53,
  LeveledSpell = 82,
  Light = 31,
  LightingTemplate = 108,
  List = 91,
  LoadScreen = 81,
  Location = 104,
  LocationRef = 5,
  Material = 126,
  MaterialType = 99,
  MenuIcon = 8,
  Message = 105,
  Misc = 32,
  MissileProjectile = 63,
  MovableStatic = 36,
  MovementType = 127,
  MusicTrack = 116,
  MusicType = 109,
  NAVI = 59,
  NPC = 43,
  NavMesh = 73,
  None = 0,
  Note = 48,
  Outfit = 124,
  PHZD = 70,
  Package = 79,
  Perk = 92,
  Potion = 46,
  Projectile = 50,
  Quest = 77,
  Race = 14,
  Ragdoll = 106,
  Reference = 61,
  ReferenceEffect = 57,
  Region = 58,
  Relationship = 121,
  ReverbParam = 134,
  Scene = 122,
  Script = 19,
  ScrollItem = 23,
  ShaderParticleGeometryData = 56,
  Shout = 119,
  Skill = 17,
  SoulGem = 52,
  Sound = 15,
  SoundCategory = 130,
  SoundDescriptor = 128,
  SoundOutput = 131,
  Spell = 22,
  Static = 34,
  StaticCollection = 35,
  StoryBranchNode = 112,
  StoryEventNode = 114,
  StoryQuestNode = 113,
  TES4 = 1,
  TLOD = 74,
  TOFT = 86,
  TalkingActivator = 25,
  TextureSet = 7,
  Topic = 75,
  TopicInfo = 76,
  Tree = 38,
  VoiceType = 98,
  Water = 84,
  Weapon = 41,
  Weather = 54,
  WordOfPower = 118,
  WorldSpace = 71,
}

export const enum WeaponType {
  Fist = 0,
  Sword,
  Dagger,
  WarAxe,
  Mace,
  Greatsword,
  Battleaxe = 6,
  Warhammer = 6,
  Bow,
  Staff,
  Crossbow,
}

export const enum EquippedItemType {
  Fist = 0,
  Sword,
  Dagger,
  WarAxe,
  Mace,
  Greatsword,
  Battleaxe = 6,
  Warhammer = 6,
  Bow,
  Staff,
  Spell,
  Shield,
  Torch,
  Crossbow,
}

export const enum SlotMask {
  Head = 0x1,
  Hair = 0x2,
  Body = 0x4,
  Hands = 0x8,
  Forearms = 0x10,
  Amulet = 0x20,
  Ring = 0x40,
  Feet = 0x80,
  Calves = 0x100,
  Shield = 0x200,
  Tail = 0x400,
  LongHair = 0x800,
  Circlet = 0x1000,
  Ears = 0x2000,
  Face = 0x4000,
  Mouth = 0x4000,
  Neck = 0x8000,
  ChestOuter = 0x10000,
  ChestPrimary = 0x10000,
  Back = 0x20000,
  Misc01 = 0x40000,
  PelvisOuter = 0x80000,
  PelvisPrimary = 0x80000,
  DecapitateHead = 0x100000,
  Decapitate = 0x200000,
  PelvisUnder = 0x400000,
  PelvisSecondary = 0x400000,
  LegOuter = 0x800000,
  LegPrimary = 0x800000,
  LegUnder = 0x1000000,
  LegSecondary = 0x1000000,
  FaceAlt = 0x2000000,
  Jewelry = 0x2000000,
  ChestUnder = 0x4000000,
  ChestSecondary = 0x4000000,
  Shoulder = 0x8000000,
  ArmUnder = 0x10000000,
  ArmSecondary = 0x10000000,
  ArmLeft = 0x10000000,
  ArmOuter = 0x20000000,
  ArmPrimary = 0x20000000,
  ArmRight = 0x20000000,
  Misc02 = 0x40000000,
  FX01 = 0x80000000,
}

export const enum SpriteEffects {
  None = 0,
  FlipHorizontally = 1,
  FlipVertically = 2
}

export declare namespace SendAnimationEventHook {
  class Context {
    readonly selfId: number
    animEventName: string

    readonly storage: Map<string, unknown>
  }

  class LeaveContext extends Context {
    readonly animationSucceeded: boolean
  }

  class Handler {
    enter(ctx: Context): void
    leave(ctx: LeaveContext): void
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  class Target {
    add(handler: Handler, minSelfId?: number, maxSelfId?: number, eventPattern?: string): number
    remove(id: number): void
  }
}

export declare namespace SendPapyrusEventHook {
  class Context {
    readonly selfId: number
    readonly papyrusEventName: string

    readonly storage: Map<string, unknown>
  }

  class Handler {
    enter(ctx: Context): void
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  class Target {
    add(handler: Handler, minSelfId?: number, maxSelfId?: number, eventPattern?: string): number
    remove(id: number): void
  }
}

export declare class Hooks {
  sendAnimationEvent: SendAnimationEventHook.Target
  sendPapyrusEvent: SendPapyrusEventHook.Target
}

export declare let hooks: Hooks

export declare class HttpResponse {
    body: string;
    status: number;
    error: string;
}
export type HttpHeaders = Record<string, string>

export declare class HttpClient {
    constructor(url: string);
    get(path: string, options?: { headers?: HttpHeaders }, callback?: (result: HttpResponse) => void): Promise<HttpResponse>;
    post(path: string, options: { body: string, contentType: string, headers?: HttpHeaders }, callback?: (result: HttpResponse) => void): Promise<HttpResponse>;
}

export declare function createText(xPos: number, yPos: number, text: string, color: number[], name?: string): number; //default name is Tavern
export declare function destroyText(textId: number): void;
export declare function destroyAllTexts(): void;

export declare function setTextPos(textId: number, xPos: number, yPos: number): void;
export declare function setTextString(textId: number, text: string): void;
export declare function setTextColor(textId: number, color: number[]): void;
export declare function setTextSize(textId: number, size: number): void;
export declare function setTextRotation(textId: number, rotation: number): void;
export declare function setTextFont(textId: number, name: string): void;
export declare function setTextDepth(textId: number, depth: number): void;
export declare function setTextEffect(textId: number, effect: number): void;
export declare function setTextOrigin(textId: number, origin: number[]): void;

export declare function getTextPos(textId: number): number[];
export declare function getTextString(textId: number): string;
export declare function getTextColor(textId: number): number[];
export declare function getTextSize(textId: number): number;
export declare function getTextRotation(textId: number): number;
export declare function getTextFont(textId: number): string;
export declare function getTextDepth(textId: number): number;
export declare function getTextEffect(textId: number): number;
export declare function getTextOrigin(textId: number): number[];

export declare function getNumCreatedTexts(): number;

export declare function getFileInfo(filename: string): { crc32: number, size: number };

export interface Extra {
  health?: number;
  enchantmentId?: number;
  maxCharge?: number;
  removeEnchantmentOnUnequip?: boolean;
  chargePercent?: number;
  name?: string;
  soul?: 0 | 1 | 2 | 3 | 4 | 5;
  poisonId?: number;
  poisonCount?: number;
  worn?: boolean;
  wornLeft?: boolean;
}

export interface BasicEntry {
  baseId: number;
  count: number;
}

export type Entry = BasicEntry & Extra;

export interface Inventory {
  entries: Entry[];
}

export declare function setInventory(formId: number, inventory: Inventory): void;

// Based on Form.pex
export declare class Form extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Form | null
  getFormID(): number
  getGoldValue(): number
  getKeywords(): PapyrusObject[] | null
  getName(): string
  getNthKeyword(index: number): Keyword | null
  getNumKeywords(): number
  getType(): number
  getWeight(): number
  getWorldModelNthTextureSet(n: number): TextureSet | null
  getWorldModelNumTextureSets(): number
  getWorldModelPath(): string
  hasKeyword(akKeyword: Keyword | null): boolean
  hasWorldModel(): boolean
  isPlayable(): boolean
  playerKnows(): boolean
  registerForActorAction(actionType: number): void
  registerForAnimationEvent(akSender: ObjectReference | null, asEventName: string): boolean
  registerForCameraState(): void
  registerForControl(control: string): void
  registerForCrosshairRef(): void
  registerForKey(keyCode: number): void
  registerForLOS(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForMenu(menuName: string): void
  registerForModEvent(eventName: string, callbackName: string): void
  registerForNiNodeUpdate(): void
  registerForSingleLOSGain(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForSingleLOSLost(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForSingleUpdate(afInterval: number): void
  registerForSingleUpdateGameTime(afInterval: number): void
  registerForSleep(): void
  registerForTrackedStatsEvent(): void
  registerForUpdate(afInterval: number): void
  registerForUpdateGameTime(afInterval: number): void
  sendModEvent(eventName: string, strArg: string, numArg: number): void
  setGoldValue(value: number): void
  setName(name: string): void
  setPlayerKnows(knows: boolean): void
  setWeight(weight: number): void
  setWorldModelNthTextureSet(nSet: TextureSet | null, n: number): void
  setWorldModelPath(path: string): void
  startObjectProfiling(): void
  stopObjectProfiling(): void
  tempClone(): Form | null
  unregisterForActorAction(actionType: number): void
  unregisterForAllControls(): void
  unregisterForAllKeys(): void
  unregisterForAllMenus(): void
  unregisterForAllModEvents(): void
  unregisterForAnimationEvent(akSender: ObjectReference | null, asEventName: string): void
  unregisterForCameraState(): void
  unregisterForControl(control: string): void
  unregisterForCrosshairRef(): void
  unregisterForKey(keyCode: number): void
  unregisterForLOS(akViewer: Actor | null, akTarget: ObjectReference | null): void
  unregisterForMenu(menuName: string): void
  unregisterForModEvent(eventName: string): void
  unregisterForNiNodeUpdate(): void
  unregisterForSleep(): void
  unregisterForTrackedStatsEvent(): void
  unregisterForUpdate(): void
  unregisterForUpdateGameTime(): void
}

// Based on Action.pex
export declare class Action extends Form {
  static from(papyrusObject: PapyrusObject | null): Action | null
}

// Based on Activator.pex
export declare class Activator extends Form {
  static from(papyrusObject: PapyrusObject | null): Activator | null
}

// Based on ActiveMagicEffect.pex
export declare class ActiveMagicEffect extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): ActiveMagicEffect | null
  addInventoryEventFilter(akFilter: Form | null): void
  dispel(): void
  getBaseObject(): MagicEffect | null
  getCasterActor(): Actor | null
  getDuration(): number
  getMagnitude(): number
  getTargetActor(): Actor | null
  getTimeElapsed(): number
  registerForActorAction(actionType: number): void
  registerForAnimationEvent(akSender: ObjectReference | null, asEventName: string): boolean
  registerForCameraState(): void
  registerForControl(control: string): void
  registerForCrosshairRef(): void
  registerForKey(keyCode: number): void
  registerForLOS(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForMenu(menuName: string): void
  registerForModEvent(eventName: string, callbackName: string): void
  registerForNiNodeUpdate(): void
  registerForSingleLOSGain(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForSingleLOSLost(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForSingleUpdate(afInterval: number): void
  registerForSingleUpdateGameTime(afInterval: number): void
  registerForSleep(): void
  registerForTrackedStatsEvent(): void
  registerForUpdate(afInterval: number): void
  registerForUpdateGameTime(afInterval: number): void
  removeAllInventoryEventFilters(): void
  removeInventoryEventFilter(akFilter: Form | null): void
  sendModEvent(eventName: string, strArg: string, numArg: number): void
  startObjectProfiling(): void
  stopObjectProfiling(): void
  unregisterForActorAction(actionType: number): void
  unregisterForAllControls(): void
  unregisterForAllKeys(): void
  unregisterForAllMenus(): void
  unregisterForAllModEvents(): void
  unregisterForAnimationEvent(akSender: ObjectReference | null, asEventName: string): void
  unregisterForCameraState(): void
  unregisterForControl(control: string): void
  unregisterForCrosshairRef(): void
  unregisterForKey(keyCode: number): void
  unregisterForLOS(akViewer: Actor | null, akTarget: ObjectReference | null): void
  unregisterForMenu(menuName: string): void
  unregisterForModEvent(eventName: string): void
  unregisterForNiNodeUpdate(): void
  unregisterForSleep(): void
  unregisterForTrackedStatsEvent(): void
  unregisterForUpdate(): void
  unregisterForUpdateGameTime(): void
}

// Based on ObjectReference.pex
export declare class ObjectReference extends Form {
  static from(papyrusObject: PapyrusObject | null): ObjectReference | null
  activate(akActivator: ObjectReference | null, abDefaultProcessingOnly: boolean): boolean
  addDependentAnimatedObjectReference(akDependent: ObjectReference | null): boolean
  addInventoryEventFilter(akFilter: Form | null): void
  addItem(akItemToAdd: Form | null, aiCount: number, abSilent: boolean): void
  addToMap(abAllowFastTravel: boolean): void
  applyHavokImpulse(afX: number, afY: number, afZ: number, afMagnitude: number): Promise<void>
  blockActivation(abBlocked: boolean): void
  calculateEncounterLevel(aiDifficulty: number): number
  canFastTravelToMarker(): boolean
  clearDestruction(): void
  createDetectionEvent(akOwner: Actor | null, aiSoundLevel: number): void
  createEnchantment(maxCharge: number, effects: PapyrusObject[] | null, magnitudes: number[] | null, areas: number[] | null, durations: number[] | null): void
  damageObject(afDamage: number): Promise<void>
  delete(): Promise<void>
  disable(abFadeOut: boolean): Promise<void>
  disableNoWait(abFadeOut: boolean): void
  dropObject(akObject: Form | null, aiCount: number): Promise<ObjectReference | null>
  enable(abFadeIn: boolean): Promise<void>
  enableFastTravel(abEnable: boolean): void
  enableNoWait(abFadeIn: boolean): void
  forceAddRagdollToWorld(): Promise<void>
  forceRemoveRagdollFromWorld(): Promise<void>
  getActorOwner(): ActorBase | null
  getAllForms(toFill: FormList | null): void
  getAngleX(): number
  getAngleY(): number
  getAngleZ(): number
  getAnimationVariableBool(arVariableName: string): boolean
  getAnimationVariableFloat(arVariableName: string): number
  getAnimationVariableInt(arVariableName: string): number
  getBaseObject(): Form | null
  getContainerForms(): PapyrusObject[] | null
  getCurrentDestructionStage(): number
  getCurrentLocation(): Location | null
  getCurrentScene(): Scene | null
  getDisplayName(): string
  getEditorLocation(): Location | null
  getEnableParent(): ObjectReference | null
  getEnchantment(): Enchantment | null
  getFactionOwner(): Faction | null
  getHeadingAngle(akOther: ObjectReference | null): number
  getHeight(): number
  getItemCharge(): number
  getItemCount(akItem: Form | null): number
  getItemHealthPercent(): number
  getItemMaxCharge(): number
  getKey(): Key | null
  getLength(): number
  getLinkedRef(apKeyword: Keyword | null): ObjectReference | null
  getLockLevel(): number
  getMass(): number
  getNthForm(index: number): Form | null
  getNthLinkedRef(aiLinkedRef: number): ObjectReference | null
  getNthReferenceAlias(n: number): ReferenceAlias | null
  getNumItems(): number
  getNumReferenceAliases(): number
  getOpenState(): number
  getParentCell(): Cell | null
  getPoison(): Potion | null
  getPositionX(): number
  getPositionY(): number
  getPositionZ(): number
  getReferenceAliases(): PapyrusObject[] | null
  getScale(): number
  getTotalArmorWeight(): number
  getTotalItemWeight(): number
  getTriggerObjectCount(): number
  getVoiceType(): VoiceType | null
  getWidth(): number
  getWorldSpace(): WorldSpace | null
  hasEffectKeyword(akKeyword: Keyword | null): boolean
  hasNode(asNodeName: string): boolean
  hasRefType(akRefType: LocationRefType | null): boolean
  ignoreFriendlyHits(abIgnore: boolean): void
  interruptCast(): void
  is3DLoaded(): boolean
  isActivateChild(akChild: ObjectReference | null): boolean
  isActivationBlocked(): boolean
  isDeleted(): boolean
  isDisabled(): boolean
  isFurnitureInUse(abIgnoreReserved: boolean): boolean
  isFurnitureMarkerInUse(aiMarker: number, abIgnoreReserved: boolean): boolean
  isHarvested(): boolean
  isIgnoringFriendlyHits(): boolean
  isInDialogueWithPlayer(): boolean
  isLockBroken(): boolean
  isLocked(): boolean
  isMapMarkerVisible(): boolean
  isOffLimits(): boolean
  knockAreaEffect(afMagnitude: number, afRadius: number): void
  lock(abLock: boolean, abAsOwner: boolean): void
  moveTo(akTarget: ObjectReference | null, afXOffset: number, afYOffset: number, afZOffset: number, abMatchRotation: boolean): Promise<void>
  moveToInteractionLocation(akTarget: ObjectReference | null): Promise<void>
  moveToMyEditorLocation(): Promise<void>
  moveToNode(akTarget: ObjectReference | null, asNodeName: string): Promise<void>
  placeActorAtMe(akActorToPlace: ActorBase | null, aiLevelMod: number, akZone: EncounterZone | null): Actor | null
  placeAtMe(akFormToPlace: Form | null, aiCount: number, abForcePersist: boolean, abInitiallyDisabled: boolean): ObjectReference | null
  playAnimation(asAnimation: string): boolean
  playAnimationAndWait(asAnimation: string, asEventName: string): Promise<boolean>
  playGamebryoAnimation(asAnimation: string, abStartOver: boolean, afEaseInTime: number): boolean
  playImpactEffect(akImpactEffect: ImpactDataSet | null, asNodeName: string, afPickDirX: number, afPickDirY: number, afPickDirZ: number, afPickLength: number, abApplyNodeRotation: boolean, abUseNodeLocalRotation: boolean): boolean
  playSyncedAnimationAndWaitSS(asAnimation1: string, asEvent1: string, akObj2: ObjectReference | null, asAnimation2: string, asEvent2: string): Promise<boolean>
  playSyncedAnimationSS(asAnimation1: string, akObj2: ObjectReference | null, asAnimation2: string): boolean
  playTerrainEffect(asEffectModelName: string, asAttachBoneName: string): void
  processTrapHit(akTrap: ObjectReference | null, afDamage: number, afPushback: number, afXVel: number, afYVel: number, afZVel: number, afXPos: number, afYPos: number, afZPos: number, aeMaterial: number, afStagger: number): void
  pushActorAway(akActorToPush: Actor | null, aiKnockbackForce: number): void
  removeAllInventoryEventFilters(): void
  removeAllItems(akTransferTo: ObjectReference | null, abKeepOwnership: boolean, abRemoveQuestItems: boolean): void
  removeDependentAnimatedObjectReference(akDependent: ObjectReference | null): boolean
  removeInventoryEventFilter(akFilter: Form | null): void
  removeItem(akItemToRemove: Form | null, aiCount: number, abSilent: boolean, akOtherContainer: ObjectReference | null): void
  reset(akTarget: ObjectReference | null): Promise<void>
  resetInventory(): void
  say(akTopicToSay: Topic | null, akActorToSpeakAs: Actor | null, abSpeakInPlayersHead: boolean): void
  sendStealAlarm(akThief: Actor | null): void
  setActorCause(akActor: Actor | null): void
  setActorOwner(akActorBase: ActorBase | null): void
  setAngle(afXAngle: number, afYAngle: number, afZAngle: number): Promise<void>
  setAnimationVariableBool(arVariableName: string, abNewValue: boolean): void
  setAnimationVariableFloat(arVariableName: string, afNewValue: number): void
  setAnimationVariableInt(arVariableName: string, aiNewValue: number): void
  setDestroyed(abDestroyed: boolean): void
  setDisplayName(name: string, force: boolean): boolean
  setEnchantment(source: Enchantment | null, maxCharge: number): void
  setFactionOwner(akFaction: Faction | null): void
  setHarvested(harvested: boolean): void
  setItemCharge(charge: number): void
  setItemHealthPercent(health: number): void
  setItemMaxCharge(maxCharge: number): void
  setLockLevel(aiLockLevel: number): void
  setMotionType(aeMotionType: MotionType, abAllowActivate: boolean): Promise<void>
  setNoFavorAllowed(abNoFavor: boolean): void
  setOpen(abOpen: boolean): void
  setPosition(afX: number, afY: number, afZ: number): Promise<void>
  setScale(afScale: number): Promise<void>
  splineTranslateTo(afX: number, afY: number, afZ: number, afXAngle: number, afYAngle: number, afZAngle: number, afTangentMagnitude: number, afSpeed: number, afMaxRotationSpeed: number): void
  splineTranslateToRefNode(arTarget: ObjectReference | null, arNodeName: string, afTangentMagnitude: number, afSpeed: number, afMaxRotationSpeed: number): void
  stopTranslation(): void
  tetherToHorse(akHorse: ObjectReference | null): void
  translateTo(afX: number, afY: number, afZ: number, afXAngle: number, afYAngle: number, afZAngle: number, afSpeed: number, afMaxRotationSpeed: number): void
  waitForAnimationEvent(asEventName: string): Promise<boolean>
  getDistance(akOther: ObjectReference | null): number
}

// Based on Actor.pex
export declare class Actor extends ObjectReference {
  static from(papyrusObject: PapyrusObject | null): Actor | null
  addPerk(akPerk: Perk | null): void
  addShout(akShout: Shout | null): boolean
  addSpell(akSpell: Spell | null, abVerbose: boolean): boolean
  allowBleedoutDialogue(abCanTalk: boolean): void
  allowPCDialogue(abTalk: boolean): void
  attachAshPile(akAshPileBase: Form | null): void
  canFlyHere(): boolean
  changeHeadPart(hPart: HeadPart | null): void
  clearArrested(): void
  clearExpressionOverride(): void
  clearExtraArrows(): void
  clearForcedMovement(): void
  clearKeepOffsetFromActor(): void
  clearLookAt(): void
  damageActorValue(asValueName: string, afDamage: number): void
  dismount(): boolean
  dispelAllSpells(): void
  dispelSpell(akSpell: Spell | null): boolean
  doCombatSpellApply(akSpell: Spell | null, akTarget: ObjectReference | null): void
  drawWeapon(): void
  enableAI(abEnable: boolean): void
  endDeferredKill(): void
  equipItem(akItem: Form | null, abPreventRemoval: boolean, abSilent: boolean): void
  equipItemById(item: Form | null, itemId: number, equipSlot: number, preventUnequip: boolean, equipSound: boolean): void
  equipItemEx(item: Form | null, equipSlot: number, preventUnequip: boolean, equipSound: boolean): void
  equipShout(akShout: Shout | null): void
  equipSpell(akSpell: Spell | null, aiSource: number): void
  evaluatePackage(): void
  forceActorValue(asValueName: string, afNewValue: number): void
  forceMovementDirection(afXAngle: number, afYAngle: number, afZAngle: number): void
  forceMovementDirectionRamp(afXAngle: number, afYAngle: number, afZAngle: number, afRampTime: number): void
  forceMovementRotationSpeed(afXMult: number, afYMult: number, afZMult: number): void
  forceMovementRotationSpeedRamp(afXMult: number, afYMult: number, afZMult: number, afRampTime: number): void
  forceMovementSpeed(afSpeedMult: number): void
  forceMovementSpeedRamp(afSpeedMult: number, afRampTime: number): void
  forceTargetAngle(afXAngle: number, afYAngle: number, afZAngle: number): void
  forceTargetDirection(afXAngle: number, afYAngle: number, afZAngle: number): void
  forceTargetSpeed(afSpeed: number): void
  getActorValue(asValueName: string): number
  getActorValueMax(asValueName: string): number
  getActorValuePercentage(asValueName: string): number
  getBaseActorValue(asValueName: string): number
  getBribeAmount(): number
  getCombatState(): number
  getCombatTarget(): Actor | null
  getCrimeFaction(): Faction | null
  getCurrentPackage(): Package | null
  getDialogueTarget(): Actor | null
  getEquippedArmorInSlot(aiSlot: number): Armor | null
  getEquippedItemId(Location: number): number
  getEquippedItemType(aiHand: number): number
  getEquippedObject(Location: number): Form | null
  getEquippedShield(): Armor | null
  getEquippedShout(): Shout | null
  getEquippedSpell(aiSource: number): Spell | null
  getEquippedWeapon(abLeftHand: boolean): Weapon | null
  getFactionRank(akFaction: Faction | null): number
  getFactionReaction(akOther: Actor | null): number
  getFactions(minRank: number, maxRank: number): PapyrusObject[] | null
  getFlyingState(): number
  getForcedLandingMarker(): ObjectReference | null
  getFurnitureReference(): ObjectReference | null
  getGoldAmount(): number
  getHighestRelationshipRank(): number
  getKiller(): Actor | null
  getLevel(): number
  getLeveledActorBase(): ActorBase | null
  getLightLevel(): number
  getLowestRelationshipRank(): number
  getNoBleedoutRecovery(): boolean
  getNthSpell(n: number): Spell | null
  getPlayerControls(): boolean
  getRace(): Race | null
  getRelationshipRank(akOther: Actor | null): number
  getSitState(): number
  getSleepState(): number
  getSpellCount(): number
  getVoiceRecoveryTime(): number
  getWarmthRating(): number
  getWornForm(slotMask: number): Form | null
  getWornItemId(slotMask: number): number
  hasAssociation(akAssociation: AssociationType | null, akOther: Actor | null): boolean
  hasFamilyRelationship(akOther: Actor | null): boolean
  hasLOS(akOther: ObjectReference | null): boolean
  hasMagicEffect(akEffect: MagicEffect | null): boolean
  hasMagicEffectWithKeyword(akKeyword: Keyword | null): boolean
  hasParentRelationship(akOther: Actor | null): boolean
  hasPerk(akPerk: Perk | null): boolean
  hasSpell(akForm: Form | null): boolean
  isAIEnabled(): boolean
  isAlarmed(): boolean
  isAlerted(): boolean
  isAllowedToFly(): boolean
  isArrested(): boolean
  isArrestingTarget(): boolean
  isBeingRidden(): boolean
  isBleedingOut(): boolean
  isBribed(): boolean
  isChild(): boolean
  isCommandedActor(): boolean
  isDead(): boolean
  isDetectedBy(akOther: Actor | null): boolean
  isDoingFavor(): boolean
  isEquipped(akItem: Form | null): boolean
  isEssential(): boolean
  isFlying(): boolean
  isGhost(): boolean
  isGuard(): boolean
  isHostileToActor(akActor: Actor | null): boolean
  isInCombat(): boolean
  isInFaction(akFaction: Faction | null): boolean
  isInKillMove(): boolean
  isIntimidated(): boolean
  isOnMount(): boolean
  isOverEncumbered(): boolean
  isPlayerTeammate(): boolean
  isPlayersLastRiddenHorse(): boolean
  isRunning(): boolean
  isSneaking(): boolean
  isSprinting(): boolean
  isSwimming(): boolean
  isTrespassing(): boolean
  isUnconscious(): boolean
  isWeaponDrawn(): boolean
  keepOffsetFromActor(arTarget: Actor | null, afOffsetX: number, afOffsetY: number, afOffsetZ: number, afOffsetAngleX: number, afOffsetAngleY: number, afOffsetAngleZ: number, afCatchUpRadius: number, afFollowRadius: number): void
  kill(akKiller: Actor | null): void
  killSilent(akKiller: Actor | null): void
  modActorValue(asValueName: string, afAmount: number): void
  modFactionRank(akFaction: Faction | null, aiMod: number): void
  moveToPackageLocation(): Promise<void>
  openInventory(abForceOpen: boolean): void
  pathToReference(aTarget: ObjectReference | null, afWalkRunPercent: number): Promise<boolean>
  playIdle(akIdle: Idle | null): boolean
  playIdleWithTarget(akIdle: Idle | null, akTarget: ObjectReference | null): boolean
  playSubGraphAnimation(asEventName: string): void
  queueNiNodeUpdate(): void
  regenerateHead(): void
  removeFromAllFactions(): void
  removeFromFaction(akFaction: Faction | null): void
  removePerk(akPerk: Perk | null): void
  removeShout(akShout: Shout | null): boolean
  removeSpell(akSpell: Spell | null): boolean
  replaceHeadPart(oPart: HeadPart | null, newPart: HeadPart | null): void
  resetAI(): void
  resetExpressionOverrides(): void
  resetHealthAndLimbs(): void
  restoreActorValue(asValueName: string, afAmount: number): void
  resurrect(): Promise<void>
  sendAssaultAlarm(): void
  sendLycanthropyStateChanged(abIsWerewolf: boolean): void
  sendTrespassAlarm(akCriminal: Actor | null): void
  sendVampirismStateChanged(abIsVampire: boolean): void
  setActorValue(asValueName: string, afValue: number): void
  setAlert(abAlerted: boolean): void
  setAllowFlying(abAllowed: boolean): void
  setAllowFlyingEx(abAllowed: boolean, abAllowCrash: boolean, abAllowSearch: boolean): void
  setAlpha(afTargetAlpha: number, abFade: boolean): void
  setAttackActorOnSight(abAttackOnSight: boolean): void
  setBribed(abBribe: boolean): void
  setCrimeFaction(akFaction: Faction | null): void
  setCriticalStage(aiStage: number): void
  setDoingFavor(abDoingFavor: boolean): void
  setDontMove(abDontMove: boolean): void
  setExpressionModifier(index: number, value: number): void
  setExpressionOverride(aiMood: number, aiStrength: number): void
  setExpressionPhoneme(index: number, value: number): void
  setEyeTexture(akNewTexture: TextureSet | null): void
  setFactionRank(akFaction: Faction | null, aiRank: number): void
  setForcedLandingMarker(aMarker: ObjectReference | null): void
  setGhost(abIsGhost: boolean): void
  setHeadTracking(abEnable: boolean): void
  setIntimidated(abIntimidate: boolean): void
  setLookAt(akTarget: ObjectReference | null, abPathingLookAt: boolean): void
  setNoBleedoutRecovery(abAllowed: boolean): void
  setNotShowOnStealthMeter(abNotShow: boolean): void
  setOutfit(akOutfit: Outfit | null, abSleepOutfit: boolean): void
  setPlayerControls(abControls: boolean): void
  setPlayerResistingArrest(): void
  setPlayerTeammate(abTeammate: boolean, abCanDoFavor: boolean): void
  setRace(akRace: Race | null): void
  setRelationshipRank(akOther: Actor | null, aiRank: number): void
  setRestrained(abRestrained: boolean): void
  setSubGraphFloatVariable(asVariableName: string, afValue: number): void
  setUnconscious(abUnconscious: boolean): void
  setVehicle(akVehicle: ObjectReference | null): void
  setVoiceRecoveryTime(afTime: number): void
  sheatheWeapon(): void
  showBarterMenu(): void
  showGiftMenu(abGivingGift: boolean, apFilterList: FormList | null, abShowStolenItems: boolean, abUseFavorPoints: boolean): Promise<number>
  startCannibal(akTarget: Actor | null): void
  startCombat(akTarget: Actor | null): void
  startDeferredKill(): void
  startSneaking(): void
  startVampireFeed(akTarget: Actor | null): void
  stopCombat(): void
  stopCombatAlarm(): void
  trapSoul(akTarget: Actor | null): boolean
  unLockOwnedDoorsInCell(): void
  unequipAll(): void
  unequipItem(akItem: Form | null, abPreventEquip: boolean, abSilent: boolean): void
  unequipItemEx(item: Form | null, equipSlot: number, preventEquip: boolean): void
  unequipItemSlot(aiSlot: number): void
  unequipShout(akShout: Shout | null): void
  unequipSpell(akSpell: Spell | null, aiSource: number): void
  updateWeight(neckDelta: number): void
  willIntimidateSucceed(): boolean
  wornHasKeyword(akKeyword: Keyword | null): boolean
}

// Based on ActorBase.pex
export declare class ActorBase extends Form {
  static from(papyrusObject: PapyrusObject | null): ActorBase | null
  getClass(): Class | null
  getCombatStyle(): CombatStyle | null
  getDeadCount(): number
  getFaceMorph(index: number): number
  getFacePreset(index: number): number
  getFaceTextureSet(): TextureSet | null
  getGiftFilter(): FormList | null
  getHairColor(): ColorForm | null
  getHeight(): number
  getIndexOfHeadPartByType(type: number): number
  getIndexOfOverlayHeadPartByType(type: number): number
  getNthHeadPart(slotPart: number): HeadPart | null
  getNthOverlayHeadPart(slotPart: number): HeadPart | null
  getNthSpell(n: number): Spell | null
  getNumHeadParts(): number
  getNumOverlayHeadParts(): number
  getOutfit(bSleepOutfit: boolean): Outfit | null
  getRace(): Race | null
  getSex(): number
  getSkin(): Armor | null
  getSkinFar(): Armor | null
  getSpellCount(): number
  getTemplate(): ActorBase | null
  getVoiceType(): VoiceType | null
  getWeight(): number
  isEssential(): boolean
  isInvulnerable(): boolean
  isProtected(): boolean
  isUnique(): boolean
  setClass(c: Class | null): void
  setCombatStyle(cs: CombatStyle | null): void
  setEssential(abEssential: boolean): void
  setFaceMorph(value: number, index: number): void
  setFacePreset(value: number, index: number): void
  setFaceTextureSet(textures: TextureSet | null): void
  setHairColor(color: ColorForm | null): void
  setHeight(height: number): void
  setInvulnerable(abInvulnerable: boolean): void
  setNthHeadPart(HeadPart: HeadPart | null, slotPart: number): void
  setOutfit(akOutfit: Outfit | null, abSleepOutfit: boolean): void
  setProtected(abProtected: boolean): void
  setSkin(skin: Armor | null): void
  setSkinFar(skin: Armor | null): void
  setVoiceType(nVoice: VoiceType | null): void
  setWeight(weight: number): void
}

// Based on ActorValueInfo.pex
export declare class ActorValueInfo extends Form {
  static from(papyrusObject: PapyrusObject | null): ActorValueInfo | null
  addSkillExperience(exp: number): void
  getBaseValue(akActor: Actor | null): number
  getCurrentValue(akActor: Actor | null): number
  getExperienceForLevel(currentLevel: number): number
  getMaximumValue(akActor: Actor | null): number
  getPerkTree(list: FormList | null, akActor: Actor | null, unowned: boolean, allRanks: boolean): void
  getPerks(akActor: Actor | null, unowned: boolean, allRanks: boolean): PapyrusObject[] | null
  getSkillExperience(): number
  getSkillImproveMult(): number
  getSkillImproveOffset(): number
  getSkillLegendaryLevel(): number
  getSkillOffsetMult(): number
  getSkillUseMult(): number
  isSkill(): boolean
  setSkillExperience(exp: number): void
  setSkillImproveMult(value: number): void
  setSkillImproveOffset(value: number): void
  setSkillLegendaryLevel(level: number): void
  setSkillOffsetMult(value: number): void
  setSkillUseMult(value: number): void
  static getActorValueInfoByID(id: number): ActorValueInfo | null
  static getActorValueInfoByName(avName: string): ActorValueInfo | null
}

// Based on Alias.pex
export declare class Alias extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Alias | null
  getID(): number
  getName(): string
  getOwningQuest(): Quest | null
  registerForActorAction(actionType: number): void
  registerForAnimationEvent(akSender: ObjectReference | null, asEventName: string): boolean
  registerForCameraState(): void
  registerForControl(control: string): void
  registerForCrosshairRef(): void
  registerForKey(keyCode: number): void
  registerForLOS(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForMenu(menuName: string): void
  registerForModEvent(eventName: string, callbackName: string): void
  registerForNiNodeUpdate(): void
  registerForSingleLOSGain(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForSingleLOSLost(akViewer: Actor | null, akTarget: ObjectReference | null): void
  registerForSingleUpdate(afInterval: number): void
  registerForSingleUpdateGameTime(afInterval: number): void
  registerForSleep(): void
  registerForTrackedStatsEvent(): void
  registerForUpdate(afInterval: number): void
  registerForUpdateGameTime(afInterval: number): void
  sendModEvent(eventName: string, strArg: string, numArg: number): void
  startObjectProfiling(): void
  stopObjectProfiling(): void
  unregisterForActorAction(actionType: number): void
  unregisterForAllControls(): void
  unregisterForAllKeys(): void
  unregisterForAllMenus(): void
  unregisterForAllModEvents(): void
  unregisterForAnimationEvent(akSender: ObjectReference | null, asEventName: string): void
  unregisterForCameraState(): void
  unregisterForControl(control: string): void
  unregisterForCrosshairRef(): void
  unregisterForKey(keyCode: number): void
  unregisterForLOS(akViewer: Actor | null, akTarget: ObjectReference | null): void
  unregisterForMenu(menuName: string): void
  unregisterForModEvent(eventName: string): void
  unregisterForNiNodeUpdate(): void
  unregisterForSleep(): void
  unregisterForTrackedStatsEvent(): void
  unregisterForUpdate(): void
  unregisterForUpdateGameTime(): void
}

// Based on Ammo.pex
export declare class Ammo extends Form {
  static from(papyrusObject: PapyrusObject | null): Ammo | null
  getDamage(): number
  getProjectile(): Projectile | null
  isBolt(): boolean
}

// Based on MiscObject.pex
export declare class MiscObject extends Form {
  static from(papyrusObject: PapyrusObject | null): MiscObject | null
}

// Based on Apparatus.pex
export declare class Apparatus extends MiscObject {
  static from(papyrusObject: PapyrusObject | null): Apparatus | null
  getQuality(): number
  setQuality(quality: number): void
}

// Based on Armor.pex
export declare class Armor extends Form {
  static from(papyrusObject: PapyrusObject | null): Armor | null
  addSlotToMask(slotMask: number): number
  getArmorRating(): number
  getEnchantment(): Enchantment | null
  getIconPath(bFemalePath: boolean): string
  getMessageIconPath(bFemalePath: boolean): string
  getModelPath(bFemalePath: boolean): string
  getNthArmorAddon(n: number): ArmorAddon | null
  getNumArmorAddons(): number
  getSlotMask(): number
  getWarmthRating(): number
  getWeightClass(): number
  modArmorRating(modBy: number): void
  removeSlotFromMask(slotMask: number): number
  setArmorRating(armorRating: number): void
  setEnchantment(e: Enchantment | null): void
  setIconPath(path: string, bFemalePath: boolean): void
  setMessageIconPath(path: string, bFemalePath: boolean): void
  setModelPath(path: string, bFemalePath: boolean): void
  setSlotMask(slotMask: number): void
  setWeightClass(weightClass: number): void
  static getMaskForSlot(slot: number): number
}

// Based on ArmorAddon.pex
export declare class ArmorAddon extends Form {
  static from(papyrusObject: PapyrusObject | null): ArmorAddon | null
  addSlotToMask(slotMask: number): number
  getModelNthTextureSet(n: number, first: boolean, female: boolean): TextureSet | null
  getModelNumTextureSets(first: boolean, female: boolean): number
  getModelPath(firstPerson: boolean, female: boolean): string
  getNthAdditionalRace(n: number): Race | null
  getNumAdditionalRaces(): number
  getSlotMask(): number
  removeSlotFromMask(slotMask: number): number
  setModelNthTextureSet(texture: TextureSet | null, n: number, first: boolean, female: boolean): void
  setModelPath(path: string, firstPerson: boolean, female: boolean): void
  setSlotMask(slotMask: number): void
}

// Based on Art.pex
export declare class Art extends Form {
  static from(papyrusObject: PapyrusObject | null): Art | null
  getModelPath(): string
  setModelPath(path: string): void
}

// Based on AssociationType.pex
export declare class AssociationType extends Form {
  static from(papyrusObject: PapyrusObject | null): AssociationType | null
}

// Based on Book.pex
export declare class Book extends Form {
  static from(papyrusObject: PapyrusObject | null): Book | null
  getSkill(): number
  getSpell(): Spell | null
  isRead(): boolean
  isTakeable(): boolean
}

// Based on Cell.pex
export declare class Cell extends Form {
  static from(papyrusObject: PapyrusObject | null): Cell | null
  getActorOwner(): ActorBase | null
  getFactionOwner(): Faction | null
  getNthRef(n: number, formTypeFilter: number): ObjectReference | null
  getNumRefs(formTypeFilter: number): number
  getWaterLevel(): number
  isAttached(): boolean
  isInterior(): boolean
  reset(): void
  setActorOwner(akActor: ActorBase | null): void
  setFactionOwner(akFaction: Faction | null): void
  setFogColor(aiNearRed: number, aiNearGreen: number, aiNearBlue: number, aiFarRed: number, aiFarGreen: number, aiFarBlue: number): void
  setFogPlanes(afNear: number, afFar: number): void
  setFogPower(afPower: number): void
  setPublic(abPublic: boolean): void
}

// Based on Class.pex
export declare class Class extends Form {
  static from(papyrusObject: PapyrusObject | null): Class | null
}

// Based on ColorForm.pex
export declare class ColorForm extends Form {
  static from(papyrusObject: PapyrusObject | null): ColorForm | null
  getColor(): number
  setColor(color: number): void
}

// Based on CombatStyle.pex
export declare class CombatStyle extends Form {
  static from(papyrusObject: PapyrusObject | null): CombatStyle | null
  getAllowDualWielding(): boolean
  getAvoidThreatChance(): number
  getCloseRangeDuelingCircleMult(): number
  getCloseRangeDuelingFallbackMult(): number
  getCloseRangeFlankingFlankDistance(): number
  getCloseRangeFlankingStalkTime(): number
  getDefensiveMult(): number
  getFlightDiveBombChance(): number
  getFlightFlyingAttackChance(): number
  getFlightHoverChance(): number
  getGroupOffensiveMult(): number
  getLongRangeStrafeMult(): number
  getMagicMult(): number
  getMeleeAttackStaggeredMult(): number
  getMeleeBashAttackMult(): number
  getMeleeBashMult(): number
  getMeleeBashPowerAttackMult(): number
  getMeleeBashRecoiledMult(): number
  getMeleeMult(): number
  getMeleePowerAttackBlockingMult(): number
  getMeleePowerAttackStaggeredMult(): number
  getMeleeSpecialAttackMult(): number
  getOffensiveMult(): number
  getRangedMult(): number
  getShoutMult(): number
  getStaffMult(): number
  getUnarmedMult(): number
  setAllowDualWielding(allow: boolean): void
  setAvoidThreatChance(chance: number): void
  setCloseRangeDuelingCircleMult(mult: number): void
  setCloseRangeDuelingFallbackMult(mult: number): void
  setCloseRangeFlankingFlankDistance(mult: number): void
  setCloseRangeFlankingStalkTime(mult: number): void
  setDefensiveMult(mult: number): void
  setFlightDiveBombChance(chance: number): void
  setFlightFlyingAttackChance(mult: number): void
  setFlightHoverChance(chance: number): void
  setGroupOffensiveMult(mult: number): void
  setLongRangeStrafeMult(mult: number): void
  setMagicMult(mult: number): void
  setMeleeAttackStaggeredMult(mult: number): void
  setMeleeBashAttackMult(mult: number): void
  setMeleeBashMult(mult: number): void
  setMeleeBashPowerAttackMult(mult: number): void
  setMeleeBashRecoiledMult(mult: number): void
  setMeleeMult(mult: number): void
  setMeleePowerAttackBlockingMult(mult: number): void
  setMeleePowerAttackStaggeredMult(mult: number): void
  setMeleeSpecialAttackMult(mult: number): void
  setOffensiveMult(mult: number): void
  setRangedMult(mult: number): void
  setShoutMult(mult: number): void
  setStaffMult(mult: number): void
  setUnarmedMult(mult: number): void
}

// Based on ConstructibleObject.pex
export declare class ConstructibleObject extends MiscObject {
  static from(papyrusObject: PapyrusObject | null): ConstructibleObject | null
  getNthIngredient(n: number): Form | null
  getNthIngredientQuantity(n: number): number
  getNumIngredients(): number
  getResult(): Form | null
  getResultQuantity(): number
  getWorkbenchKeyword(): Keyword | null
  setNthIngredient(required: Form | null, n: number): void
  setNthIngredientQuantity(value: number, n: number): void
  setResult(result: Form | null): void
  setResultQuantity(quantity: number): void
  setWorkbenchKeyword(aKeyword: Keyword | null): void
}

// Based on Container.pex
export declare class Container extends Form {
  static from(papyrusObject: PapyrusObject | null): Container | null
}

// Based on Debug.pex
export declare class Debug extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Debug | null
  static centerOnCell(param1: string): void
  static centerOnCellAndWait(param1: string): Promise<number>
  static closeUserLog(param1: string): void
  static dBSendPlayerPosition(): void
  static debugChannelNotify(param1: string, param2: string): void
  static dumpAliasData(param1: Quest | null): void
  static getConfigName(): Promise<string>
  static getPlatformName(): Promise<string>
  static getVersionNumber(): Promise<string>
  static messageBox(param1: string): void
  static notification(param1: string): void
  static openUserLog(param1: string): boolean
  static playerMoveToAndWait(param1: string): Promise<number>
  static quitGame(): void
  static sendAnimationEvent(param1: ObjectReference | null, param2: string): void
  static setFootIK(param1: boolean): void
  static setGodMode(param1: boolean): void
  static showRefPosition(arRef: ObjectReference | null): void
  static startScriptProfiling(param1: string): void
  static startStackProfiling(): void
  static stopScriptProfiling(param1: string): void
  static stopStackProfiling(): void
  static takeScreenshot(param1: string): void
  static toggleAI(): void
  static toggleCollisions(): void
  static toggleMenus(): void
  static trace(param1: string, param2: number): void
  static traceStack(param1: string, param2: number): void
  static traceUser(param1: string, param2: string, param3: number): boolean
}

// Based on DefaultObjectManager.pex
export declare class DefaultObjectManager extends Form {
  static from(papyrusObject: PapyrusObject | null): DefaultObjectManager | null
  getForm(key: string): Form | null
  setForm(key: string, newForm: Form | null): void
}

// Based on Door.pex
export declare class Door extends Form {
  static from(papyrusObject: PapyrusObject | null): Door | null
}

// Based on EffectShader.pex
export declare class EffectShader extends Form {
  static from(papyrusObject: PapyrusObject | null): EffectShader | null
  play(param1: ObjectReference | null, param2: number): void
  stop(param1: ObjectReference | null): void
}

// Based on Enchantment.pex
export declare class Enchantment extends Form {
  static from(papyrusObject: PapyrusObject | null): Enchantment | null
  getBaseEnchantment(): Enchantment | null
  getCostliestEffectIndex(): number
  getKeywordRestrictions(): FormList | null
  getNthEffectArea(index: number): number
  getNthEffectDuration(index: number): number
  getNthEffectMagicEffect(index: number): MagicEffect | null
  getNthEffectMagnitude(index: number): number
  getNumEffects(): number
  isHostile(): boolean
  setKeywordRestrictions(newKeywordList: FormList | null): void
  setNthEffectArea(index: number, value: number): void
  setNthEffectDuration(index: number, value: number): void
  setNthEffectMagnitude(index: number, value: number): void
}

// Based on EncounterZone.pex
export declare class EncounterZone extends Form {
  static from(papyrusObject: PapyrusObject | null): EncounterZone | null
}

// Based on EquipSlot.pex
export declare class EquipSlot extends Form {
  static from(papyrusObject: PapyrusObject | null): EquipSlot | null
  getNthParent(n: number): EquipSlot | null
  getNumParents(): number
}

// Based on Explosion.pex
export declare class Explosion extends Form {
  static from(papyrusObject: PapyrusObject | null): Explosion | null
}

// Based on Faction.pex
export declare class Faction extends Form {
  static from(papyrusObject: PapyrusObject | null): Faction | null
  canPayCrimeGold(): boolean
  clearFactionFlag(flag: number): void
  getBuySellList(): FormList | null
  getCrimeGold(): number
  getCrimeGoldNonViolent(): number
  getCrimeGoldViolent(): number
  getInfamy(): number
  getInfamyNonViolent(): number
  getInfamyViolent(): number
  getMerchantContainer(): ObjectReference | null
  getReaction(akOther: Faction | null): number
  getStolenItemValueCrime(): number
  getStolenItemValueNoCrime(): number
  getVendorEndHour(): number
  getVendorRadius(): number
  getVendorStartHour(): number
  isFactionFlagSet(flag: number): boolean
  isFactionInCrimeGroup(akOther: Faction | null): boolean
  isNotSellBuy(): boolean
  isPlayerExpelled(): boolean
  modCrimeGold(aiAmount: number, abViolent: boolean): void
  modReaction(akOther: Faction | null, aiAmount: number): void
  onlyBuysStolenItems(): boolean
  playerPayCrimeGold(abRemoveStolenItems: boolean, abGoToJail: boolean): void
  sendAssaultAlarm(): void
  sendPlayerToJail(abRemoveInventory: boolean, abRealJail: boolean): Promise<void>
  setAlly(akOther: Faction | null, abSelfIsFriendToOther: boolean, abOtherIsFriendToSelf: boolean): void
  setBuySellList(akList: FormList | null): void
  setCrimeGold(aiGold: number): void
  setCrimeGoldViolent(aiGold: number): void
  setEnemy(akOther: Faction | null, abSelfIsNeutralToOther: boolean, abOtherIsNeutralToSelf: boolean): void
  setFactionFlag(flag: number): void
  setMerchantContainer(akContainer: ObjectReference | null): void
  setNotSellBuy(notSellBuy: boolean): void
  setOnlyBuysStolenItems(onlyStolen: boolean): void
  setPlayerEnemy(abIsEnemy: boolean): void
  setPlayerExpelled(abIsExpelled: boolean): void
  setReaction(akOther: Faction | null, aiNewValue: number): void
  setVendorEndHour(hour: number): void
  setVendorRadius(radius: number): void
  setVendorStartHour(hour: number): void
}

// Based on Flora.pex
export declare class Flora extends Activator {
  static from(papyrusObject: PapyrusObject | null): Flora | null
  getHarvestSound(): SoundDescriptor | null
  getIngredient(): Form | null
  setHarvestSound(akSoundDescriptor: SoundDescriptor | null): void
  setIngredient(akIngredient: Form | null): void
}

// Based on FormList.pex
export declare class FormList extends Form {
  static from(papyrusObject: PapyrusObject | null): FormList | null
  addForm(apForm: Form | null): void
  addForms(forms: PapyrusObject[] | null): void
  find(apForm: Form | null): number
  getAt(aiIndex: number): Form | null
  getSize(): number
  hasForm(akForm: Form | null): boolean
  removeAddedForm(apForm: Form | null): void
  revert(): void
  toArray(): PapyrusObject[] | null
}

// Based on Furniture.pex
export declare class Furniture extends Activator {
  static from(papyrusObject: PapyrusObject | null): Furniture | null
}

// Based on Game.pex
export declare class Game extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Game | null
  static addAchievement(aiAchievementID: number): void
  static addHavokBallAndSocketConstraint(arRefA: ObjectReference | null, arRefANode: string, arRefB: ObjectReference | null, arRefBNode: string, afRefALocalOffsetX: number, afRefALocalOffsetY: number, afRefALocalOffsetZ: number, afRefBLocalOffsetX: number, afRefBLocalOffsetY: number, afRefBLocalOffsetZ: number): Promise<boolean>
  static addPerkPoints(aiPerkPoints: number): void
  static advanceSkill(asSkillName: string, afMagnitude: number): void
  static calculateFavorCost(aiFavorPrice: number): number
  static clearPrison(): void
  static clearTempEffects(): void
  static disablePlayerControls(abMovement: boolean, abFighting: boolean, abCamSwitch: boolean, abLooking: boolean, abSneaking: boolean, abMenu: boolean, abActivate: boolean, abJournalTabs: boolean, aiDisablePOVType: number): void
  static enableFastTravel(abEnable: boolean): void
  static enablePlayerControls(abMovement: boolean, abFighting: boolean, abCamSwitch: boolean, abLooking: boolean, abSneaking: boolean, abMenu: boolean, abActivate: boolean, abJournalTabs: boolean, aiDisablePOVType: number): void
  static fadeOutGame(abFadingOut: boolean, abBlackFade: boolean, afSecsBeforeFade: number, afFadeDuration: number): void
  static fastTravel(akDestination: ObjectReference | null): void
  static findClosestActor(afX: number, afY: number, afZ: number, afRadius: number): Actor | null
  static findClosestReferenceOfAnyTypeInList(arBaseObjects: FormList | null, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference | null
  static findClosestReferenceOfType(arBaseObject: Form | null, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference | null
  static findRandomActor(afX: number, afY: number, afZ: number, afRadius: number): Actor | null
  static findRandomReferenceOfAnyTypeInList(arBaseObjects: FormList | null, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference | null
  static findRandomReferenceOfType(arBaseObject: Form | null, afX: number, afY: number, afZ: number, afRadius: number): ObjectReference | null
  static forceFirstPerson(): void
  static forceThirdPerson(): void
  static getCameraState(): number
  static getCurrentConsoleRef(): ObjectReference | null
  static getCurrentCrosshairRef(): ObjectReference | null
  static getDialogueTarget(): ObjectReference | null
  static getExperienceForLevel(currentLevel: number): number
  static getForm(aiFormID: number): Form | null
  static getFormEx(formId: number): Form | null
  static getFormFromFile(aiFormID: number, asFilename: string): Form | null
  static getGameSettingFloat(asGameSetting: string): number
  static getGameSettingInt(asGameSetting: string): number
  static getGameSettingString(asGameSetting: string): Promise<string>
  static getHotkeyBoundObject(hotkey: number): Form | null
  static getLightModAuthor(idx: number): string
  static getLightModByName(name: string): number
  static getLightModCount(): number
  static getLightModDependencyCount(idx: number): number
  static getLightModDescription(idx: number): string
  static getLightModName(idx: number): string
  static getModAuthor(modIndex: number): string
  static getModByName(name: string): number
  static getModCount(): number
  static getModDependencyCount(modIndex: number): number
  static getModDescription(modIndex: number): string
  static getModName(modIndex: number): string
  static getNthLightModDependency(modIdx: number, idx: number): number
  static getNthTintMaskColor(n: number): number
  static getNthTintMaskTexturePath(n: number): string
  static getNthTintMaskType(n: number): number
  static getNumTintMasks(): number
  static getNumTintsByType(type: number): number
  static getPerkPoints(): number
  static getPlayerExperience(): number
  static getPlayerGrabbedRef(): ObjectReference | null
  static getPlayerMovementMode(): boolean
  static getPlayersLastRiddenHorse(): Actor | null
  static getRealHoursPassed(): number
  static getSunPositionX(): number
  static getSunPositionY(): number
  static getSunPositionZ(): number
  static getTintMaskColor(type: number, index: number): number
  static getTintMaskTexturePath(type: number, index: number): string
  static hideTitleSequenceMenu(): void
  static incrementSkill(asSkillName: string): void
  static incrementSkillBy(asSkillName: string, aiCount: number): void
  static incrementStat(asStatName: string, aiModAmount: number): void
  static isActivateControlsEnabled(): boolean
  static isCamSwitchControlsEnabled(): boolean
  static isFastTravelControlsEnabled(): boolean
  static isFastTravelEnabled(): boolean
  static isFightingControlsEnabled(): boolean
  static isJournalControlsEnabled(): boolean
  static isLookingControlsEnabled(): boolean
  static isMenuControlsEnabled(): boolean
  static isMovementControlsEnabled(): boolean
  static isObjectFavorited(Form: Form | null): boolean
  static isPlayerSungazing(): boolean
  static isPluginInstalled(name: string): boolean
  static isSneakingControlsEnabled(): boolean
  static isWordUnlocked(akWord: WordOfPower | null): boolean
  static loadGame(name: string): void
  static modPerkPoints(perkPoints: number): void
  static playBink(asFilename: string, abInterruptible: boolean, abMuteAudio: boolean, abMuteMusic: boolean, abLetterbox: boolean): void
  static precacheCharGen(): void
  static precacheCharGenClear(): void
  static queryStat(asStat: string): number
  static quitToMainMenu(): void
  static removeHavokConstraints(arFirstRef: ObjectReference | null, arFirstRefNodeName: string, arSecondRef: ObjectReference | null, arSecondRefNodeName: string): Promise<boolean>
  static requestAutosave(): void
  static requestModel(asModelName: string): void
  static requestSave(): void
  static saveGame(name: string): void
  static sendWereWolfTransformation(): void
  static serveTime(): void
  static setAllowFlyingMountLandingRequests(abAllow: boolean): void
  static setBeastForm(abEntering: boolean): void
  static setCameraTarget(arTarget: Actor | null): void
  static setGameSettingBool(setting: string, value: boolean): void
  static setGameSettingFloat(setting: string, value: number): void
  static setGameSettingInt(setting: string, value: number): void
  static setGameSettingString(setting: string, value: string): void
  static setHudCartMode(abSetCartMode: boolean): void
  static setInChargen(abDisableSaving: boolean, abDisableWaiting: boolean, abShowControlsDisabledMessage: boolean): void
  static setMiscStat(name: string, value: number): void
  static setNthTintMaskColor(n: number, color: number): void
  static setNthTintMaskTexturePath(path: string, n: number): void
  static setPerkPoints(perkPoints: number): void
  static setPlayerAIDriven(abAIDriven: boolean): void
  static setPlayerExperience(exp: number): void
  static setPlayerLevel(level: number): void
  static setPlayerReportCrime(abReportCrime: boolean): void
  static setPlayersLastRiddenHorse(horse: Actor | null): void
  static setSittingRotation(afValue: number): void
  static setSunGazeImageSpaceModifier(apImod: ImageSpaceModifier | null): void
  static setTintMaskColor(color: number, type: number, index: number): void
  static setTintMaskTexturePath(path: string, type: number, index: number): void
  static showFirstPersonGeometry(abShow: boolean): void
  static showLimitedRaceMenu(): void
  static showRaceMenu(): void
  static showTitleSequenceMenu(): void
  static showTrainingMenu(aTrainer: Actor | null): void
  static startTitleSequence(asSequenceName: string): void
  static teachWord(akWord: WordOfPower | null): void
  static triggerScreenBlood(aiValue: number): void
  static unbindObjectHotkey(hotkey: number): void
  static unlockWord(akWord: WordOfPower | null): void
  static updateHairColor(): void
  static updateThirdPerson(): void
  static updateTintMaskColors(): void
  static usingGamepad(): boolean
  static getPlayer(): Actor | null
  static shakeCamera(akSource: ObjectReference | null, afStrength: number, afDuration: number): void
  static shakeController(afSmallMotorStrength: number, afBigMotorStreangth: number, afDuration: number): void
}

// Based on GlobalVariable.pex
export declare class GlobalVariable extends Form {
  static from(papyrusObject: PapyrusObject | null): GlobalVariable | null
  getValue(): number
  setValue(param1: number): void
}

// Based on Hazard.pex
export declare class Hazard extends Form {
  static from(papyrusObject: PapyrusObject | null): Hazard | null
}

// Based on HeadPart.pex
export declare class HeadPart extends Form {
  static from(papyrusObject: PapyrusObject | null): HeadPart | null
  getIndexOfExtraPart(p: HeadPart | null): number
  getNthExtraPart(n: number): HeadPart | null
  getNumExtraParts(): number
  getPartName(): string
  getType(): number
  getValidRaces(): FormList | null
  hasExtraPart(p: HeadPart | null): boolean
  isExtraPart(): boolean
  setValidRaces(vRaces: FormList | null): void
  static getHeadPart(name: string): HeadPart | null
}

// Based on Idle.pex
export declare class Idle extends Form {
  static from(papyrusObject: PapyrusObject | null): Idle | null
}

// Based on ImageSpaceModifier.pex
export declare class ImageSpaceModifier extends Form {
  static from(papyrusObject: PapyrusObject | null): ImageSpaceModifier | null
  apply(param1: number): void
  applyCrossFade(param1: number): void
  popTo(param1: ImageSpaceModifier | null, param2: number): void
  remove(): void
  static removeCrossFade(param1: number): void
}

// Based on ImpactDataSet.pex
export declare class ImpactDataSet extends Form {
  static from(papyrusObject: PapyrusObject | null): ImpactDataSet | null
}

// Based on Ingredient.pex
export declare class Ingredient extends Form {
  static from(papyrusObject: PapyrusObject | null): Ingredient | null
  getCostliestEffectIndex(): number
  getEffectAreas(): number[] | null
  getEffectDurations(): number[] | null
  getEffectMagnitudes(): number[] | null
  getIsNthEffectKnown(index: number): boolean
  getMagicEffects(): PapyrusObject[] | null
  getNthEffectArea(index: number): number
  getNthEffectDuration(index: number): number
  getNthEffectMagicEffect(index: number): MagicEffect | null
  getNthEffectMagnitude(index: number): number
  getNumEffects(): number
  isHostile(): boolean
  learnAllEffects(): void
  learnEffect(aiIndex: number): void
  learnNextEffect(): number
  setNthEffectArea(index: number, value: number): void
  setNthEffectDuration(index: number, value: number): void
  setNthEffectMagnitude(index: number, value: number): void
}

// Based on Input.pex
export declare class Input extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Input | null
  static getMappedControl(keycode: number): string
  static getMappedKey(control: string, deviceType: number): number
  static getNthKeyPressed(n: number): number
  static getNumKeysPressed(): number
  static holdKey(dxKeycode: number): void
  static isKeyPressed(dxKeycode: number): boolean
  static releaseKey(dxKeycode: number): void
  static tapKey(dxKeycode: number): void
}

// Based on Key.pex
export declare class Key extends MiscObject {
  static from(papyrusObject: PapyrusObject | null): Key | null
}

// Based on Keyword.pex
export declare class Keyword extends Form {
  static from(papyrusObject: PapyrusObject | null): Keyword | null
  getString(): string
  sendStoryEvent(akLoc: Location | null, akRef1: ObjectReference | null, akRef2: ObjectReference | null, aiValue1: number, aiValue2: number): void
  sendStoryEventAndWait(akLoc: Location | null, akRef1: ObjectReference | null, akRef2: ObjectReference | null, aiValue1: number, aiValue2: number): Promise<boolean>
  static getKeyword(key: string): Keyword | null
}

// Based on LeveledActor.pex
export declare class LeveledActor extends Form {
  static from(papyrusObject: PapyrusObject | null): LeveledActor | null
  addForm(apForm: Form | null, aiLevel: number): void
  getNthCount(n: number): number
  getNthForm(n: number): Form | null
  getNthLevel(n: number): number
  getNumForms(): number
  revert(): void
  setNthCount(n: number, count: number): void
  setNthLevel(n: number, level: number): void
}

// Based on LeveledItem.pex
export declare class LeveledItem extends Form {
  static from(papyrusObject: PapyrusObject | null): LeveledItem | null
  addForm(apForm: Form | null, aiLevel: number, aiCount: number): void
  getChanceGlobal(): GlobalVariable | null
  getChanceNone(): number
  getNthCount(n: number): number
  getNthForm(n: number): Form | null
  getNthLevel(n: number): number
  getNumForms(): number
  revert(): void
  setChanceGlobal(glob: GlobalVariable | null): void
  setChanceNone(chance: number): void
  setNthCount(n: number, count: number): void
  setNthLevel(n: number, level: number): void
}

// Based on LeveledSpell.pex
export declare class LeveledSpell extends Form {
  static from(papyrusObject: PapyrusObject | null): LeveledSpell | null
  addForm(apForm: Form | null, aiLevel: number): void
  getChanceNone(): number
  getNthForm(n: number): Form | null
  getNthLevel(n: number): number
  getNumForms(): number
  revert(): void
  setChanceNone(chance: number): void
  setNthLevel(n: number, level: number): void
}

// Based on Light.pex
export declare class Light extends Form {
  static from(papyrusObject: PapyrusObject | null): Light | null
  getWarmthRating(): number
}

// Based on Location.pex
export declare class Location extends Form {
  static from(papyrusObject: PapyrusObject | null): Location | null
  getKeywordData(param1: Keyword | null): number
  getRefTypeAliveCount(param1: LocationRefType | null): number
  getRefTypeDeadCount(param1: LocationRefType | null): number
  hasCommonParent(param1: Location | null, param2: Keyword | null): boolean
  hasRefType(param1: LocationRefType | null): boolean
  isChild(param1: Location | null): boolean
  isCleared(): boolean
  isLoaded(): boolean
  setCleared(param1: boolean): void
  setKeywordData(param1: Keyword | null, param2: number): void
}

// Based on LocationAlias.pex
export declare class LocationAlias extends Alias {
  static from(papyrusObject: PapyrusObject | null): LocationAlias | null
  clear(): void
  forceLocationTo(param1: Location | null): void
  getLocation(): Location | null
}

// Based on LocationRefType.pex
export declare class LocationRefType extends Keyword {
  static from(papyrusObject: PapyrusObject | null): LocationRefType | null
}

// Based on MagicEffect.pex
export declare class MagicEffect extends Form {
  static from(papyrusObject: PapyrusObject | null): MagicEffect | null
  clearEffectFlag(flag: number): void
  getArea(): number
  getAssociatedSkill(): Promise<string>
  getBaseCost(): number
  getCastTime(): number
  getCastingArt(): Art | null
  getCastingType(): number
  getDeliveryType(): number
  getEnchantArt(): Art | null
  getEnchantShader(): EffectShader | null
  getEquipAbility(): Spell | null
  getExplosion(): Explosion | null
  getHitEffectArt(): Art | null
  getHitShader(): EffectShader | null
  getImageSpaceMod(): ImageSpaceModifier | null
  getImpactDataSet(): ImpactDataSet | null
  getLight(): Light | null
  getPerk(): Perk | null
  getProjectile(): Projectile | null
  getResistance(): string
  getSkillLevel(): number
  getSkillUsageMult(): number
  getSounds(): PapyrusObject[] | null
  isEffectFlagSet(flag: number): boolean
  setArea(area: number): void
  setAssociatedSkill(skill: string): void
  setBaseCost(cost: number): void
  setCastTime(castTime: number): void
  setCastingArt(obj: Art | null): void
  setEffectFlag(flag: number): void
  setEnchantArt(obj: Art | null): void
  setEnchantShader(obj: EffectShader | null): void
  setEquipAbility(obj: Spell | null): void
  setExplosion(obj: Explosion | null): void
  setHitEffectArt(obj: Art | null): void
  setHitShader(obj: EffectShader | null): void
  setImageSpaceMod(obj: ImageSpaceModifier | null): void
  setImpactDataSet(obj: ImpactDataSet | null): void
  setLight(obj: Light | null): void
  setPerk(obj: Perk | null): void
  setProjectile(obj: Projectile | null): void
  setResistance(skill: string): void
  setSkillLevel(level: number): void
  setSkillUsageMult(usageMult: number): void
}

// Based on Message.pex
export declare class Message extends Form {
  static from(papyrusObject: PapyrusObject | null): Message | null
  show(param1: number, param2: number, param3: number, param4: number, param5: number, param6: number, param7: number, param8: number, param9: number): Promise<number>
  showAsHelpMessage(param1: string, param2: number, param3: number, param4: number): void
  static resetHelpMessage(param1: string): void
}

// Based on MusicType.pex
export declare class MusicType extends Form {
  static from(papyrusObject: PapyrusObject | null): MusicType | null
  add(): void
  remove(): void
}

// Based on NetImmerse.pex
export declare class NetImmerse extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): NetImmerse | null
  static getNodeLocalPosition(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static getNodeLocalPositionX(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeLocalPositionY(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeLocalPositionZ(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeLocalRotationEuler(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static getNodeLocalRotationMatrix(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static getNodeScale(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeWorldPosition(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static getNodeWorldPositionX(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeWorldPositionY(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeWorldPositionZ(ref: ObjectReference | null, node: string, firstPerson: boolean): number
  static getNodeWorldRotationEuler(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static getNodeWorldRotationMatrix(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static getRelativeNodePosition(ref: ObjectReference | null, nodeA: string, nodeB: string, _in: number[] | null, firstPerson: boolean): boolean
  static getRelativeNodePositionX(ref: ObjectReference | null, nodeA: string, nodeB: string, firstPerson: boolean): number
  static getRelativeNodePositionY(ref: ObjectReference | null, nodeA: string, nodeB: string, firstPerson: boolean): number
  static getRelativeNodePositionZ(ref: ObjectReference | null, nodeA: string, nodeB: string, firstPerson: boolean): number
  static hasNode(ref: ObjectReference | null, node: string, firstPerson: boolean): boolean
  static setNodeLocalPosition(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static setNodeLocalPositionX(ref: ObjectReference | null, node: string, x: number, firstPerson: boolean): void
  static setNodeLocalPositionY(ref: ObjectReference | null, node: string, y: number, firstPerson: boolean): void
  static setNodeLocalPositionZ(ref: ObjectReference | null, node: string, z: number, firstPerson: boolean): void
  static setNodeLocalRotationEuler(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static setNodeLocalRotationMatrix(ref: ObjectReference | null, node: string, _in: number[] | null, firstPerson: boolean): boolean
  static setNodeScale(ref: ObjectReference | null, node: string, scale: number, firstPerson: boolean): void
  static setNodeTextureSet(ref: ObjectReference | null, node: string, tSet: TextureSet | null, firstPerson: boolean): void
}

// Based on Outfit.pex
export declare class Outfit extends Form {
  static from(papyrusObject: PapyrusObject | null): Outfit | null
  getNthPart(n: number): Form | null
  getNumParts(): number
}

// Based on Projectile.pex
export declare class Projectile extends Form {
  static from(papyrusObject: PapyrusObject | null): Projectile | null
}

// Based on Package.pex
export declare class Package extends Form {
  static from(papyrusObject: PapyrusObject | null): Package | null
  getOwningQuest(): Quest | null
  getTemplate(): Package | null
}

// Based on Perk.pex
export declare class Perk extends Form {
  static from(papyrusObject: PapyrusObject | null): Perk | null
  getNextPerk(): Perk | null
  getNthEntryLeveledList(n: number): LeveledItem | null
  getNthEntryPriority(n: number): number
  getNthEntryQuest(n: number): Quest | null
  getNthEntryRank(n: number): number
  getNthEntrySpell(n: number): Spell | null
  getNthEntryStage(n: number): number
  getNthEntryText(n: number): string
  getNthEntryValue(n: number, i: number): number
  getNumEntries(): number
  setNthEntryLeveledList(n: number, lList: LeveledItem | null): boolean
  setNthEntryPriority(n: number, priority: number): boolean
  setNthEntryQuest(n: number, newQuest: Quest | null): boolean
  setNthEntryRank(n: number, rank: number): boolean
  setNthEntrySpell(n: number, newSpell: Spell | null): boolean
  setNthEntryStage(n: number, stage: number): boolean
  setNthEntryText(n: number, newText: string): boolean
  setNthEntryValue(n: number, i: number, value: number): boolean
}

// Based on Potion.pex
export declare class Potion extends Form {
  static from(papyrusObject: PapyrusObject | null): Potion | null
  getCostliestEffectIndex(): number
  getEffectAreas(): number[] | null
  getEffectDurations(): number[] | null
  getEffectMagnitudes(): number[] | null
  getMagicEffects(): PapyrusObject[] | null
  getNthEffectArea(index: number): number
  getNthEffectDuration(index: number): number
  getNthEffectMagicEffect(index: number): MagicEffect | null
  getNthEffectMagnitude(index: number): number
  getNumEffects(): number
  getUseSound(): SoundDescriptor | null
  isFood(): boolean
  isHostile(): boolean
  isPoison(): boolean
  setNthEffectArea(index: number, value: number): void
  setNthEffectDuration(index: number, value: number): void
  setNthEffectMagnitude(index: number, value: number): void
}

// Based on Quest.pex
export declare class Quest extends Form {
  static from(papyrusObject: PapyrusObject | null): Quest | null
  completeAllObjectives(): void
  completeQuest(): void
  failAllObjectives(): void
  getAlias(aiAliasID: number): Alias | null
  getAliasById(aliasId: number): Alias | null
  getAliasByName(name: string): Alias | null
  getAliases(): PapyrusObject[] | null
  getCurrentStageID(): number
  getID(): string
  getNthAlias(index: number): Alias | null
  getNumAliases(): number
  getPriority(): number
  isActive(): boolean
  isCompleted(): boolean
  isObjectiveCompleted(aiObjective: number): boolean
  isObjectiveDisplayed(aiObjective: number): boolean
  isObjectiveFailed(aiObjective: number): boolean
  isRunning(): boolean
  isStageDone(aiStage: number): boolean
  isStarting(): boolean
  isStopped(): boolean
  isStopping(): boolean
  reset(): void
  setActive(abActive: boolean): void
  setCurrentStageID(aiStageID: number): Promise<boolean>
  setObjectiveCompleted(aiObjective: number, abCompleted: boolean): void
  setObjectiveDisplayed(aiObjective: number, abDisplayed: boolean, abForce: boolean): void
  setObjectiveFailed(aiObjective: number, abFailed: boolean): void
  start(): Promise<boolean>
  stop(): void
  updateCurrentInstanceGlobal(aUpdateGlobal: GlobalVariable | null): boolean
  static getQuest(editorId: string): Quest | null
}

// Based on Race.pex
export declare class Race extends Form {
  static from(papyrusObject: PapyrusObject | null): Race | null
  clearRaceFlag(n: number): void
  getDefaultVoiceType(female: boolean): VoiceType | null
  getNthSpell(n: number): Spell | null
  getSkin(): Armor | null
  getSpellCount(): number
  isRaceFlagSet(n: number): boolean
  setDefaultVoiceType(female: boolean, voice: VoiceType | null): void
  setRaceFlag(n: number): void
  setSkin(skin: Armor | null): void
  static getNthPlayableRace(n: number): Race | null
  static getNumPlayableRaces(): number
  static getRace(editorId: string): Race | null
}

// Based on ReferenceAlias.pex
export declare class ReferenceAlias extends Alias {
  static from(papyrusObject: PapyrusObject | null): ReferenceAlias | null
  addInventoryEventFilter(param1: Form | null): void
  clear(): void
  forceRefTo(param1: ObjectReference | null): void
  getReference(): ObjectReference | null
  removeAllInventoryEventFilters(): void
  removeInventoryEventFilter(param1: Form | null): void
}

// Based on Spell.pex
export declare class Spell extends Form {
  static from(papyrusObject: PapyrusObject | null): Spell | null
  cast(akSource: ObjectReference | null, akTarget: ObjectReference | null): Promise<void>
  getCastTime(): number
  getCostliestEffectIndex(): number
  getEffectAreas(): number[] | null
  getEffectDurations(): number[] | null
  getEffectMagnitudes(): number[] | null
  getEffectiveMagickaCost(caster: Actor | null): number
  getEquipType(): EquipSlot | null
  getMagicEffects(): PapyrusObject[] | null
  getMagickaCost(): number
  getNthEffectArea(index: number): number
  getNthEffectDuration(index: number): number
  getNthEffectMagicEffect(index: number): MagicEffect | null
  getNthEffectMagnitude(index: number): number
  getNumEffects(): number
  getPerk(): Perk | null
  isHostile(): boolean
  preload(): void
  remoteCast(akSource: ObjectReference | null, akBlameActor: Actor | null, akTarget: ObjectReference | null): Promise<void>
  setEquipType(type: EquipSlot | null): void
  setNthEffectArea(index: number, value: number): void
  setNthEffectDuration(index: number, value: number): void
  setNthEffectMagnitude(index: number, value: number): void
  unload(): void
}

// Based on Static.pex
export declare class Static extends Form {
  static from(papyrusObject: PapyrusObject | null): Static | null
}

// Based on Scene.pex
export declare class Scene extends Form {
  static from(papyrusObject: PapyrusObject | null): Scene | null
  forceStart(): void
  getOwningQuest(): Quest | null
  isActionComplete(param1: number): boolean
  isPlaying(): boolean
  start(): void
  stop(): void
}

// Based on Scroll.pex
export declare class Scroll extends Form {
  static from(papyrusObject: PapyrusObject | null): Scroll | null
  cast(akSource: ObjectReference | null, akTarget: ObjectReference | null): Promise<void>
  getCastTime(): number
  getCostliestEffectIndex(): number
  getEffectAreas(): number[] | null
  getEffectDurations(): number[] | null
  getEffectMagnitudes(): number[] | null
  getEquipType(): EquipSlot | null
  getMagicEffects(): PapyrusObject[] | null
  getNthEffectArea(index: number): number
  getNthEffectDuration(index: number): number
  getNthEffectMagicEffect(index: number): MagicEffect | null
  getNthEffectMagnitude(index: number): number
  getNumEffects(): number
  getPerk(): Perk | null
  setEquipType(type: EquipSlot | null): void
  setNthEffectArea(index: number, value: number): void
  setNthEffectDuration(index: number, value: number): void
  setNthEffectMagnitude(index: number, value: number): void
}

// Based on ShaderParticleGeometry.pex
export declare class ShaderParticleGeometry extends Form {
  static from(papyrusObject: PapyrusObject | null): ShaderParticleGeometry | null
  apply(param1: number): void
  remove(param1: number): void
}

// Based on Shout.pex
export declare class Shout extends Form {
  static from(papyrusObject: PapyrusObject | null): Shout | null
  getNthRecoveryTime(n: number): number
  getNthSpell(n: number): Spell | null
  getNthWordOfPower(n: number): WordOfPower | null
  setNthRecoveryTime(n: number, time: number): void
  setNthSpell(n: number, aSpell: Spell | null): void
  setNthWordOfPower(n: number, aWoop: WordOfPower | null): void
}

// Based on SoulGem.pex
export declare class SoulGem extends MiscObject {
  static from(papyrusObject: PapyrusObject | null): SoulGem | null
  getGemSize(): number
  getSoulSize(): number
}

// Based on Sound.pex
export declare class Sound extends Form {
  static from(papyrusObject: PapyrusObject | null): Sound | null
  getDescriptor(): SoundDescriptor | null
  play(akSource: ObjectReference | null): number
  playAndWait(akSource: ObjectReference | null): Promise<boolean>
  static setInstanceVolume(aiPlaybackInstance: number, afVolume: number): void
  static stopInstance(aiPlaybackInstance: number): void
}

// Based on SoundCategory.pex
export declare class SoundCategory extends Form {
  static from(papyrusObject: PapyrusObject | null): SoundCategory | null
  mute(): void
  pause(): void
  setFrequency(param1: number): void
  setVolume(param1: number): void
  unMute(): void
  unPause(): void
}

// Based on SoundDescriptor.pex
export declare class SoundDescriptor extends Form {
  static from(papyrusObject: PapyrusObject | null): SoundDescriptor | null
  getDecibelAttenuation(): number
  getDecibelVariance(): number
  getFrequencyShift(): number
  getFrequencyVariance(): number
  setDecibelAttenuation(dbAttenuation: number): void
  setDecibelVariance(dbVariance: number): void
  setFrequencyShift(frequencyShift: number): void
  setFrequencyVariance(frequencyVariance: number): void
}

// Based on TESModPlatform.pex
export declare class TESModPlatform extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): TESModPlatform | null
  static addItemEx(containerRefr: ObjectReference | null, item: Form | null, countDelta: number, health: number, enchantment: Enchantment | null, maxCharge: number, removeEnchantmentOnUnequip: boolean, chargePercent: number, textDisplayData: string, soul: number, poison: Potion | null, poisonCount: number): void
  static clearTintMasks(targetActor: Actor | null): void
  static createNpc(): ActorBase | null
  static evaluateLeveledNpc(commaSeparatedListOfIds: string): ActorBase | null
  static getNthVtableElement(pointer: Form | null, pointerOffset: number, elementIndex: number): number
  static getSkinColor(base: ActorBase | null): ColorForm | null
  static isPlayerRunningEnabled(): boolean
  static moveRefrToPosition(refr: ObjectReference | null, cell: Cell | null, world: WorldSpace | null, posX: number, posY: number, posZ: number, rotX: number, rotY: number, rotZ: number): void
  static pushTintMask(targetActor: Actor | null, type: number, argb: number, texturePath: string): void
  static pushWornState(worn: boolean, wornLeft: boolean): void
  static resetContainer(container: Form | null): void
  static resizeHeadpartsArray(npc: ActorBase | null, newSize: number): void
  static resizeTintsArray(newSize: number): void
  static setFormIdUnsafe(Form: Form | null, newId: number): void
  static setNpcHairColor(npc: ActorBase | null, hairColor: number): void
  static setNpcRace(npc: ActorBase | null, race: Race | null): void
  static setNpcSex(npc: ActorBase | null, sex: number): void
  static setNpcSkinColor(npc: ActorBase | null, skinColor: number): void
  static setWeaponDrawnMode(actor: Actor | null, mode: number): void
  static updateEquipment(actor: Actor | null, item: Form | null, leftHand: boolean): void
}

// Based on TalkingActivator.pex
export declare class TalkingActivator extends Activator {
  static from(papyrusObject: PapyrusObject | null): TalkingActivator | null
}

// Based on TextureSet.pex
export declare class TextureSet extends Form {
  static from(papyrusObject: PapyrusObject | null): TextureSet | null
  getNthTexturePath(n: number): string
  getNumTexturePaths(): number
  setNthTexturePath(n: number, texturePath: string): void
}

// Based on Topic.pex
export declare class Topic extends Form {
  static from(papyrusObject: PapyrusObject | null): Topic | null
  add(): void
}

// Based on TopicInfo.pex
export declare class TopicInfo extends Form {
  static from(papyrusObject: PapyrusObject | null): TopicInfo | null
  getOwningQuest(): Quest | null
}

// Based on TreeObject.pex
export declare class TreeObject extends Form {
  static from(papyrusObject: PapyrusObject | null): TreeObject | null
  getHarvestSound(): SoundDescriptor | null
  getIngredient(): Form | null
  setHarvestSound(akSoundDescriptor: SoundDescriptor | null): void
  setIngredient(akIngredient: Form | null): void
}

// Based on Ui.pex
export declare class Ui extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Ui | null
  static closeCustomMenu(): void
  static getBool(menuName: string, target: string): boolean
  static getFloat(menuName: string, target: string): number
  static getInt(menuName: string, target: string): number
  static getString(menuName: string, target: string): string
  static invokeBool(menuName: string, target: string, arg: boolean): void
  static invokeBoolA(menuName: string, target: string, args: boolean[] | null): void
  static invokeFloat(menuName: string, target: string, arg: number): void
  static invokeFloatA(menuName: string, target: string, args: number[] | null): void
  static invokeForm(menuName: string, target: string, arg: Form | null): void
  static invokeInt(menuName: string, target: string, arg: number): void
  static invokeIntA(menuName: string, target: string, args: number[] | null): void
  static invokeString(menuName: string, target: string, arg: string): void
  static invokeStringA(menuName: string, target: string, args: string[] | null): void
  static isMenuOpen(menuName: string): boolean
  static isTextInputEnabled(): boolean
  static openCustomMenu(swfPath: string, flags: number): void
  static setBool(menuName: string, target: string, value: boolean): void
  static setFloat(menuName: string, target: string, value: number): void
  static setInt(menuName: string, target: string, value: number): void
  static setString(menuName: string, target: string, value: string): void
}

// Based on VisualEffect.pex
export declare class VisualEffect extends Form {
  static from(papyrusObject: PapyrusObject | null): VisualEffect | null
  play(param1: ObjectReference | null, param2: number, param3: ObjectReference | null): void
  stop(param1: ObjectReference | null): void
}

// Based on VoiceType.pex
export declare class VoiceType extends Form {
  static from(papyrusObject: PapyrusObject | null): VoiceType | null
}

// Based on Weapon.pex
export declare class Weapon extends Form {
  static from(papyrusObject: PapyrusObject | null): Weapon | null
  fire(akSource: ObjectReference | null, akAmmo: Ammo | null): void
  getBaseDamage(): number
  getCritDamage(): number
  getCritEffect(): Spell | null
  getCritEffectOnDeath(): boolean
  getCritMultiplier(): number
  getEnchantment(): Enchantment | null
  getEnchantmentValue(): number
  getEquipType(): EquipSlot | null
  getEquippedModel(): Static | null
  getIconPath(): string
  getMaxRange(): number
  getMessageIconPath(): string
  getMinRange(): number
  getModelPath(): string
  getReach(): number
  getResist(): string
  getSkill(): string
  getSpeed(): number
  getStagger(): number
  getTemplate(): Weapon | null
  getWeaponType(): number
  setBaseDamage(damage: number): void
  setCritDamage(damage: number): void
  setCritEffect(ce: Spell | null): void
  setCritEffectOnDeath(ceod: boolean): void
  setCritMultiplier(crit: number): void
  setEnchantment(e: Enchantment | null): void
  setEnchantmentValue(value: number): void
  setEquipType(type: EquipSlot | null): void
  setEquippedModel(model: Static | null): void
  setIconPath(path: string): void
  setMaxRange(maxRange: number): void
  setMessageIconPath(path: string): void
  setMinRange(minRange: number): void
  setModelPath(path: string): void
  setReach(reach: number): void
  setResist(resist: string): void
  setSkill(skill: string): void
  setSpeed(speed: number): void
  setStagger(stagger: number): void
  setWeaponType(type: number): void
}

// Based on Weather.pex
export declare class Weather extends Form {
  static from(papyrusObject: PapyrusObject | null): Weather | null
  forceActive(abOverride: boolean): void
  getClassification(): number
  getFogDistance(day: boolean, type: number): number
  getSunDamage(): number
  getSunGlare(): number
  getWindDirection(): number
  getWindDirectionRange(): number
  setActive(abOverride: boolean, abAccelerate: boolean): void
  static findWeather(auiType: number): Weather | null
  static getCurrentWeather(): Weather | null
  static getCurrentWeatherTransition(): number
  static getOutgoingWeather(): Weather | null
  static getSkyMode(): number
  static releaseOverride(): void
}

// Based on WordOfPower.pex
export declare class WordOfPower extends Form {
  static from(papyrusObject: PapyrusObject | null): WordOfPower | null
}

// Based on WorldSpace.pex
export declare class WorldSpace extends Form {
  static from(papyrusObject: PapyrusObject | null): WorldSpace | null
}

// Based on Utility.pex
export declare class Utility extends PapyrusObject {
  static from(papyrusObject: PapyrusObject | null): Utility | null
  static captureFrameRate(numFrames: number): string
  static createAliasArray(size: number, fill: Alias | null): PapyrusObject[] | null
  static createBoolArray(size: number, fill: boolean): boolean[] | null
  static createFloatArray(size: number, fill: number): number[] | null
  static createFormArray(size: number, fill: Form | null): PapyrusObject[] | null
  static createIntArray(size: number, fill: number): number[] | null
  static createStringArray(size: number, fill: string): string[] | null
  static endFrameRateCapture(): void
  static gameTimeToString(afGameTime: number): Promise<string>
  static getAverageFrameRate(): number
  static getBudgetCount(): number
  static getBudgetName(aiBudgetNumber: number): string
  static getCurrentBudget(aiBudgetNumber: number): number
  static getCurrentGameTime(): number
  static getCurrentMemory(): number
  static getCurrentRealTime(): number
  static getINIBool(ini: string): boolean
  static getINIFloat(ini: string): number
  static getINIInt(ini: string): number
  static getINIString(ini: string): string
  static getMaxFrameRate(): number
  static getMinFrameRate(): number
  static isInMenuMode(): boolean
  static overBudget(aiBudgetNumber: number): boolean
  static randomFloat(afMin: number, afMax: number): number
  static randomInt(aiMin: number, aiMax: number): number
  static resizeAliasArray(source: PapyrusObject[] | null, size: number, fill: Alias | null): PapyrusObject[] | null
  static resizeBoolArray(source: boolean[] | null, size: number, fill: boolean): boolean[] | null
  static resizeFloatArray(source: number[] | null, size: number, fill: number): number[] | null
  static resizeFormArray(source: PapyrusObject[] | null, size: number, fill: Form | null): PapyrusObject[] | null
  static resizeIntArray(source: number[] | null, size: number, fill: number): number[] | null
  static resizeStringArray(source: string[] | null, size: number, fill: string): string[] | null
  static setINIBool(ini: string, value: boolean): void
  static setINIFloat(ini: string, value: number): void
  static setINIInt(ini: string, value: number): void
  static setINIString(ini: string, value: string): void
  static startFrameRateCapture(): void
  static wait(afSeconds: number): Promise<void>
  static waitGameTime(afHours: number): Promise<void>
  static waitMenuMode(afSeconds: number): Promise<void>
}
