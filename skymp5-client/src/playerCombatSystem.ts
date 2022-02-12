import * as sp from "skyrimPlatform";

const attackStartEventPattern = "attackStart*";
const attackStart2EventPattern = "AttackStart*";
const attackPowerStartEventPattern = "AttackPowerStart*";
const weaponTimeouts = new Map<sp.WeaponType, number>([
  [sp.WeaponType.Fist, 180],
  [sp.WeaponType.Sword, 250],
  [sp.WeaponType.Dagger, 180],
  [sp.WeaponType.WarAxe, 250],
  [sp.WeaponType.Mace, 300],
  [sp.WeaponType.Greatsword, 450],
  [sp.WeaponType.Battleaxe, 450],
  [sp.WeaponType.Warhammer, 520],
  [sp.WeaponType.Bow, 0],
  [sp.WeaponType.Staff, 180],
  [sp.WeaponType.Crossbow, 0]
]);

let blockPlayerControlTimeStamp: number = 0;
let isPlayerControlDisabled: boolean = true;
let playerAttackTimeout: number = 0;

export const start = (): void => { };

sp.hooks.sendAnimationEvent.add({
  enter: (() => { }),
  leave: (() => blockPlayerAttack())
}, 0x14, 0x14, attackStartEventPattern);

sp.hooks.sendAnimationEvent.add({
  enter: (() => { }),
  leave: (() => blockPlayerAttack())
}, 0x14, 0x14, attackStart2EventPattern);

sp.hooks.sendAnimationEvent.add({
  enter: (() => { }),
  leave: (() => blockPlayerAttack())
}, 0x14, 0x14, attackPowerStartEventPattern);

sp.on("update", () => {
  if (isPlayerControlDisabled === true && Date.now() - blockPlayerControlTimeStamp >= playerAttackTimeout) {
    sp.Game.getPlayer()!.setDontMove(false);
    isPlayerControlDisabled = false;
  }
});

const blockPlayerAttack = (): void => {
  sp.once("update", () => {
    sp.Utility.wait(0.5).then(() => {
      sp.once("update", () => {
        isPlayerControlDisabled = true;
        blockPlayerControlTimeStamp = Date.now();

        const player = sp.Game.getPlayer()!;
        player.setDontMove(true);
        playerAttackTimeout = calculateTimeout(player.getEquippedWeapon(true)?.getWeaponType(), player.getEquippedWeapon(false)?.getWeaponType());
      });
    });
  });
}

const calculateTimeout = (leftHand?: sp.WeaponType, righHand?: sp.WeaponType): number => {
  let result = 0;
  if (leftHand) {
    result = weaponTimeouts.get(leftHand) ?? 0;
  }
  if (righHand) {
    result += weaponTimeouts.get(righHand) ?? 0;
    result /= leftHand ? 2 : 1;
  }

  return result || (weaponTimeouts.get(sp.WeaponType.Fist) ?? 0);
};
