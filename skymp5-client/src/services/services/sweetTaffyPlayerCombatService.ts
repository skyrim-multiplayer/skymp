import { logTrace, logError } from "../../logging";
import { ClientListener, Sp, CombinedController } from "./clientListener";
import { WeaponType } from "skyrimPlatform";

// TODO: move to service
let playerLastStaminaValue = 0;
let blockPlayerControlTimeStamp = 0;
let isPlayerControlDisabled = true;
let playerAttackTimeout = 0;
let activeTimers = new Set<string>();

// TODO: move to config
const weaponTimings = new Map<WeaponType, [number, number]>([
  [WeaponType.Fist, [0, 30]],
  [WeaponType.Sword, [0, 30]],
  [WeaponType.Dagger, [0, 60]],
  [WeaponType.WarAxe, [0, 60]],
  [WeaponType.Mace, [0, 65]],
  [WeaponType.Greatsword, [0, 65]],
  // Note: both of Battleaxe and Warhammer weapon types correspond to id=6.
  [WeaponType.Battleaxe, [0, 70]],
  [WeaponType.Warhammer, [0, 70]],
  [WeaponType.Bow, [0, 70]],
  [WeaponType.Staff, [0, 70]],
  [WeaponType.Crossbow, [0, 70]],
]);

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
      playerLastStaminaValue = this.sp.Game.getPlayer()!.getActorValue("Stamina");
    });

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
        this.sp.Utility.wait(1).then(() => {
          logTrace(this, "setDontMode false - enable controls");
          this.sp.Game.enablePlayerControls(true, false, false, true, false, false, false, false, 0);
        });
        logTrace(this, "setDontMode false");
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
          this.sp.Utility.wait(0.5).then(() => {
            this.sp.Game.disablePlayerControls(true, false, false, true, false, false, false, false, 0);
          });
          logTrace(this, "setDontMode true");
          playerAttackTimeout = timeout;
        });
      });
    });
  }

  private getTimings(weapon?: WeaponType): [number, number] {
    if (!weapon || !weaponTimings.has(weapon)) {
      weapon = WeaponType.Fist;
    }

    const res = weaponTimings.get(weapon);

    if (res === undefined) {
      logError(this, `No timings found for weapon type`, weapon);
      return [0, 0];
    }

    return res;
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
