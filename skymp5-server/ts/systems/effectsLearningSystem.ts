import { Content, Log, System, SystemContext } from './system';

type Mp = any;
type LearnedEffects = Record<string, number[]>;

export class EffectsLearningSystem implements System {
  systemName = 'EffectsLearningSystem';

  constructor(private log: Log) {}

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext,
  ): void {
    if (type !== 'learnedIngredientEffects') {
      return;
    }

    const effects = content['effects'];
    if (!effects || typeof effects !== 'object' || Array.isArray(effects)) {
      return;
    }

    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const current: LearnedEffects = mp.get(actorId, 'private.learnedIngredientEffects') ?? {};

    // Merge: union of known indices per ingredient
    let changed = false;
    for (const [hexId, indices] of Object.entries(effects as LearnedEffects)) {
      if (!Array.isArray(indices)) {
        continue;
      }
      const existing = new Set<number>(current[hexId] ?? []);
      const before = existing.size;
      for (const idx of indices) {
        if (typeof idx === 'number' && idx >= 0 && idx <= 3) {
          existing.add(idx);
        }
      }
      if (existing.size !== before) {
        current[hexId] = [...existing];
        changed = true;
      } else if (!current[hexId]) {
        current[hexId] = [...existing];
        changed = true;
      }
    }

    if (changed) {
      mp.set(actorId, 'private.learnedIngredientEffects', current);
      this.log(`[EffectsLearningSystem] Actor ${actorId.toString(16)} learned new ingredient effects`);
    }
  }

  connect(userId: number, ctx: SystemContext): void {
    const actorId = ctx.svr.getUserActor(userId);
    if (!actorId) {
      return;
    }

    const mp = ctx.svr as unknown as Mp;
    const effects: LearnedEffects = mp.get(actorId, 'private.learnedIngredientEffects') ?? {};

    if (Object.keys(effects).length === 0) {
      return;
    }

    this.log(`[EffectsLearningSystem] Restoring ingredient effects for actor ${actorId.toString(16)}`);
    ctx.svr.sendCustomPacket(
      userId,
      JSON.stringify({
        customPacketType: 'restoreIngredientEffects',
        effects,
      }),
    );
  }
}
