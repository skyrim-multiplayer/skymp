import { Settings } from "../settings";
import { System, Log, SystemContext } from "./system";
import { Mp } from "../../../skymp5-functions-lib/src/types/mp"

function randomInteger(min: number, max: number) {
  const rand = min + Math.random() * (max + 1 - min);
  return Math.floor(rand);
}

export class Spawn implements System {
  systemName = "Spawn";
  constructor(private log: Log) {}

  async initAsync(ctx: SystemContext): Promise<void> {
    ctx.gm.on("spawnAllowed", (userId: number, userProfileId: number, discordRoleIds: string[]) => {
      const { startPoints } = Settings.get();
      // TODO: Show race menu if character is not created after relogging
      let actorId = ctx.svr.getActorsByProfileId(userProfileId)[0];
      if (actorId) {
        this.log("Loading character", actorId.toString(16));
        ctx.svr.setEnabled(actorId, true);
        ctx.svr.setUserActor(userId, actorId);
      } else {
        const idx = randomInteger(0, startPoints.length - 1);
        actorId = ctx.svr.createActor(
          0,
          startPoints[idx].pos,
          startPoints[idx].angleZ,
          +startPoints[idx].worldOrCell,
          userProfileId
        );
        this.log("Creating character", actorId.toString(16));
        ctx.svr.setUserActor(userId, actorId);
        ctx.svr.setRaceMenuOpen(actorId, true);
      }
      (ctx.svr as unknown as Mp).set(actorId, "private.discordRoles", discordRoleIds);
    });
  }

  disconnect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (actorId !== 0) ctx.svr.setEnabled(actorId, false);
  }
}
