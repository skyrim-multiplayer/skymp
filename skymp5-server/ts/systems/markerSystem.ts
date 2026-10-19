import { Content, Log, System, SystemContext } from './system';

type Mp = any;

export class MarkerSystem implements System {
  systemName = 'MarkerSystem';
  constructor(private log: Log) {}

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext,
  ): void {
    if (type !== 'markerDiscovered') {
      return;
    }

    const markerFormId = content['markerFormId'];
    if (typeof markerFormId !== 'number') {
      return;
    }

    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const current: number[] = mp.get(actorId, 'private.discoveredMarkers') ?? [];
    if (!current.includes(markerFormId)) {
      mp.set(actorId, 'private.discoveredMarkers', [...current, markerFormId]);
      this.log(`[MarkerSystem] Actor ${actorId.toString(16)} discovered marker ${markerFormId.toString(16)}`);
    }
  }

  connect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const discovered: number[] = mp.get(actorId, 'private.discoveredMarkers') ?? [];
    if (discovered.length === 0) {
      return;
    }

    this.log(`[MarkerSystem] Restoring ${discovered.length} markers for actor ${actorId.toString(16)}`);
    ctx.svr.sendCustomPacket(
      userId,
      JSON.stringify({
        customPacketType: 'restoreMarkers',
        markerFormIds: discovered,
      }),
    );
  }
}
