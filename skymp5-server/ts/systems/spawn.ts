import { Settings } from "../settings";
import { System, Log, SystemContext } from "./system";

type Mp = any; // TODO

function randomInteger(min: number, max: number) {
  const rand = min + Math.random() * (max + 1 - min);
  return Math.floor(rand);
}

export class Spawn implements System {
  systemName = "Spawn";
  constructor(private log: Log) {}

  async initAsync(ctx: SystemContext): Promise<void> {
    const settingsObject = await Settings.get();
    ctx.gm.on("spawnAllowed", (userId: number, userProfileId: number, discordRoleIds: string[], discordId: string | undefined) => {
      const { startPoints } = settingsObject;
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

      const mp = ctx.svr as unknown as Mp;
      mp.set(actorId, "private.discordRoles", discordRoleIds);

      if (discordId !== undefined) {
        // This helps us to test if indexes registration works in LoadForm or not
        if (mp.get(actorId, "private.indexed.discordId") !== discordId) {
          mp.set(actorId, "private.indexed.discordId", discordId);
        }

        const forms = mp.findFormsByPropertyValue("private.indexed.discordId", discordId) as number[];
        console.log(`Found forms ${forms}`);
      }
    });
  }

  disconnect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (actorId !== 0) ctx.svr.setEnabled(actorId, false);
  }
}
