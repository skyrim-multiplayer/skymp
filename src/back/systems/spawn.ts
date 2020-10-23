import { System, Log, SystemContext } from "./system";

const spawnpoints = [
  { pos: [22659, -8697, -3594], worldOrCell: 0x1a26f, angleZ: 268 },
];

function randomInteger(min: number, max: number) {
  const rand = min + Math.random() * (max + 1 - min);
  return Math.floor(rand);
}

export class Spawn implements System {
  systemName = "Spawn";
  constructor(private log: Log) {}

  async initAsync(ctx: SystemContext): Promise<void> {
    ctx.gm.on("spawnAllowed", (userId: number, userProfileId: number) => {
      // TODO: Show race menu if character is not created after relogging
      let actorId = ctx.svr.getActorsByProfileId(userProfileId)[0];
      if (actorId) {
        this.log("Loading character", actorId.toString(16));
        ctx.svr.setEnabled(actorId, true);
        ctx.svr.setUserActor(userId, actorId);
      } else {
        const idx = randomInteger(0, spawnpoints.length - 1);
        actorId = ctx.svr.createActor(
          0,
          spawnpoints[idx].pos,
          spawnpoints[idx].angleZ,
          spawnpoints[idx].worldOrCell,
          userProfileId
        );
        this.log("Creating character", actorId.toString(16));
        ctx.svr.setUserActor(userId, actorId);
        ctx.svr.setRaceMenuOpen(actorId, true);
      }
    });
  }

  disconnect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (actorId !== 0) ctx.svr.setEnabled(actorId, false);
  }
}
