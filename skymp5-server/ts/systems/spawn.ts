import { Settings } from '../settings';
import { Log, System, SystemContext } from './system';

type Mp = any; // TODO

function randomInteger(min: number, max: number) {
  const rand = min + Math.random() * (max + 1 - min);
  return Math.floor(rand);
}

function toSpawnPoint(point: {
  pos: number[];
  worldOrCell: string;
  angleZ: number;
}) {
  return {
    pos: point.pos,
    rot: [0, 0, point.angleZ],
    cellOrWorldDesc: point.worldOrCell,
  };
}

export class Spawn implements System {
  systemName = 'Spawn';
  constructor(private log: Log) {}

  async initAsync(ctx: SystemContext): Promise<void> {
    const settingsObject = await Settings.get();
    const listenerFn = (
      userId: number,
      userProfileId: number,
      discordRoleIds: string[],
      discordId?: string,
    ) => {
      const { startPoints, starterInventory } = settingsObject;
      // TODO: Show race menu if character is not created after relogging
      const previousActorId = ctx.svr.getUserActor(userId);
      let actorId = ctx.svr.getActorsByProfileId(userProfileId)[0];
      if (actorId) {
        this.log('Loading character', actorId.toString(16));
        ctx.svr.setEnabled(actorId, true);
        if (previousActorId && previousActorId !== actorId) {
          try {
            const manager = (globalThis as any).voiceActivityManager;
            manager?.removePlayer?.(String(previousActorId));
          } catch (e) {
            console.error('[Voice Activity] cleanup failed on respawn/load:', e);
          }
        }
        ctx.svr.setUserActor(userId, actorId);
      } else {
        const idx = randomInteger(0, startPoints.length - 1);
        const startPoint = startPoints[idx];
        actorId = ctx.svr.createActor(
          0,
          startPoint.pos,
          startPoint.angleZ,
          +startPoint.worldOrCell,
          userProfileId,
        );
        this.log('Creating character', actorId.toString(16));
        if (previousActorId && previousActorId !== actorId) {
          try {
            const manager = (globalThis as any).voiceActivityManager;
            manager?.removePlayer?.(String(previousActorId));
          } catch (e) {
            console.error('[Voice Activity] cleanup failed on respawn/create:', e);
          }
        }
        ctx.svr.setUserActor(userId, actorId);

        const mp = ctx.svr as unknown as Mp;
        mp.set(actorId, 'spawnPoint', toSpawnPoint(startPoint));
        if (starterInventory.entries.length > 0) {
          mp.set(actorId, 'inventory', starterInventory);
        }
        ctx.svr.setRaceMenuOpen(actorId, true);
      }

      const mp = ctx.svr as unknown as Mp;
      mp.set(actorId, 'private.discordRoles', discordRoleIds);

      if (discordId !== undefined) {
        // This helps us to test if indexes registration works in LoadForm or not
        if (mp.get(actorId, 'private.indexed.discordId') !== discordId) {
          mp.set(actorId, 'private.indexed.discordId', discordId);
        }

        const forms = mp.findFormsByPropertyValue(
          'private.indexed.discordId',
          discordId,
        ) as number[];
        console.log(`Found forms ${forms}`);
      }
    };
    ctx.gm.on('spawnAllowed', listenerFn);
    (ctx.svr as any)._onSpawnAllowed = listenerFn;
  }

  disconnect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (actorId !== 0) {
      ctx.svr.setEnabled(actorId, false);
    }
  }
}
