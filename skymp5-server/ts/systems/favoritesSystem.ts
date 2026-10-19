import { Content, Log, System, SystemContext } from './system';

type Mp = any;

export class FavoritesSystem implements System {
  systemName = 'FavoritesSystem';

  constructor(private log: Log) {}

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext,
  ): void {
    if (type !== 'favoritesChanged') {
      return;
    }

    const formIds = content['formIds'];
    if (!Array.isArray(formIds)) {
      return;
    }

    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const validated: number[] = formIds.filter((id) => typeof id === 'number');
    mp.set(actorId, 'private.favorites', validated);
    this.log(
      `[FavoritesSystem] Actor ${actorId.toString(16)} has ${validated.length} favorite(s)`,
    );
  }

  connect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const formIds: number[] = mp.get(actorId, 'private.favorites') ?? [];

    if (formIds.length === 0) {
      return;
    }

    this.log(
      `[FavoritesSystem] Restoring ${formIds.length} favorite(s) for actor ${actorId.toString(16)}`,
    );
    ctx.svr.sendCustomPacket(
      userId,
      JSON.stringify({
        customPacketType: 'restoreFavorites',
        formIds,
      }),
    );
  }
}
