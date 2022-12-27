import { Game, Actor } from "skyrimPlatform";

const protection = new Map<number, number>();

const isInDialogue = (ac: Actor): boolean =>
  ac.isInDialogueWithPlayer() || !!ac.getDialogueTarget();

function processOneActor(): void {
  const pc = Game.getPlayer() as Actor;
  const actor = Game.findRandomActor(
    pc.getPositionX(),
    pc.getPositionY(),
    pc.getPositionZ(),
    8192
  ) as Actor;
  const actorId = actor.getFormID();

  const currentProtection = protection.get(actorId) as number;
  if (currentProtection > 0) return;

  if (!actor || actorId === 0x14 || actor.isDisabled() || actor.isDeleted())
    return;

  if (isInDialogue(actor)) {
    // Deleting actor in dialogue crashes Skyrim
    // https://github.com/skyrim-multiplayer/issue-tracker/issues/13
    actor.setPosition(0, 0, 0);
    actor.disableNoWait(true); // Seems to not crash
    return;
  }
  actor.disable(false).then(() => {
    const ac = Actor.from(Game.getFormEx(actorId));
    if (!ac || isInDialogue(ac)) return;
    ac.delete();
  });
}

export function updateWc(): void {
  return processOneActor();
}

export function modWcProtection(actorId: number, mod: number): void {
  const currentProtection = protection.get(actorId);
  protection.set(actorId, currentProtection ? currentProtection + mod : mod);
}
