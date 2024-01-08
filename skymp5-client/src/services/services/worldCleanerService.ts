import { Game, Actor } from "skyrimPlatform";
import { ClientListener, CombinedController, Sp } from "./clientListener";

export class WorldCleanerService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.on("update", () => this.onUpdate());
  }

  modWcProtection(actorId: number, mod: number): void {
    const currentProtection = this.protection.get(actorId);
    this.protection.set(actorId, currentProtection ? currentProtection + mod : mod);
  }
  
  getWcProtection(actorId: number): number {
    return this.protection.get(actorId) || 0;
  }

  private onUpdate() {
    this.processOneActor();
  }

  private processOneActor() {
    const pc = Game.getPlayer();
    if (pc === null) return;

    const actor = Game.findRandomActor(
      pc.getPositionX(),
      pc.getPositionY(),
      pc.getPositionZ(),
      8192
    );
    if (actor === null) return;

    const actorId = actor.getFormID();

    const currentProtection = this.protection.get(actorId) || 0;
    if (currentProtection > 0) return;

    if (actorId === 0x14 || actor.isDisabled() || actor.isDeleted()) return;

    if (this.isActorInDialogue(actor)) {
      // Deleting actor in dialogue crashes Skyrim
      // https://github.com/skyrim-multiplayer/issue-tracker/issues/13
      actor.setPosition(0, 0, 0);
      actor.disableNoWait(true); // Seems to not crash
      return;
    }

    if (actor.isDead()) {
      actor.blockActivation(true);
      return;
    }

    actor.disable(false).then(() => {
      const ac = Actor.from(Game.getFormEx(actorId));
      if (!ac || this.isActorInDialogue(ac)) return;
      ac.delete();
    });
  }

  private isActorInDialogue(ac: Actor) {
    return ac.isInDialogueWithPlayer() || ac.getDialogueTarget() !== null;
  }

  private protection = new Map<number, number>();
}
