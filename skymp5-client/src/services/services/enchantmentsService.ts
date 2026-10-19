import { Armor, Weapon } from 'skyrimPlatform';
import { MenuCloseEvent } from 'skyrimPlatform';

import { MsgType } from '../../messages';
import { logError, logTrace } from '../../logging';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';

export class EnchantmentsService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    controller.on('menuClose', (e: MenuCloseEvent) => this.onMenuClose(e));
    controller.emitter.on('customPacketMessage', (e) =>
      this.onCustomPacketMessage(e),
    );
  }

  private onMenuClose(e: MenuCloseEvent) {
    // Update the seen enchantment cache whenever the player browses inventory/containers/magic
    if (
      e.name === 'InventoryMenu' ||
      e.name === 'ContainerMenu' ||
      e.name === 'MagicMenu'
    ) {
      this.updateSeenEnchantments();
      return;
    }

    // After using the arcane enchanter, check if any cached enchantments are now known
    if (e.name === 'Crafting Menu') {
      this.updateSeenEnchantments();
      this.checkAndSyncKnownEnchantments();
    }
  }

  private updateSeenEnchantments() {
    const player = this.sp.Game.getPlayer();
    if (!player) {
      return;
    }

    const numItems = player.getNumItems();
    for (let i = 0; i < numItems; i++) {
      const form = player.getNthForm(i);
      if (!form) {
        continue;
      }
      // Collect base enchantments from weapons and armor
      const weapon = Weapon.from(form);
      if (weapon) {
        const enchantment = weapon.getEnchantment();
        if (enchantment) {
          this.seenEnchantmentIds.add(enchantment.getFormID());
        }
        continue;
      }
      const armor = Armor.from(form);
      if (armor) {
        const enchantment = armor.getEnchantment();
        if (enchantment) {
          this.seenEnchantmentIds.add(enchantment.getFormID());
        }
      }
    }
  }

  private checkAndSyncKnownEnchantments() {
    const newlyKnown: number[] = [];

    this.seenEnchantmentIds.forEach((formId) => {
      if (this.knownEnchantmentIds.has(formId)) {
        return;
      }
      const form = this.sp.Game.getFormEx(formId);
      if (!form) {
        return;
      }
      if (form.playerKnows()) {
        newlyKnown.push(formId);
        this.knownEnchantmentIds.add(formId);
      }
    });

    if (newlyKnown.length === 0) {
      return;
    }

    logTrace(this, `Learned ${newlyKnown.length} new enchantment(s), syncing to server`);

    const message: CustomPacketMessage = {
      t: MsgType.CustomPacket,
      contentJsonDump: JSON.stringify({
        customPacketType: 'learnedEnchantments',
        formIds: newlyKnown,
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

    if (content['customPacketType'] !== 'restoreKnownEnchantments') {
      return;
    }

    const formIds = content['formIds'];
    if (!Array.isArray(formIds)) {
      return;
    }

    let restored = 0;
    for (const id of formIds) {
      if (typeof id !== 'number') {
        continue;
      }
      const form = this.sp.Game.getFormEx(id);
      if (!form) {
        continue;
      }
      // TODO: Verify that Form.setPlayerKnows(true) is silent during restore.
      // If not, we need a skyrimPlatform API to prevent "You learned..." notifications (see skymp#TODO-ENCHANT-SILENT)
      form.setPlayerKnows(true);
      this.knownEnchantmentIds.add(id);
      this.seenEnchantmentIds.add(id);
      restored++;
    }

    if (restored > 0) {
      logTrace(this, `Restored ${restored} known enchantment(s)`);
    }
  }

  private seenEnchantmentIds = new Set<number>();
  private knownEnchantmentIds = new Set<number>();
}
