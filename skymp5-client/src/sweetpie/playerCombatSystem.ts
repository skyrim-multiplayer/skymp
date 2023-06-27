import * as sp from "skyrimPlatform";

const weaponTimings = new Map<sp.WeaponType, [number, number]>([
  [sp.WeaponType.Fist, [200, 0]],
  [sp.WeaponType.Sword, [256, 41]],
  [sp.WeaponType.Dagger, [218, 35]],
  [sp.WeaponType.WarAxe, [288, 46]],
  [sp.WeaponType.Mace, [320, 51]],
  [sp.WeaponType.Greatsword, [368, 59]],
  // NOTE: both of the next two weapon types correspond to id=6.
  // TODO(#xyz): do something about it. Maybe we can distinguish them somehow...
  [sp.WeaponType.Battleaxe, [416, 66]],
  [sp.WeaponType.Warhammer, [416, 66]],
  [sp.WeaponType.Bow, [0, 218]],
  [sp.WeaponType.Staff, [320, 51]],
  [sp.WeaponType.Crossbow, [0, 320]],
]);

let blockPlayerControlTimeStamp: number = 0;
let isPlayerControlDisabled: boolean = true;
let playerAttackTimeout: number = 0;

export const start = (): void => {
  sp.once("update", () => setAttackStaminaRestriction());

  // Sup asked for this (Temporary disabled)
  //sp.once('update', registerHandlersIfNeeded);
};

const hasSweetPie = (): boolean => {
  const modCount = sp.Game.getModCount();
  for (let i = 0; i < modCount; ++i) {
    if (sp.Game.getModName(i).toLowerCase().includes('sweetpie')) {
      return true;
    }
  }
  return false;
}

const registerHandlersIfNeeded = (): void => {
  if (!hasSweetPie()) {
    return;
  }

  for (const pattern of ['attackStart*', 'AttackStart*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: (() => { }),
      leave: ((ctx) => blockPlayerAttack(ctx.animEventName.toLowerCase().includes('lefthand'))),
    }, 0x14, 0x14, pattern);
  }

  for (const pattern of ['attackPowerStart*', 'AttackPowerStart*', 'Jump*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: (() => { }),
      leave: (() => {
        playerAttackTimeout = 0;
        activeTimers.clear();
      }),
    }, 0x14, 0x14, pattern);
  }

  sp.on("update", () => {
    if (isPlayerControlDisabled === true && Date.now() - blockPlayerControlTimeStamp >= playerAttackTimeout) {
      sp.Game.getPlayer()!.setDontMove(false);
      isPlayerControlDisabled = false;
    }
  });
};

const activeTimers = new Set<string>();

const blockPlayerAttack = (isLeftHand: boolean): void => {
  sp.once("update", () => {
    const player = sp.Game.getPlayer()!;
    if (player.getAnimationVariableBool("bInJumpState")) {
      return;
    }
    const [delay, timeout] = getTimings(player.getEquippedWeapon(isLeftHand)?.getWeaponType());
    const rnd = Math.random().toString();
    activeTimers.add(rnd);
    sp.Utility.wait(delay / 1000).then(() => {
      if (!activeTimers.delete(rnd)) {
        return;
      }
      sp.once("update", () => {
        isPlayerControlDisabled = true;
        blockPlayerControlTimeStamp = Date.now();

        sp.Game.getPlayer()!.setDontMove(true);
        playerAttackTimeout = timeout;
      });
    });
  });
}

const getTimings = (weapon?: sp.WeaponType): [number, number] => {
  if (weapon === undefined) {
    return getTimings(sp.WeaponType.Fist);
  }

  const timings = weaponTimings.get(weapon);
  if (!timings) {
    return getTimings(sp.WeaponType.Fist);
  }

  return timings;
};

type AttackType = "Std" | "Power" | "Jump" | "Bow" | "Crossbow";
let playerLastStaminaValue = 0;
const staminaAttackMap = new Map<AttackType, number>([
  ["Std", 7],
  ["Power", 35],
  ["Jump", 15],
  ["Bow", 25],
  ["Crossbow", 30],
]);
const setAttackStaminaRestriction = () => {
  if (!hasSweetPie()) {
    return;
  }

  sp.on("update", () => {
    playerLastStaminaValue = sp.Game.getPlayer()!.getActorValue("Stamina");
  });

  for (const pattern of ['attackStart*', 'AttackStart*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: ((ctx) => {
        if (playerLastStaminaValue < (staminaAttackMap.get("Std") ?? 0)) {
          ctx.animEventName = "";
        }
      }),
      leave: (() => { }),
    }, 0x14, 0x14, pattern);
  }

  for (const pattern of ['attackPowerStart*', 'AttackPowerStart*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: ((ctx) => {
        if (playerLastStaminaValue < (staminaAttackMap.get("Power") ?? 0)) {
          ctx.animEventName = "";
        }
      }),
      leave: (() => { }),
    }, 0x14, 0x14, pattern);
  }

  for (const pattern of ['JumpDirectionalStart*', 'JumpStandingStart*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: ((ctx) => {
        if (playerLastStaminaValue < (staminaAttackMap.get("Jump") ?? 0)) {
          ctx.animEventName = "";
        }
      }),
      leave: (() => { }),
    }, 0x14, 0x14, pattern);
  }

  for (const pattern of ['bowAttackStart*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: ((ctx) => {
        if (playerLastStaminaValue < (staminaAttackMap.get("Bow") ?? 0)) {
          ctx.animEventName = "";
        }
      }),
      leave: (() => { }),
    }, 0x14, 0x14, pattern);
  }

  for (const pattern of ['crossbowAttackStart*']) {
    sp.hooks.sendAnimationEvent.add({
      enter: ((ctx) => {
        if (playerLastStaminaValue < (staminaAttackMap.get("Crossbow") ?? 0)) {
          ctx.animEventName = "";
        }
      }),
      leave: (() => { }),
    }, 0x14, 0x14, pattern);
  }
}

const canAttackWithStamina = (aType: AttackType, actor: sp.Actor): boolean => {
  return actor.getActorValue("Stamina") > (staminaAttackMap.get(aType) ?? 0);
}
