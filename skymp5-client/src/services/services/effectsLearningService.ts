import { MsgType } from '../../messages';
import { logError, logTrace } from '../../logging';
import { MenuCloseEvent } from 'skyrimPlatform';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';

// Map from ingredientFormId (hex string) to array of known effect indices (0-3)
type LearnedEffects = Record<string, number[]>;

export class EffectsLearningService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    // TODO: Replace menuClose polling with native ingredientLearned event once skyrimPlatform provides it.
    // This would follow the Markers pattern (locationDiscovery) and be more efficient + accurate.
    controller.on('menuClose', (e: MenuCloseEvent) => {
      if (e.name === 'InventoryMenu' || e.name === 'FavoritesMenu') {
        this.scanAndSync();
      }
    });
    controller.emitter.on('customPacketMessage', (e) =>
      this.onCustomPacketMessage(e),
    );
  }

  private scanAndSync() {
    const player = this.sp.Game.getPlayer();
    if (!player) {
      return;
    }

    const numItems = player.getNumItems();
    const current: LearnedEffects = {};

    for (let i = 0; i < numItems; i++) {
      const form = player.getNthForm(i);
      if (!form) {
        continue;
      }
      const ingredient = this.sp.Ingredient.from(form);
      if (!ingredient) {
        continue;
      }

      const numEffects = ingredient.getNumEffects();
      const known: number[] = [];
      for (let idx = 0; idx < numEffects; idx++) {
        if (ingredient.getIsNthEffectKnown(idx)) {
          known.push(idx);
        }
      }

      if (known.length > 0) {
        const key = form.getFormID().toString(16);
        current[key] = known;
      }
    }

    // Merge with previously known effects (carry over ingredients no longer in inventory)
    const merged: LearnedEffects = { ...current };
    for (const [key, indices] of Object.entries(this.knownEffects)) {
      if (!merged[key]) {
        merged[key] = indices;
      } else {
        const set = new Set<number>();
        for (const i of this.knownEffects[key]) set.add(i);
        for (const i of merged[key]) set.add(i);
        const arr: number[] = [];
        set.forEach((v) => arr.push(v));
        merged[key] = arr;
      }
    }

    // Check if anything changed
    const changed = JSON.stringify(merged) !== JSON.stringify(this.knownEffects);
    if (!changed) {
      return;
    }

    this.knownEffects = merged;

    logTrace(this, 'Ingredient effects changed, syncing to server');

    const message: CustomPacketMessage = {
      t: MsgType.CustomPacket,
      contentJsonDump: JSON.stringify({
        customPacketType: 'learnedIngredientEffects',
        effects: merged,
      }),
    };

    this.controller.emitter.emit('sendMessage', {
      message,
      reliability: 'reliable',
    });
  }

  private onCustomPacketMessage(
    event: ConnectionMessage<CustomPacketMessage>,
  ): void {
    let content: Record<string, unknown> = {};
    try {
      content = JSON.parse(event.message.contentJsonDump);
    } catch (e) {
      if (e instanceof SyntaxError) {
        logError(this, 'Failed to parse customPacket JSON', e.message);
        return;
      }
      throw e;
    }

    if (content['customPacketType'] !== 'restoreIngredientEffects') {
      return;
    }

    // TODO: Verify that Ingredient.learnEffect() is silent during restore.
    // If not, we need a skyrimPlatform API for silent effect learning (see skymp#TODO-EFFECTS-SILENT)

    const effects = content['effects'];
    if (!effects || typeof effects !== 'object' || Array.isArray(effects)) {
      return;
    }

    const effectsMap = effects as Record<string, unknown>;
    let restored = 0;

    for (const [hexId, indices] of Object.entries(effectsMap)) {
      if (!Array.isArray(indices)) {
        continue;
      }
      const formId = parseInt(hexId, 16);
      if (isNaN(formId)) {
        continue;
      }
      const form = this.sp.Game.getFormEx(formId);
      const ingredient = this.sp.Ingredient.from(form);
      if (!ingredient) {
        continue;
      }
      for (const idx of indices) {
        if (typeof idx === 'number') {
          ingredient.learnEffect(idx);
          restored++;
        }
      }
    }

    if (restored > 0) {
      logTrace(this, `Restored ${restored} learned ingredient effects`);
      // Update local cache so we don't immediately re-sync
      this.knownEffects = effectsMap as LearnedEffects;
    }
  }

  private knownEffects: LearnedEffects = {};
}
