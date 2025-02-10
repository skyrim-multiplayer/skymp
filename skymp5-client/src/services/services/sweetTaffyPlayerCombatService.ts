import { logTrace, logError } from "../../logging";
import { ClientListener, Sp, CombinedController } from "./clientListener";
import { WeaponType } from "skyrimPlatform";

// TODO: move to service
let playerLastStaminaValue = 0;
let playerLastIsSprintingValue = false;
let blockPlayerControlTimeStamp = 0;
let isPlayerControlDisabled = true;
let playerAttackTimeout = 0;
let activeTimers = new Set<string>();

interface WeaponTimings {
  fist: [number, number];
  sword: [number, number];
  dagger: [number, number];
  warAxe: [number, number];
  mace: [number, number];
  greatsword: [number, number];
  battleaxe: [number, number];
  warhammer: [number, number];
  bow: [number, number];
  staff: [number, number];
  crossbow: [number, number];
}

interface SweetTaffyPlayerCombatServiceSettings {
  weaponTimings?: WeaponTimings;
}

type AttackType = "Std" | "Power" | "Jump" | "Bow" | "Crossbow";

// TODO: move to config
const staminaAttackMap = new Map<AttackType, number>([
  ["Std", 7],
  ["Power", 35],
  ["Jump", 15],
  ["Bow", 25],
  ["Crossbow", 30],
]);

