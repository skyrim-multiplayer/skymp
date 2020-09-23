import { System, Log, SystemContext } from "./system";

function randomInteger(min: number, max: number) {
  const rand = min + Math.random() * (max + 1 - min);
  return Math.floor(rand);
}

export class Spawn implements System {
  systemName = "Spawn";
  constructor(private log: Log) {}

  async initAsync(ctx: SystemContext): Promise<void> {
    ctx.gm.on("spawnAllowed", (userId: number) => {
      const formId = 0xff000001 + userId;

      try {
        ctx.svr.destroyActor(formId);
      } catch (e) {}

      const spawnpoints = [
        { pos: [22659, -8697, -3594], worldOrCell: 0x1a26f, angleZ: 268 },
      ];
      const idx = randomInteger(0, spawnpoints.length - 1);

      ctx.svr.createActor(
        formId,
        spawnpoints[idx].pos,
        spawnpoints[idx].angleZ,
        spawnpoints[idx].worldOrCell
      );

      ctx.svr.setUserActor(userId, formId);
      ctx.svr.setRaceMenuOpen(formId, true);
    });
  }

  disconnect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (actorId !== 0) ctx.svr.destroyActor(actorId);
  }
}
