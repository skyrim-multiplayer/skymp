import { ClientListener, CombinedController, Sp } from "./clientListener";
import { NiPoint3 } from "../../sync/movement";
import { ObjectReferenceEx } from "../../extensions/objectReferenceEx";
import { Actor } from "skyrimPlatform";
import { logTrace } from "../../logging";

const DELETE_DELAY_TICKS = 15;
// Upper bound on update ticks we will wait for the `disable(false)` promise
// to resolve before dropping the pending entry. Prevents a permanent state
// leak (and permanently-suppressed processing) if the promise never fires.
const MAX_DISABLE_WAIT_TICKS = 300;

interface PendingDeleteState {
  disabled: boolean;
  ticksRemaining: number;
  ticksWaitingForDisable: number;
}

export class WorldCleanerService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    this.controller.on("update", () => this.onUpdate());
    this.controller.emitter.on("gameLoad", () => this.onGameLoad());
  }

  modWcProtection(actorId: number, mod: number): void {
    const currentProtection = this.protection.get(actorId);
    this.protection.set(actorId, currentProtection ? currentProtection + mod : mod);
  }

  getWcProtection(actorId: number): number {
    return this.protection.get(actorId) || 0;
  }

  isPendingDelete(actorId: number): boolean {
    return this.pendingDelete.has(actorId);
  }

  private onGameLoad() {
    this.pendingDelete.clear();

    let player = this.sp.Game.getPlayer();
    if (!player) {
      return;
    }

    this.initialPos = ObjectReferenceEx.getPos(player);
    this.initialCellOrWorld = ObjectReferenceEx.getWorldOrCell(player);
  }

  private onUpdate() {
    this.processPendingDeletions();
    this.processOneActor();
  }

  private processPendingDeletions() {
    if (this.pendingDelete.size === 0) {
      return;
    }

    const readyForDelete: number[] = [];
    const abandoned: number[] = [];
    this.pendingDelete.forEach((state, actorId) => {
      if (!state.disabled) {
        state.ticksWaitingForDisable++;
        if (state.ticksWaitingForDisable > MAX_DISABLE_WAIT_TICKS) {
          abandoned.push(actorId);
        }
        return;
      }
      if (state.ticksRemaining > 0) {
        state.ticksRemaining--;
        return;
      }
      readyForDelete.push(actorId);
    });

    for (const actorId of abandoned) {
      this.pendingDelete.delete(actorId);
      logTrace(this, "Pending delete: disable never resolved, dropping", actorId.toString(16));
    }

    for (const actorId of readyForDelete) {
      this.pendingDelete.delete(actorId);
      const ac = this.sp.Actor.from(this.sp.Game.getFormEx(actorId));
      if (!ac) {
        logTrace(this, "Pending delete: form gone, dropping", actorId.toString(16));
        continue;
      }
      if (this.isActorInDialogue(ac)) {
        logTrace(this, "Pending delete: actor entered dialogue, aborting delete", actorId.toString(16));
        continue;
      }
      logTrace(this, "Pending delete: final delete", actorId.toString(16));
      ac.delete();
    }
  }

  private processOneActor() {
    const pc = this.sp.Game.getPlayer();
    if (pc === null) {
      return;
    }

    const actor = this.sp.Game.findRandomActor(
      pc.getPositionX(),
      pc.getPositionY(),
      pc.getPositionZ(),
      8192
    );
    if (actor === null) {
      return;
    }

    const actorId = actor.getFormID();

    if (this.pendingDelete.has(actorId)) {
      return;
    }

    const currentProtection = this.protection.get(actorId) || 0;
    if (currentProtection > 0) {
      return;
    }

    if (actorId === 0x14 || actor.isDisabled() || actor.isDeleted()) {
      return;
    }

    if (this.isActorInDialogue(actor)) {
      // Deleting actor in dialogue crashes Skyrim
      // https://github.com/skyrim-multiplayer/issue-tracker/issues/13
      actor.setPosition(0, 0, 0);
      actor.disableNoWait(true); // Seems to not crash
      return;
    }

    // Keep vanila pre-placed bodies, but delete player bodies
    if (actor.isDead() && actorId < 0xff000000) {
      actor.blockActivation(true);
      return;
    }

    const pos = ObjectReferenceEx.getPos(actor);
    const cellOrWorld = ObjectReferenceEx.getWorldOrCell(actor);

    const chickenRace = 0xa919d;

    // We discovered anomaly chickens that fail to Disable if we load game near to them
    // Refs: 106C22, 106C23
    if (actorId < 0xff000000 && actor.getRace()?.getFormID() === chickenRace) {
      if (this.initialPos && ObjectReferenceEx.getDistanceNoZ(pos, this.initialPos) < 4096) {
        if (cellOrWorld === this.initialCellOrWorld) {
          if (this.isActorInDialogue(actor)) {
            return;
          }
          logTrace(this, `Deleting chicken anomaly`, actorId.toString(16));
          actor.killSilent(null);
          actor.blockActivation(true);
          actor.disableNoWait(false);
          actor.setAlpha(0, false);
          return;
        }
      }
    }

    // Deferred delete: mark pending, disable, then delete N ticks after disable resolves.
    const state: PendingDeleteState = { disabled: false, ticksRemaining: DELETE_DELAY_TICKS, ticksWaitingForDisable: 0 };
    this.pendingDelete.set(actorId, state);
    logTrace(this, "Pending delete: scheduling disable", actorId.toString(16));

    actor.disable(false).then(() => {
      const currentState = this.pendingDelete.get(actorId);
      if (!currentState) {
        return;
      }
      currentState.disabled = true;
      logTrace(this, "Pending delete: disable resolved", actorId.toString(16));
    });
  }

  private isActorInDialogue(ac: Actor) {
    return ac.isInDialogueWithPlayer() || ac.getDialogueTarget() !== null;
  }

  private protection = new Map<number, number>();
  private pendingDelete = new Map<number, PendingDeleteState>();
  private initialPos?: NiPoint3;
  private initialCellOrWorld?: number;
}