// TODO: consider splitting this service (separate stamina and timer management)
// TODO: subscribe events/hooks in constructor
export class SweetTaffyPlayerCombatService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();

    if (!this.hasSweetPie()) {
      logTrace(this, "SweetTaffy features disabled");
    }
    else {
      logTrace(this, "SweetTaffy features enabled");
    }

    this.controller.once("update", () => this.onceUpdate());
  }

  private onceUpdate() {
    this.setAttackStaminaRestriction();
    this.registerHandlersIfNeeded();
  }

  private setAttackStaminaRestriction() {
    if (!this.hasSweetPie()) {
      return;
    }

    this.controller.on("update", () => {
      const player = this.sp.Game.getPlayer()!;
      playerLastStaminaValue = player.getActorValue("Stamina");
      playerLastIsSprintingValue = player.isSprinting();
    });

    for (const pattern of ['SprintStart']) {
      const sp = this.sp;
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastStaminaValue < 7.5) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['blockStart']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastIsSprintingValue) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['attackStart*', 'AttackStart*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastStaminaValue < (staminaAttackMap.get("Std") ?? 0)) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['attackPowerStart*', 'AttackPowerStart*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastStaminaValue < (staminaAttackMap.get("Power") ?? 0)) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['JumpDirectionalStart*', 'JumpStandingStart*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastStaminaValue < (staminaAttackMap.get("Jump") ?? 0)) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['bowAttackStart*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastStaminaValue < (staminaAttackMap.get("Bow") ?? 0)) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['crossbowAttackStart*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: ((ctx) => {
          if (playerLastStaminaValue < (staminaAttackMap.get("Crossbow") ?? 0)) {
            ctx.animEventName = "";
          }
        }),
        leave: (() => { }),
      }, 0x14, 0x14, pattern);
    }
  }

  private registerHandlersIfNeeded(): void {
    const self = this;

    if (!this.hasSweetPie()) {
      return;
    }

    for (const pattern of ['attackStart*', 'AttackStart*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: (() => { }),
        leave: ((ctx) => self.blockPlayerAttack(ctx.animEventName.toLowerCase().includes('lefthand'))),
      }, 0x14, 0x14, pattern);
    }

    for (const pattern of ['attackPowerStart*', 'AttackPowerStart*', 'Jump*']) {
      this.sp.hooks.sendAnimationEvent.add({
        enter: (() => { }),
        leave: (() => {
          playerAttackTimeout = 0;
          activeTimers.clear();
        }),
      }, 0x14, 0x14, pattern);
    }

    this.controller.on("update", () => {
      if (isPlayerControlDisabled === true && Date.now() - blockPlayerControlTimeStamp >= playerAttackTimeout) {
        this.sp.Game.getPlayer()!.setDontMove(false);
        isPlayerControlDisabled = false;
      }
    });
  }

  private blockPlayerAttack(isLeftHand: boolean): void {
    this.controller.once("update", () => {
      const player = this.sp.Game.getPlayer()!;
      if (player.getAnimationVariableBool("bInJumpState")) {
        return;
      }
      const [delay, timeout] = this.getTimings(player.getEquippedWeapon(isLeftHand)?.getWeaponType());
      const rnd = Math.random().toString();
      activeTimers.add(rnd);
      this.sp.Utility.wait(delay / 1000).then(() => {
        if (!activeTimers.delete(rnd)) {
          return;
        }
        this.controller.once("update", () => {
          isPlayerControlDisabled = true;
          blockPlayerControlTimeStamp = Date.now();

          this.sp.Game.getPlayer()!.setDontMove(true);
          playerAttackTimeout = timeout;
        });
      });
    });
  }

  private getAndValidateTimingsConfigKey(key: keyof WeaponTimings, weaponTimings: unknown): [number, number] {
    const value = (weaponTimings as Record<string, unknown>)[key];

    if (value && Array.isArray(value) && value.length === 2) {
      const element0 = value[0];
      if (typeof element0 !== "number") {
        logError(this, `Invalid timings config for weapon type`, key, value);
        return [0, 0];
      }
      const element1 = value[1];
      if (typeof element1 !== "number") {
        logError(this, `Invalid timings config for weapon type`, key, value);
        return [0, 0];
      }
      return [element0, element1];
    }

    return [0, 0];
  };

  private getSettingsFromFile(): WeaponTimings | null {
    const sweetTaffyPlayerCombatService = this.sp.settings["skymp5-client"]["sweetTaffyPlayerCombatService"];

    if (!sweetTaffyPlayerCombatService || typeof sweetTaffyPlayerCombatService !== "object") {
      logError(this, `No sweetTaffyPlayerCombatService settings found`);
      return null;
    }

    const weaponTimings = (sweetTaffyPlayerCombatService as Record<string, unknown>).weaponTimings;
    if (!weaponTimings || typeof weaponTimings !== "object") {
      logError(this, `No weaponTimings settings found`);
      return null;
    }

    return weaponTimings as WeaponTimings;
  }

  private getSettingsDefault(): WeaponTimings | null {
    return {
      fist: [0, 30],
      sword: [0, 30],
      dagger: [0, 60],
      warAxe: [0, 60],
      mace: [0, 65],
      greatsword: [0, 65],
      // Note: both of Battleaxe and Warhammer weapon types correspond to id=6.
      battleaxe: [0, 70],
      warhammer: [0, 70],
      bow: [0, 70],
      staff: [0, 70],
      crossbow: [0, 70]
    };
  }

  private getTimingsFromConfig(weapon?: WeaponType): [number, number] | null {
    if (!weapon) {
      weapon = WeaponType.Fist;
    }

    // skymp5-client-settings.txt is perfectly editable by users, so we don't use it for now.
    // It's well-tested though, so we can enable it once we have a protection mechanism.

    // const weaponTimings = this.getSettingsFromFile();
    const weaponTimings = this.getSettingsDefault();
    if (!weaponTimings) {
      return null;
    }

    const weaponTimingsResult: WeaponTimings = {
      fist: [0, 0],
      sword: [0, 0],
      dagger: [0, 0],
      warAxe: [0, 0],
      mace: [0, 0],
      greatsword: [0, 0],
      battleaxe: [0, 0],
      warhammer: [0, 0],
      bow: [0, 0],
      staff: [0, 0],
      crossbow: [0, 0]
    };
    weaponTimingsResult.fist = this.getAndValidateTimingsConfigKey("fist", weaponTimings);
    weaponTimingsResult.sword = this.getAndValidateTimingsConfigKey("sword", weaponTimings);
    weaponTimingsResult.dagger = this.getAndValidateTimingsConfigKey("dagger", weaponTimings);
    weaponTimingsResult.warAxe = this.getAndValidateTimingsConfigKey("warAxe", weaponTimings);
    weaponTimingsResult.mace = this.getAndValidateTimingsConfigKey("mace", weaponTimings);
    weaponTimingsResult.greatsword = this.getAndValidateTimingsConfigKey("greatsword", weaponTimings);
    weaponTimingsResult.battleaxe = this.getAndValidateTimingsConfigKey("battleaxe", weaponTimings);
    weaponTimingsResult.warhammer = this.getAndValidateTimingsConfigKey("warhammer", weaponTimings);
    weaponTimingsResult.bow = this.getAndValidateTimingsConfigKey("bow", weaponTimings);
    weaponTimingsResult.staff = this.getAndValidateTimingsConfigKey("staff", weaponTimings);
    weaponTimingsResult.crossbow = this.getAndValidateTimingsConfigKey("crossbow", weaponTimings);

    switch (weapon) {
      case WeaponType.Fist:
        return weaponTimingsResult.fist;
      case WeaponType.Sword:
        return weaponTimingsResult.sword;
      case WeaponType.Dagger:
        return weaponTimingsResult.dagger;
      case WeaponType.WarAxe:
        return weaponTimingsResult.warAxe;
      case WeaponType.Mace:
        return weaponTimingsResult.mace;
      case WeaponType.Greatsword:
        return weaponTimingsResult.greatsword;
      case WeaponType.Battleaxe:
        return weaponTimingsResult.battleaxe;
      case WeaponType.Warhammer:
        return weaponTimingsResult.warhammer;
      case WeaponType.Bow:
        return weaponTimingsResult.bow;
      case WeaponType.Staff:
        return weaponTimingsResult.staff;
      case WeaponType.Crossbow:
        return weaponTimingsResult.crossbow;
      default:
        logError(this, `No timings found for weapon type`, weapon);
        return null;
    }
  }

  private getTimings(weapon?: WeaponType): [number, number] {
    let timingsFromConfig = this.getTimingsFromConfig(weapon);

    if (!weapon || !timingsFromConfig) {
      weapon = WeaponType.Fist;
      timingsFromConfig = this.getTimingsFromConfig(weapon);
    }

    if (!timingsFromConfig) {
      logError(this, `No timings found for weapon type`, weapon);
      return [0, 0];
    }

    return timingsFromConfig;
  };

  private hasSweetPie(): boolean {
    const modCount = this.sp.Game.getModCount();
    for (let i = 0; i < modCount; ++i) {
      if (this.sp.Game.getModName(i).toLowerCase().includes('sweetpie')) {
        return true;
      }
    }
    return false;
  }
}
