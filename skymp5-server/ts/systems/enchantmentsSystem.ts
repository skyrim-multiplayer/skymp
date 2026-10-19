import { Content, Log, System, SystemContext } from './system';

type Mp = any;

export class EnchantmentsSystem implements System {
  systemName = 'EnchantmentsSystem';

  constructor(private log: Log) {}

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext,
  ): void {
    if (type !== 'learnedEnchantments') {
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
    const current: number[] = mp.get(actorId, 'private.knownEnchantments') ?? [];
    const existing = new Set<number>(current);
    let changed = false;

    for (const id of formIds) {
      if (typeof id === 'number' && !existing.has(id)) {
        existing.add(id);
        changed = true;
      }
    }

    if (changed) {
      const updated: number[] = [];
      existing.forEach((v) => updated.push(v));
      mp.set(actorId, 'private.knownEnchantments', updated);
      this.log(`[EnchantmentsSystem] Actor ${actorId.toString(16)} learned ${updated.length} enchantment(s) total`);
    }
  }

  connect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const formIds: number[] = mp.get(actorId, 'private.knownEnchantments') ?? [];

    if (formIds.length === 0) {
      return;
    }

    this.log(`[EnchantmentsSystem] Restoring ${formIds.length} known enchantment(s) for actor ${actorId.toString(16)}`);
    ctx.svr.sendCustomPacket(
      userId,
      JSON.stringify({
        customPacketType: 'restoreKnownEnchantments',
        formIds,
      }),
    );
  }
}
