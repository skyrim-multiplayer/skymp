import {
  Game,
  TESModPlatform,
  ObjectReference,
  Utility,
  Cell,
  Debug,
  hooks,
  once,
  WorldSpace,
} from "skyrimPlatform";

const spawnPosition = [227, 239, 53];
const spawnRotation = [0, 0, 0];
const greenZoneId = 0x165a7;

let lastHp = 1;
let gAllowGetUp = true;
let wasInGreenZone = false;
let pcIsInGreenZone = false;

hooks.sendAnimationEvent.add({
  enter(ctx) {
    if (ctx.selfId !== 0x14) return;
    if (!gAllowGetUp && ctx.animEventName === "GetUpBegin")
      ctx.animEventName = "";
    if (
      pcIsInGreenZone &&
      (ctx.animEventName.toLowerCase().includes("attack") ||
        ctx.animEventName.toLowerCase().includes("bash"))
    )
      ctx.animEventName = "";
  },
  leave() {
    return;
  },
});

const isGreenZone = () => {
  const pc = Game.getPlayer();
  const cell = pc.getParentCell();
  return cell && cell.getFormID() == greenZoneId;
};

const blockPvpInTemple = (): void => {
  pcIsInGreenZone = isGreenZone();

  if (pcIsInGreenZone) Game.getPlayer().restoreActorValue("health", 1000000);

  if (wasInGreenZone != pcIsInGreenZone) {
    wasInGreenZone = pcIsInGreenZone;
    TESModPlatform.setWeaponDrawnMode(
      Game.getPlayer(),
      pcIsInGreenZone ? 0 : -1
    );
  }
};

const moveTo = (worldOrCellId: number, pos: number[], rot: number[]) => {
  TESModPlatform.moveRefrToPosition(
    Game.getPlayer(),
    Cell.from(Game.getFormEx(worldOrCellId)),
    WorldSpace.from(Game.getFormEx(worldOrCellId)),
    pos[0],
    pos[1],
    pos[2],
    rot[0],
    rot[1],
    rot[2]
  );
};

const playSpell = () => {
  const activator = ObjectReference.from(Game.getFormEx(0x103263));
  if (activator) activator.activate(Game.getPlayer(), false);
};

const die = () => {
  gAllowGetUp = false;
  const actor = Game.getPlayer();
  actor.pushActorAway(actor, 0);
  actor.damageActorValue("health", 1000);
  Utility.wait(6).then(() => {
    gAllowGetUp = true;
    moveTo(greenZoneId, spawnPosition, spawnRotation);
    once("update", () => {
      Utility.wait(2).then(() => {
        once("update", () => {
          playSpell();
          Debug.sendAnimationEvent(actor, "GetUpBegin");
          actor.restoreActorValue("health", 10000000);
        });
      });
    });
  });
};

const handleDeath = () => {
  let hp = Game.getPlayer().getActorValuePercentage("health");
  hp = hp <= 0 ? 0 : hp;
  if (lastHp != hp) {
    lastHp = hp;
    if (hp <= 0) die();
  }
};

export const update = (): void => {
  Game.getPlayer().startDeferredKill();

  blockPvpInTemple();
  handleDeath();
};
