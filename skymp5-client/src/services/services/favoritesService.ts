import { MenuCloseEvent } from 'skyrimPlatform';

import { MsgType } from '../../messages';
import { logError, logTrace } from '../../logging';
import { ConnectionMessage } from '../events/connectionMessage';
import { CustomPacketMessage } from '../messages/customPacketMessage';
import { ClientListener, CombinedController, Sp } from './clientListener';

export class FavoritesService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
    controller.on('menuClose', (e: MenuCloseEvent) => this.onMenuClose(e));
    controller.emitter.on('customPacketMessage', (e) =>
      this.onCustomPacketMessage(e),
    );
  }

  private onMenuClose(e: MenuCloseEvent) {
    if (e.name === 'FavoritesMenu' || e.name === 'InventoryMenu') {
      this.scanAndSync();
    }
  }

  private scanAndSync() {
    const player = this.sp.Game.getPlayer();
    if (!player) {
      return;
    }

    const numItems = player.getNumItems();
    const favorited: number[] = [];

    for (let i = 0; i < numItems; i++) {
      const form = player.getNthForm(i);
      if (!form) {
        continue;
      }
      if (this.sp.Game.isObjectFavorited(form)) {
        favorited.push(form.getFormID());
      }
    }

    const changed =
      JSON.stringify(favorited.slice().sort()) !==
      JSON.stringify(this.lastFavorites.slice().sort());

    if (!changed) {
      return;
    }

    this.lastFavorites = favorited;
    logTrace(this, `Favorites changed (${favorited.length}), syncing to server`);

    const message: CustomPacketMessage = {
      t: MsgType.CustomPacket,
      contentJsonDump: JSON.stringify({
        customPacketType: 'favoritesChanged',
        formIds: favorited,
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

    if (content['customPacketType'] !== 'restoreFavorites') {
      return;
    }

    const formIds = content['formIds'];
    if (!Array.isArray(formIds)) {
      return;
    }

    logTrace(this, `Server has ${formIds.length} persisted favorite(s)`);

    // TODO: SkyrimPlatform currently has no API to programmatically favorite an
    // item (Game.isObjectFavorited exists for reading but there is no setter).
    // When such an API is available, restore favorites here using it.
    // Track the persisted list locally so scanAndSync() does not immediately
    // overwrite the server state with an empty list on the next menu close.
    this.lastFavorites = formIds.filter((id) => typeof id === 'number');
  }

  private lastFavorites: number[] = [];
}
