import { Game, Actor } from "skyrimPlatform";

const protection = new Map<number, number>();

function processOneActor(): void {
  const pc = Game.getPlayer() as Actor;
  const actor = Game.findClosestActor(
    pc.getPositionX(),
    pc.getPositionY(),
    pc.getPositionZ(),
    8192
  ) as Actor;
  const actorId = actor.getFormID();
  const currentProtection = protection.get(actorId) as number;
  const ac = Actor.from(Game.getFormEx(actorId));
  if (actor != ac || actorId === 0x14 || actor.isDisabled() || actor.isDeleted())
    return;
  if (currentProtection) {
  actor.disable(false).then(() => {
    ac.delete();
  });
  }
}

export function updateWc(): void {
  return processOneActor();
}

export function modWcProtection(actorId: number, mod: number): void {
  const currentProtection = protection.get(actorId);
  protection.set(actorId, currentProtection ? currentProtection + mod : mod);
}
